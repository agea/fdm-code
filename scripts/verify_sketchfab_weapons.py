"""Read-only Unreal audit of saved catalog, animation and source dependencies."""
from pathlib import Path
from collections import Counter
import csv
import hashlib
import json
import math
import runpy
import unreal as ue

project=Path(ue.Paths.project_dir()).resolve()
root=project/'Assets/weapons/SourceAssets/SketchfabWeapons'
catalog=json.loads((root/'import-report.json').read_text())
source=json.loads((root/'source-manifest.json').read_text())
failures=[];results=[]
hand_math=runpy.run_path(str(project/'scripts/retarget_weapon_hands.py'))
xyz=hand_math['xyz'];add=hand_math['add'];sub=hand_math['sub'];mul=hand_math['mul'];length=hand_math['length'];rotate=hand_math['rotate'];quat=hand_math['quat'];source_position=hand_math['source_position']
def check(value,label):
    ue.log(('ER_REFERENCE_ASSET_PASS ' if value else 'ER_REFERENCE_ASSET_FAIL ')+label)
    if not value:failures.append(label)

check(len(catalog)==10,'complete_ten_slot_catalog')
check(len(source)==9,'nine_original_archives_with_attribution')
for item in source:
    path=root/item['uid']/'original.zip';digest=hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
    check(digest.hexdigest()==item['archive_sha256'],'original_archive_sha256_'+item['uid'])

