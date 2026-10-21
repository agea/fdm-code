"""Preserve fitted FPS poses in compressed builds without raw-data debug flags.

Run once through Unreal Python after compiling the Editor module. The migration
backs up the owned skeleton/animations privately, bakes only translations whose
retarget modes must change, and compares every frame before saving. Subsequent
runs verify the saved migration; -ERFPSCookVerify requires it to exist already.
"""
from pathlib import Path
import hashlib
import json
import math
import time
import zipfile
import unreal as ue

ROOT = Path(ue.Paths.project_dir()).resolve()
RIG = '/Game/EclipseRecoil/Characters/FirstPerson/SKEL_ER_FirstPerson'
MARKER = 'ERCompressedFirstPersonPoseVersion'
skeleton = ue.load_asset(RIG)
mesh = ue.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/FirstPerson/SK_ER_Twinblast_Alpha_Officer_Forearms')
if not skeleton or not mesh:
    raise RuntimeError('Missing independent FPS rig')
migrated = ue.EditorAssetLibrary.get_metadata_tag(skeleton, MARKER) == '1'
if 'ERFPSCookVerify' in ue.SystemLibrary.get_command_line() and not migrated:
    raise RuntimeError('Run the FPS translation migration before packaging')

pistol = ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/Pistol/DA_Pistol')
pistol_clips = {pistol.get_editor_property(p) for p in ['idle_animation', 'fire_animation', 'reload_animation']}
clips = []
for item in ue.AssetRegistryHelpers.get_asset_registry().get_assets_by_path('/Game/EclipseRecoil', recursive=True):
    if str(item.asset_class_path.asset_name) == 'AnimSequence':
        clip = item.get_asset()
        if clip.get_editor_property('skeleton') == skeleton:
            clips.append(clip)
clips.sort(key=lambda c: c.get_path_name())
if not all(c in clips for c in pistol_clips if c):
    raise RuntimeError('Pistol animation closure is incomplete')

def pose(clip, t, retarget=True, compressed=False, skin=mesh):
    options = ue.AnimPoseEvaluationOptions()
    options.should_retarget = retarget
    options.optional_skeletal_mesh = skin
    options.evaluation_type = ue.AnimDataEvalType.COMPRESSED if compressed else ue.AnimDataEvalType.RAW
    return ue.AnimPoseExtensions.get_anim_pose_at_time(clip, t, options)

def local(p, bone):
    return ue.AnimPoseExtensions.get_bone_pose(p, bone, ue.AnimPoseSpaces.LOCAL)

def times(clip):
    if migrated:
        return [0., clip.get_play_length() * .5, clip.get_play_length()]
    model = clip.get_editor_property('controller').get_model_interface()
    rate = model.get_frame_rate()
    return [min(clip.get_play_length(), i * rate.denominator / rate.numerator)
            for i in range(model.get_number_of_frames() + 1)]

# Find the exact tracks the old pistol debug mode kept from being retargeted.
changed_bones = set()
if not migrated:
    for clip in pistol_clips:
        if not clip:
            continue
        for t in times(clip):
            raw, retargeted = pose(clip, t, False), pose(clip, t)
            for bone in ue.AnimPoseExtensions.get_bone_names(raw):
                if (local(raw, bone).translation - local(retargeted, bone).translation).length() > .0001:
                    changed_bones.add(str(bone))
    if not changed_bones:
        raise RuntimeError('Expected translated pistol shoulder/pelvis tracks')

# Snapshot all expected frames before changing the shared rig. Other weapons
# retain their currently retargeted local poses, including reload trajectories.
expected = {}
tracks = {}
for clip in clips:
    frames = [pose(clip, t, clip not in pistol_clips or migrated) for t in times(clip)]
    names = list(ue.AnimPoseExtensions.get_bone_names(frames[0]))
    expected[clip] = (names, [[local(p, n) for n in names] for p in frames])
    present = {str(n) for n in clip.get_editor_property('controller').get_model_interface().get_bone_track_names()}
    tracks[clip] = {n: [local(p, n) for p in frames] for n in changed_bones if n in present}

