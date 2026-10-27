"""Fit the supplied 3.57–3.61s knife gesture to the independent FPS arms.

Run through Unreal Python. Gameplay values, source art and the base fitted idle
stay unchanged. The owned ready/stab clips can be rebuilt without accumulating
the source transform on a previously fitted pose.
"""
from pathlib import Path
import hashlib
import json
import runpy
import zipfile
import unreal as ue


def fit(settings):
    root = Path(ue.Paths.project_dir()).resolve()
    folder = '/Game/EclipseRecoil/Weapons/Sketchfab/Knife'
    current = ue.load_asset(folder+'/DA_Knife')
    if current and ue.EditorAssetLibrary.get_metadata_tag(current,'ERKnifeManualGripApproved')=='1':
        return runpy.run_path(str(root/'scripts/apply_manual_knife_grip.py'))['apply'](settings)
    source = next((root/'Assets/weapons/SourceAssets/SketchfabWeapons').glob('f3140*'))
    motion = json.loads((source/'derived/motion-Knife.json').read_text())
    helpers = runpy.run_path(str(root/'scripts/retarget_weapon_hands.py'))
    # Use the last fully drawn-back pose before the 3.57s counter; frame108
    # is already halfway through the thrust at the source 30Hz sampling rate.
    ready, tip = [motion['clips']['Source'][i-1] for i in (107, 110)]
    def sample(a, b, weight):
        qa = helpers['source_rotation'](a['root'])
        qb = helpers['source_rotation'](b['root'])
        if helpers['dot'](qa, qb) < 0:
            qb = helpers['mul'](qb, -1)
        q = helpers['unit'](tuple(x+(y-x)*weight for x,y in zip(qa,qb)))
        # Convert the interpolated rotation back from Unreal's Y/X reflection.
        columns = [helpers['rotate'](q, axis) for axis in ((1,0,0),(0,1,0),(0,0,1))]
        order = (1,0,2)
        matrix = [[columns[order[j]][order[i]] for j in range(3)]+[
            a['root'][i][3]+(b['root'][i][3]-a['root'][i][3])*weight] for i in range(3)]
        matrix.append([0,0,0,1])
        return dict(root=matrix)

    # 67ms outward stroke, 33ms contact, 200ms eased recovery. First/last
    # samples are exactly the ready pose, so the return to looping idle is clean.
    weights = [0, .5, 1, 1] + [1-(t*t*(3-2*t)) for t in (1/6,2/6,3/6,4/6,5/6,1)]
    frames = [sample(ready,tip,w) for w in weights]
    base = ue.load_asset(folder+'/A_Knife_Hands_Idle')
    if not base:
        raise RuntimeError('Missing preserved knife base pose')
    settings['idle_animation'] = helpers['retarget'](base, folder+'/A_Knife_Hands_Ready',
        [frames[0],frames[0]], 30, True,preserve_support_hand=True)
    settings['fire_animation'] = helpers['retarget'](base, folder+'/A_Knife_Hands_Stab',frames,30,True,preserve_support_hand=True)
    # Root motion is carried by the palm, not duplicated in the weapon skeleton.
    weapon_path = folder+'/A_Knife_Weapon_Stab'
    weapon = ue.load_asset(weapon_path) or ue.EditorAssetLibrary.duplicate_asset(
        settings['weapon_idle_animation'].get_path_name(), weapon_path)
    controller = weapon.get_editor_property('controller')
    controller.open_bracket('Match rigid knife clip to fitted stab',False)
    try:
        controller.set_frame_rate(ue.FrameRate(numerator=30,denominator=1),False)
        controller.set_number_of_frames(ue.FrameNumber(value=9),False)
    finally:
        controller.close_bracket(False)
    if not ue.EditorAssetLibrary.save_loaded_asset(weapon,False):
        raise RuntimeError('Cannot save rigid knife clip')
    settings['weapon_fire_animation'] = weapon
    # Move the fitted forearm/knife assembly away from the eye without scaling
    # the skin or changing the collision query/camera.
    settings['view_offset'] += ue.Vector(12,0,0)
    grip_receipt = runpy.run_path(str(root/'scripts/fit_knife_grip.py'))['fit'](settings)
    options = ue.AnimPoseEvaluationOptions()
    options.optional_skeletal_mesh = ue.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/FirstPerson/SK_ER_Twinblast_Alpha_Officer_Forearms')
    options.evaluation_type = ue.AnimDataEvalType.COMPRESSED
    poses = [ue.AnimPoseExtensions.get_anim_pose_at_time(settings['fire_animation'],t,options)
             for t in (0,2/30,9/30)]
    palms = [ue.AnimPoseExtensions.get_socket_pose(p,'HandGrip_R',ue.AnimPoseSpaces.WORLD) for p in poses]
    if (palms[0].translation-palms[2].translation).length()>.01:
        raise RuntimeError('Knife does not recover to its ready pose')
    # Fitted rig +Y becomes camera-forward through its existing -90 degree yaw.
    if palms[1].translation.y <= palms[0].translation.y+5:
        raise RuntimeError('Knife gesture must extend forward')
    original = ue.AnimPoseExtensions.get_anim_pose_at_time(base,0,options)
    max_finger_contact_error = 0.
    for pose in poses:
        for bone in ('upperarm_l','lowerarm_l','hand_l'):
            a = ue.AnimPoseExtensions.get_bone_pose(pose,bone,ue.AnimPoseSpaces.LOCAL)
            b = ue.AnimPoseExtensions.get_bone_pose(original,bone,ue.AnimPoseSpaces.LOCAL)
            if (a.translation-b.translation).length()>.01 or abs(helpers['dot'](helpers['quat'](a.rotation),helpers['quat'](b.rotation)))<.9999:
                raise RuntimeError('Knife changed the unused support arm')
        parent = ue.AnimPoseExtensions.get_socket_pose(pose,'ER_Palm_R',ue.AnimPoseSpaces.WORLD)
        weapon = ue.MathLibrary.compose_transforms(settings['first_person_grip_transform'],parent)
        weapon.set_editor_property('scale3d',settings['first_person_grip_transform'].scale3d)
        for finger,details in grip_receipt['fingers'].items():
            distal = ue.AnimPoseExtensions.get_bone_pose(pose,finger+'_03_r',ue.AnimPoseSpaces.WORLD)
            actual = ue.MathLibrary.transform_location(distal,ue.Vector(*details['tip_local_cm']))
            expected = ue.MathLibrary.transform_location(weapon,ue.Vector(*details['contact_weapon_cm']))
            error = (actual-expected).length()
            max_finger_contact_error = max(max_finger_contact_error,error)
            if error>.03:raise RuntimeError('Compressed knife grip misses handle: '+finger)
    return dict(source_frames=[107,110],source_seconds=[106/30,109/30],
        attack_seconds=2/30,total_seconds=9/30,view_advance_cm=12,
        forward_stroke_cm=palms[1].translation.y-palms[0].translation.y,grip=grip_receipt,
        compressed_fingertip_contact_error_cm=max_finger_contact_error)