fitted=ue.load_asset('/Game/EclipseRecoil/Characters/FirstPerson/SKEL_ER_FirstPerson')
for index,item in enumerate(catalog):
    key=item['key'];data=ue.load_asset(item['data']);mesh=ue.load_asset(item['model'])
    check(data and mesh and data.get_editor_property('mesh')==mesh,'saved_model_binding_'+key)
    check(all(s.material_interface and '/Sketchfab/' in s.material_interface.get_path_name() for s in mesh.get_editor_property('materials')),'private_material_bindings_'+key)
    for mat in {s.material_interface for s in mesh.get_editor_property('materials')}:
        rough=ue.MaterialEditingLibrary.get_material_property_input_node(mat,ue.MaterialProperty.MP_ROUGHNESS)
        if isinstance(rough,ue.MaterialExpressionOneMinus):
            inputs=ue.MaterialEditingLibrary.get_inputs_for_material_expression(mat,rough)
            check(len(inputs)==1 and inputs[0] is not None,'glossiness_inversion_connected_'+mat.get_name())
    effective=dict(item['stats'])
    effective.update(runpy.run_path(str(project/'scripts/weapon_gameplay_overrides.py'))['OVERRIDES'].get(key,{}))
    for name,expected in effective.items():
        check(abs(data.get_editor_property(name)-expected)<1e-5,'source_stat_'+key+'_'+name)
    idle=data.get_editor_property('idle_animation')
    check(idle and idle.get_editor_property('skeleton')==fitted,'existing_fps_skeleton_'+key)
    hand_options=ue.AnimPoseEvaluationOptions();hand_options.set_editor_property('should_retarget',False)
    hand_options.set_editor_property('optional_skeletal_mesh',ue.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/FirstPerson/SK_ER_Twinblast_Alpha_Officer_Forearms'))
    idle_pose=ue.AnimPoseExtensions.get_anim_pose_at_time(idle,0,hand_options)
    arm=ue.AnimPoseExtensions.get_bone_pose(idle_pose,'lowerarm_r',ue.AnimPoseSpaces.WORLD)
    check(min(arm.scale3d.y,arm.scale3d.z)>=.8,'thicker_fps_forearm_'+key)
    if key=='Pistol':
        wrist=ue.AnimPoseExtensions.get_bone_pose(idle_pose,'hand_r',ue.AnimPoseSpaces.WORLD)
        check(abs(wrist.scale3d.x-.62)<.001,'saved_pistol_larger_hand_proportions')
        delta=wrist.translation-arm.translation
        check(abs(delta.z)<math.hypot(delta.x,delta.y),'saved_pistol_relaxed_forearm')
        palm=ue.AnimPoseExtensions.get_socket_pose(idle_pose,'ER_Palm_R',ue.AnimPoseSpaces.WORLD)
        old=ue.AnimPoseExtensions.get_socket_pose(idle_pose,'HandGrip_R',ue.AnimPoseSpaces.WORLD)
        current=ue.MathLibrary.compose_transforms(data.get_editor_property('first_person_grip_transform'),palm)
        current.set_editor_property('scale3d',data.first_person_grip_transform.scale3d)
        grip_wrist=ue.MathLibrary.inverse_transform_location(current,wrist.translation)
        knuckle=ue.AnimPoseExtensions.get_bone_pose(idle_pose,'middle_01_r',ue.AnimPoseSpaces.WORLD)
        grip_knuckle=ue.MathLibrary.inverse_transform_location(current,knuckle.translation)
        check(abs(grip_wrist.y)<2 and .5<grip_knuckle.y<4,'saved_pistol_wrist_behind_grip_and_knuckles_at_flank')
        previous=ue.MathLibrary.compose_transforms(data.get_editor_property('grip_transform'),old)
        check(abs(sum(a*b for a,b in zip(quat(current.rotation),quat(previous.rotation))))>.999999,'saved_pistol_weapon_follows_fitted_wrist_angle')
        contact=ue.ERCharacter.get_weapon_surface_contact(mesh,ue.Vector(-5,100,2),ue.Vector(-5,-100,2),data.get_editor_property('weapon_idle_animation'))
        actual=ue.MathLibrary.transform_location(current,contact+ue.Vector(-.8,-3.4,-.7))
        expected=ue.MathLibrary.transform_location(palm,ue.Vector(0,0,-.35/palm.scale3d.z))
        check(data.get_editor_property('first_person_palm_grip') and (actual-expected).length()<.001,'saved_pistol_fitted_grip_anchor')
        fit=json.loads((root/'pistol-presentation.json').read_text())
        thumb=ue.AnimPoseExtensions.get_bone_pose(idle_pose,'thumb_03_r',ue.AnimPoseSpaces.WORLD)
        tip=ue.MathLibrary.transform_location(thumb,ue.Vector(*fit['thumb_tip_local']))
        expected=ue.MathLibrary.transform_location(current,ue.Vector(*fit['thumb_target_weapon']))
        check((tip-expected).length()<.03,'saved_pistol_thumb_wraps_opposite_grip_surface')
        presentation=runpy.run_path(str(project/'scripts/fit_pistol_presentation.py'))
        base=ue.get_default_object(ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter').generated_class()).first_person_body.get_editor_property('relative_location')
        skin=hand_options.get_editor_property('optional_skeletal_mesh')
        pistol_tip=presentation['camera_tip'](data,skin,base)
        body=ue.Transform(location=base+data.view_offset,rotation=ue.Rotator(pitch=0,yaw=-90,roll=0))
        camera_weapon=ue.MathLibrary.compose_transforms(current,body)
        direction=ue.MathLibrary.transform_direction(camera_weapon,ue.Vector(1,0,0))
        aim=pistol_tip+direction*((350-pistol_tip.x)/direction.x)
        check(direction.x>.98 and math.hypot(aim.y,aim.z)<.01,'saved_pistol_bore_converges_with_center_aim_ray')
        check(abs(pistol_tip.x-56)<.01,'saved_pistol_reference_depth')
        rifle_tip=presentation['camera_tip'](ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/Assault/DA_Assault'),skin,base)
        check(pistol_tip.x<rifle_tip.x and
              abs(pistol_tip.y/pistol_tip.x-.164)<.001 and
              abs(pistol_tip.z/pistol_tip.x+.1603125)<.001,
              'saved_pistol_matches_supplied_reference_framing')
    weapon_idle=data.get_editor_property('weapon_idle_animation')
    if weapon_idle:
        opt=ue.AnimPoseEvaluationOptions();opt.set_editor_property('should_retarget',False);opt.set_editor_property('optional_skeletal_mesh',mesh)
        first=ue.AnimPoseExtensions.get_anim_pose_at_time(weapon_idle,0,opt)
        still=True
        for ratio in [.25,.5,.75,1]:
            pose=ue.AnimPoseExtensions.get_anim_pose_at_time(weapon_idle,weapon_idle.get_play_length()*ratio,opt)
            for n in ue.AnimPoseExtensions.get_bone_names(first):
                a=ue.AnimPoseExtensions.get_bone_pose(first,n,ue.AnimPoseSpaces.WORLD);b=ue.AnimPoseExtensions.get_bone_pose(pose,n,ue.AnimPoseSpaces.WORLD)
                still &= length(sub(xyz(a.translation),xyz(b.translation)))<.001 and abs(sum(v*w for v,w in zip(quat(a.rotation),quat(b.rotation))))>.999999
        check(still,'idle_all_mechanical_bones_frozen_'+key)
    for label,entry in item['clips'].items():
        animation=ue.load_asset(entry['path'])
        check(animation and animation.get_editor_property('skeleton')==mesh.get_editor_property('skeleton'),'weapon_clip_skeleton_'+key+'_'+label)
    if 1<=index<=7:
        reload=data.get_editor_property('weapon_reload_animation');hands=data.get_editor_property('reload_animation')
        check(reload and hands and hands.get_editor_property('skeleton')==fitted,'authored_reload_and_skin_hands_'+key)
        options=ue.AnimPoseEvaluationOptions();options.set_editor_property('should_retarget',False);options.set_editor_property('optional_skeletal_mesh',mesh)
        start=ue.AnimPoseExtensions.get_anim_pose_at_time(reload,0,options)
        names=ue.AnimPoseExtensions.get_bone_names(start)
        moved=set()
        for ratio in [.25,.5,.75,1]:
            pose=ue.AnimPoseExtensions.get_anim_pose_at_time(reload,reload.get_play_length()*ratio,options)
            for bone in names:
                first=ue.AnimPoseExtensions.get_bone_pose(start,bone,ue.AnimPoseSpaces.WORLD)
                current=ue.AnimPoseExtensions.get_bone_pose(pose,bone,ue.AnimPoseSpaces.WORLD)
                if str(first)!=str(current):moved.add(str(bone))
        check(bool(moved),'saved_reload_contains_mechanical_motion_'+key)
        source_dir=root/item['source'].split('/')[0]
        motion=json.loads((source_dir/'derived'/('motion-'+key+'.json')).read_text())
        frames=motion['clips']['Reload'];fps=motion['fps']
        palm_grip=data.get_editor_property('first_person_palm_grip')
        right_socket='ER_Palm_R' if palm_grip else 'HandGrip_R'
        initial_right=ue.AnimPoseExtensions.get_socket_pose(idle_pose,right_socket,ue.AnimPoseSpaces.WORLD)
        origin=xyz(initial_right.translation)
        origin_offset=(0,0,0)
        if palm_grip:
            origin=xyz(ue.MathLibrary.transform_location(initial_right,data.get_editor_property('first_person_grip_transform').translation))
            origin_offset=sub(origin,xyz(initial_right.translation))
        scale=1. if palm_grip else initial_right.scale3d.x;error=0
        basis=runpy.run_path(str(project/'scripts/fit_pistol_presentation.py'))['motion_basis']() if palm_grip else (0,0,0,1)
        motion_position=lambda p:rotate(basis,source_position(p))
        motion_rotation=lambda matrix:hand_math['qmul'](hand_math['qmul'](basis,hand_math['source_rotation'](matrix)),hand_math['inverse'](basis))
        for i in range(round((len(frames)-1)*30/fps)+1):
            frame=frames[min(len(frames)-1,round(i*fps/30))]
            pose=ue.AnimPoseExtensions.get_anim_pose_at_time(hands,i/30,hand_options)
            for side in ['r','l']:
                if side=='l' and 'grip' not in frame.get('hands',{}).get(side,{}):continue
                p=[frame['root'][j][3] for j in range(3)] if side=='r' else frame['hands'][side]['grip']
                target=add(origin if side=='l' else xyz(initial_right.translation),mul(motion_position(p),scale))
                if side=='r':target=add(target,sub(origin_offset,rotate(motion_rotation(frame['root']),origin_offset)))
                actual=ue.AnimPoseExtensions.get_socket_pose(pose,right_socket if side=='r' else 'HandGrip_L',ue.AnimPoseSpaces.WORLD)
                error=max(error,length(sub(xyz(actual.translation),target)))
        check(error<.15,'saved_reload_palm_contact_under_1_5mm_'+key+' error_cm='+str(round(error,5)))
    results.append(dict(key=key,mesh=mesh.get_path_name(),material_slots=len(mesh.get_editor_property('materials')),clips=list(item['clips'])))

launcher=ue.load_asset(catalog[7]['data'])
awp=ue.load_asset(catalog[6]['data'])
check(all(abs(awp.get_editor_property(k)-v)<1e-5 for k,v in dict(damage=115,shot_interval=1.46,armor_ratio=1.95,range_modifier=.99).items()),'saved_sniper_uses_awp_row')
check(launcher.get_editor_property('projectile_mesh') is not None,'source_model_projectile_binding')
launcher_mesh=launcher.get_editor_property('mesh')
weapon_pose=ue.AnimPoseExtensions.get_anim_pose_at_time(launcher.get_editor_property('weapon_idle_animation'),0,ue.AnimPoseEvaluationOptions())
check(not any('scope' in str(n).lower() for n in ue.AnimPoseExtensions.get_bone_names(weapon_pose)),'milkor_scope_removed_from_derivative')
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget']:
    bp=ue.load_asset(path);defaults=ue.get_default_object(bp.generated_class())
    references=defaults.get_editor_property('reference_arsenal')
    check([v.get_path_name() for v in references[:10]]==[v['data'] for v in catalog] and len(references)==11 and references[10].get_editor_property('payload')==ue.ERPayload.CLAYMORE,'blueprint_catalog_'+path.rsplit('/',1)[1])
controller=ue.get_default_object(ue.load_asset('/Game/EclipseRecoil/Core/BP_ERPlayerController').generated_class())
check(controller.get_editor_property('reference_arsenal_menu'),'default_menu_uses_ten_slot_controls')
check(len(controller.get_editor_property('primary_options'))==7,'legacy_menu_and_server_loadout_choices_retained')
defaults=ue.get_default_object(ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter').generated_class())
palm_meshes=set()
for team in ['alpha','omega']:
    for appearance in defaults.get_editor_property(team+'_skins'):
        for mesh in [appearance.mesh,appearance.first_person_mesh]:
            palm_meshes.add(mesh.get_path_name())
            check(all(mesh.find_socket('ER_Palm_'+side.upper()) and
                      str(mesh.find_socket('ER_Palm_'+side.upper()).bone_name)=='hand_'+side
                      for side in ['l','r']),'saved_skin_palm_sockets_'+mesh.get_name())
check(len(palm_meshes)==40,'forty_owned_world_and_fps_palm_meshes')

counts=Counter()
for path in ue.EditorAssetLibrary.list_assets('/Game/EclipseRecoil/Weapons/Sketchfab',recursive=True,include_folder=False):
    asset=ue.load_asset(path)
    if asset:counts[asset.get_class().get_name()]+=1
receipt=dict(passed=not failures,failures=failures,original_archives=len(source),catalog_slots=len(catalog),asset_counts=dict(counts),weapons=results)
(project/'docs/REFERENCE_WEAPON_RESULTS.json').write_text(json.dumps(receipt,indent=2)+'\n')
check(not failures,'complete_saved_asset_audit')
if failures:raise RuntimeError('Reference weapon asset verification failed: '+str(failures))
