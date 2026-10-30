"""Calibrate world FeetOffset from animated boot geometry, without changing art.
Run with Unreal's PythonScript commandlet. Idempotent; backs up the three catalogs.
The capsule's normal floor clearance supplies the small visible ground gap.
"""
from pathlib import Path
import datetime
import json
import zipfile
import unreal as u

ROOT = '/Game/EclipseRecoil/Core/'
NAMES = ['BP_ERCharacter', 'BP_ERTrainingTarget', 'BP_ERCorpse']
project = Path(u.Paths.project_dir()).resolve()
out = project / 'Saved/Testing/CharacterGrounding'
out.mkdir(parents=True, exist_ok=True)
stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
blueprints = [u.load_asset(ROOT + name) for name in NAMES]
assert all(blueprints)
with zipfile.ZipFile(out / (stamp + '-catalogs.zip'), 'w', zipfile.ZIP_DEFLATED) as backup:
    for name in NAMES:
        file = project / 'Content/EclipseRecoil/Core' / (name + '.uasset')
        backup.write(file, file.name)

cache = {}
def sole_height(a):
    key = (a.mesh.get_path_name(), a.corpse_pose.get_path_name(), a.scale)
    if key not in cache:
        vertices = {b: list(u.ERCharacter.get_presentation_vertices(a.mesh, b))
                    for b in ['foot_l', 'foot_r', 'ball_l', 'ball_r']}
        assert all(vertices.values()), key
        options = u.AnimPoseEvaluationOptions()
        options.optional_skeletal_mesh = a.mesh
        samples = []
        duration = a.corpse_pose.get_play_length()
        for frame in range(17):
            pose = u.AnimPoseExtensions.get_anim_pose_at_time(a.corpse_pose, duration * frame / 16, options)
            samples.append(min(u.MathLibrary.transform_location(
                u.AnimPoseExtensions.get_bone_pose(pose, bone, u.AnimPoseSpaces.WORLD), vertex).z * a.scale
                for bone, points in vertices.items() for vertex in points))
        cache[key] = min(samples)
    return cache[key]

rows = []
for bp in blueprints:
    cdo = u.get_default_object(bp.generated_class())
    for team in ['alpha', 'omega']:
        choices = list(cdo.get_editor_property(team + '_skins'))
        for index, a in enumerate(choices):
            old = a.feet_offset
            sole = sole_height(a)
            new = -92.5 - sole
            a.set_editor_property('feet_offset', new)
            assert abs(92.5 + a.feet_offset + sole) < .001
            rows.append(dict(catalog=bp.get_name(), team=team, index=index, name=a.display_name,
                             before=old, after=a.feet_offset, lift_cm=a.feet_offset-old))
        cdo.set_editor_property(team + '_skins', choices)
        # The fallback must agree with selectable skin zero.
        fallback = cdo.get_editor_property(team + '_appearance')
        fallback.set_editor_property('feet_offset', -92.5 - sole_height(fallback))
        cdo.set_editor_property(team + '_appearance', fallback)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp, False)
(out / (stamp + '-calibration.json')).write_text(json.dumps(rows, indent=2))
u.log('ER_CHARACTER_GROUNDING_PASS skins=%d meshes=%d' % (len(rows), len(cache)))
