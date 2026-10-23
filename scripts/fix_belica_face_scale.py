"""Restore Belica's authored facial attachment scale in owned rifle clips.

The Manny IK export normalizes animated scales to one, enlarging Belica's
face independently of the head. Preserve all translation/rotation keys and
body tracks. Back up packages before saving; never change vendor meshes.
Run through UnrealEditor -run=pythonscript.
"""
from pathlib import Path
from datetime import datetime
import json
import zipfile
import unreal as ue

assets = ue.EditorAssetLibrary
folder = '/Game/EclipseRecoil/Characters/Teams/Alpha/Belica/Animations'
catalog = ue.get_default_object(assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
mesh = catalog.alpha_skins[5].mesh
reference = mesh.skeleton.get_reference_pose()
scale = ue.AnimPoseExtensions.get_bone_pose(reference, 'faceAttach', ue.AnimPoseSpaces.LOCAL).scale3d
assert abs(scale.x - .785146) < .0001 and abs(scale.y-scale.x) < .0001 and abs(scale.z-scale.x) < .0001
clips = [assets.load_asset(p) for p in assets.list_assets(folder, recursive=True)]
clips = [a for a in clips if isinstance(a, ue.AnimSequence)]
assert clips
project = Path(ue.Paths.project_dir())
backup = project/'Saved/AssetBackups'/('BelicaFaceScale-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
backup.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(backup, 'w', zipfile.ZIP_DEFLATED) as archive:
    for clip in clips:
        filename = project/'Content'/(clip.get_path_name().split('.')[0].removeprefix('/Game/')+'.uasset')
        archive.write(filename, str(filename.relative_to(project)))
results = []
for clip in clips:
    controller = clip.get_editor_property('controller')
    model = controller.get_model_interface()
    has_track = 'faceattach' in [str(n).lower() for n in model.get_bone_track_names()]
    options = ue.AnimPoseEvaluationOptions()
    options.optional_skeletal_mesh = mesh
    options.should_retarget = False
    count = model.get_number_of_frames()+1
    rate = model.get_frame_rate()
    keys = []
    for frame in range(count):
        pose = ue.AnimPoseExtensions.get_anim_pose_at_time(clip, frame*rate.denominator/rate.numerator, options)
        keys.append(ue.AnimPoseExtensions.get_bone_pose(pose, 'faceAttach', ue.AnimPoseSpaces.LOCAL))
    controller.open_bracket('Restore authored Belica facial scale', False)
    try:
        if not has_track:
            assert controller.add_bone_curve('faceAttach', False)
        assert controller.set_bone_track_keys('faceAttach', [k.translation for k in keys], [k.rotation for k in keys], [scale]*count, False)
    finally:
        controller.close_bracket(False)
    assert assets.save_loaded_asset(clip, False)
    for time in (0, clip.get_play_length()*.5, clip.get_play_length()):
        pose = ue.AnimPoseExtensions.get_anim_pose_at_time(clip, time, options)
        actual = ue.AnimPoseExtensions.get_bone_pose(pose, 'faceAttach', ue.AnimPoseSpaces.LOCAL).scale3d
        assert max(abs(actual.x-scale.x), abs(actual.y-scale.y), abs(actual.z-scale.z)) < .0001, clip.get_name()
    results.append({'clip': clip.get_path_name(), 'frames': count, 'previous_scale': keys[0].scale3d.x, 'scale': scale.x})
assert results
report = project/'Saved/Testing/belica-face-scale.json'
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps({'backup': str(backup), 'clips': results}, indent=2))
ue.log_warning('ER_BELICA_FACE_SCALE_PASS clips=%d report=%s' % (len(results), report))