def setup():
    root = Path(ue.Paths.project_dir()).resolve()
    folder = '/Game/EclipseRecoil/Weapons/Sketchfab/Knife'
    data = ue.load_asset(folder+'/DA_Knife')
    backup = root/'Assets/weapons/SourceAssets/SketchfabWeapons/KnifePresentationBackup.zip'
    if not backup.exists():
        files = sorted((root/'Assets/weapons/Content/EclipseRecoil/Weapons/Sketchfab/Knife').rglob('*.uasset'))
        with zipfile.ZipFile(backup,'x',zipfile.ZIP_DEFLATED) as archive:
            hashes = {}
            for file in files:
                name = str(file.relative_to(root/'Assets/weapons'))
                content = file.read_bytes()
                archive.writestr(name,content)
                hashes[name] = hashlib.sha256(content).hexdigest()
            archive.writestr('manifest.json',json.dumps(hashes))
        with zipfile.ZipFile(backup) as archive:
            if archive.testzip() or any(hashlib.sha256(archive.read(n)).hexdigest()!=h for n,h in hashes.items()):
                raise RuntimeError('Knife rollback verification failed')
    fields = ('damage','hitscan_range','shot_interval','armor_ratio','magazine_size','payload')
    before = {p:data.get_editor_property(p) for p in fields}
    marker = 'ERKnifePresentationVersion'
    baseline = 'ERKnifeBaseViewOffset'
    value = ue.EditorAssetLibrary.get_metadata_tag(data,baseline)
    original = json.loads(value) if value else [data.view_offset.x,data.view_offset.y,data.view_offset.z]
    settings = dict(weapon_idle_animation=data.weapon_idle_animation,view_offset=ue.Vector(*original))
    receipt = fit(settings)
    for key,value in settings.items():
        data.set_editor_property(key,value)
    if before != {p:data.get_editor_property(p) for p in fields}:
        raise RuntimeError('Knife gameplay changed')
    ue.EditorAssetLibrary.set_metadata_tag(data,baseline,json.dumps(original))
    ue.EditorAssetLibrary.set_metadata_tag(data,marker,'manual-1' if receipt.get('manual_edit') else '5')
    if not ue.EditorAssetLibrary.save_loaded_asset(data,False):
        raise RuntimeError('Cannot save knife presentation')
    receipt['gameplay'] = {p:str(v) for p,v in before.items()}
    (backup.parent/'knife-presentation.json').write_text(json.dumps(receipt,indent=2)+'\n')
    ue.log('ER_KNIFE_PRESENTATION PASS '+json.dumps(receipt))


if __name__ == '__main__':
    setup()