backup = None
if not migrated:
    backup = ROOT.parent / '_backups' / ('fps-cooked-translations-' + time.strftime('%Y%m%d-%H%M%S') + '.zip')
    backup.parent.mkdir(parents=True, exist_ok=True)
    manifest = {}
    with zipfile.ZipFile(backup, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
        for asset in [skeleton] + clips:
            relative = 'Content/' + asset.get_path_name().split('.')[0].removeprefix('/Game/') + '.uasset'
            content = (ROOT / relative).read_bytes()
            archive.writestr(relative, content)
            manifest[relative] = hashlib.sha256(content).hexdigest()
        archive.writestr('manifest.json', json.dumps(manifest, indent=2))
    with zipfile.ZipFile(backup) as archive:
        if archive.testzip() or any(hashlib.sha256(archive.read(n)).hexdigest() != h for n, h in manifest.items()):
            raise RuntimeError('FPS rollback archive failed verification')
    for clip, bones in tracks.items():
        if not bones:
            continue
        controller = clip.get_editor_property('controller')
        controller.open_bracket('Bake effective FPS translations for cooked playback', False)
        try:
            for bone, keys in bones.items():
                if not controller.set_bone_track_keys(bone, [k.translation for k in keys],
                        [k.rotation for k in keys], [k.scale3d for k in keys], False):
                    raise RuntimeError('Cannot bake ' + clip.get_name() + ':' + bone)
        finally:
            controller.close_bracket(False)
    if not ue.ERCharacter.configure_first_person_translation_tracks(skeleton, sorted(changed_bones)):
        raise RuntimeError('Cannot enable authored FPS translation tracks')

max_translation_error = 0.
for clip, (names, frames) in expected.items():
    for t, old in zip(times(clip), frames):
        current = pose(clip, t)
        for bone, before in zip(names, old):
            after = local(current, bone)
            error = (before.translation - after.translation).length()
            max_translation_error = max(max_translation_error, error)
            dot = abs(sum(getattr(before.rotation, n) * getattr(after.rotation, n) for n in ['x', 'y', 'z', 'w']))
            if error > .002 or dot < .999999 or (before.scale3d - after.scale3d).length() > .0001:
                raise RuntimeError('FPS pose changed: %s %s t=%.5f error=%.5f' % (clip.get_name(), bone, t, error))
    ue.log('ER_FPS_COOK_POSE_PASS ' + clip.get_name())

if not migrated:
    ue.EditorAssetLibrary.set_metadata_tag(skeleton, MARKER, '1')
    for asset in [skeleton] + [c for c in clips if tracks[c]]:
        if not ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
            raise RuntimeError('Cannot save ' + asset.get_path_name())

# Exercise the compressed path with retargeting enabled on all twenty forearms.
# The fitted barrel is checked against a 350cm convergence point. Default lossy
# rotation compression introduces a visible-axis error even when hand positions
# agree. Store these three small clips as full-precision compressed tracks.
compression_path = '/Game/EclipseRecoil/Characters/FirstPerson/BC_ER_PistolPrecise'
compression = ue.load_asset(compression_path)
compression_changed = False
if not compression:
    compression = ue.AssetToolsHelpers.get_asset_tools().create_asset(
        'BC_ER_PistolPrecise', '/Game/EclipseRecoil/Characters/FirstPerson',
        ue.AnimBoneCompressionSettings, ue.AnimBoneCompressionSettingsFactory())
    codec = ue.new_object(ue.load_class(None, '/Script/Engine.AnimCompress_BitwiseCompressOnly'), outer=compression)
    compression.set_editor_property('codecs', [codec])
    compression.set_editor_property('error_threshold', 0.)
    compression.set_editor_property('force_below_threshold', True)
if ue.EditorAssetLibrary.get_metadata_tag(compression, 'ERPrecisePistolCompressionVersion') != '1':
    # This legacy codec's enum has no Python wrapper. Unreal's SET command
    # imports the enum text on this specific owned object, with edit notices.
    # ACF_None keeps all four quaternion components; reconstructing W near a
    # 180-degree hand rotation loses enough precision to move the barrel axis.
    for codec in compression.get_editor_property('codecs'):
        for field in ['TranslationCompressionFormat', 'RotationCompressionFormat', 'ScaleCompressionFormat']:
            ue.SystemLibrary.execute_console_command(None, 'set "' + codec.get_path_name() + '" ' + field + ' ACF_None')
    ue.EditorAssetLibrary.set_metadata_tag(compression, 'ERPrecisePistolCompressionVersion', '1')
    if not ue.EditorAssetLibrary.save_loaded_asset(compression, only_if_is_dirty=False):
        raise RuntimeError('Cannot save precise pistol compression settings')
    compression_changed = True
for clip in pistol_clips:
    if clip and (compression_changed or ue.EditorAssetLibrary.get_metadata_tag(clip, 'ERPrecisePistolCompressionVersion') != '1'):
        # A settings object edited in place can leave the sequence's previously
        # evaluated data cached. Change the binding to request fresh compression.
        clip.set_editor_property('bone_compression_settings', None)
        clip.set_editor_property('bone_compression_settings', compression)
        if not ue.EditorAssetLibrary.save_loaded_asset(clip, only_if_is_dirty=False):
            raise RuntimeError('Cannot save precise pistol clip ' + clip.get_name())

skins = [i.get_asset() for team in ['Alpha', 'Omega']
         for i in ue.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(
             '/Game/EclipseRecoil/Characters/Teams/' + team + '/FirstPerson', recursive=True)
         if str(i.asset_class_path.asset_name) == 'SkeletalMesh']
compressed_error = 0.
rotation_error = 0.
for skin in skins:
    for clip in pistol_clips:
        if not clip:
            continue
        for t in [0., clip.get_play_length() * .5, clip.get_play_length()]:
            a, b = pose(clip, t, skin=skin), pose(clip, t, compressed=True, skin=skin)
            for bone in ['hand_r', 'lowerarm_r', 'thumb_03_r', 'hand_l']:
                av, bv = [ue.AnimPoseExtensions.get_bone_pose(p, bone, ue.AnimPoseSpaces.WORLD) for p in [a, b]]
                error = (av.translation - bv.translation).length()
                compressed_error = max(compressed_error, error)
                qa = [getattr(av.rotation, n) for n in ['x', 'y', 'z', 'w']]
                qb = [getattr(bv.rotation, n) for n in ['x', 'y', 'z', 'w']]
                qa = [v / math.sqrt(sum(x*x for x in qa)) for v in qa]
                qb = [v / math.sqrt(sum(x*x for x in qb)) for v in qb]
                angle = 2 * math.degrees(min(math.dist(qa, qb), math.dist(qa, [-v for v in qb])))
                rotation_error = max(rotation_error, angle)
                if error > .2:
                    raise RuntimeError('Compressed FPS mismatch: %s %s %.4fcm' % (skin.get_name(), clip.get_name(), error))
                if angle > .001 and bone == 'hand_r' and clip == pistol.get_editor_property('idle_animation') and t == 0:
                    raise RuntimeError('Compressed pistol aim mismatch: %s %s t=%.5f %.6f degrees' % (skin.get_name(), bone, t, angle))

report = dict(result='PASS', animations=len(clips), skins=len(skins), translated_bones=sorted(changed_bones),
              max_pose_error_cm=max_translation_error, max_compressed_hand_error_cm=compressed_error,
              max_compressed_rotation_error_degrees=rotation_error,
              backup=str(backup) if backup else None, already_migrated=migrated)
for clip in pistol_clips:
    if clip and ue.EditorAssetLibrary.get_metadata_tag(clip, 'ERPrecisePistolCompressionVersion') != '1':
        ue.EditorAssetLibrary.set_metadata_tag(clip, 'ERPrecisePistolCompressionVersion', '1')
        ue.EditorAssetLibrary.save_loaded_asset(clip, only_if_is_dirty=False)
out = ROOT / 'Saved/Testing/cooked-first-person-report.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, indent=2) + '\n')
ue.log('ER_FPS_COOK_COMPLETE ' + json.dumps(report))
