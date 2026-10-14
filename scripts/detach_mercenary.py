"""One-time Unreal migration: preserve fitted FPS motion, remove vendor dependencies.

Duplicates the small fitted rig/post-process/poses, without mercenary geometry
or materials. Vendor packages and old owned packages remain untouched so they
can be archived/deleted separately. Run after compiling the Editor module.
"""
import json
import math
from pathlib import Path
import shutil
import unreal as ue

assets = ue.EditorAssetLibrary
root = '/Game/EclipseRecoil/Characters/FirstPerson'
legacy = '/Game/EclipseRecoil/Characters/Mercenary'
project = Path(ue.Paths.project_dir()).resolve()
backup = project / 'Saved/AssetAudit/MercenaryDetachBackup'
backup.mkdir(parents=True, exist_ok=True)

def load(path):
    asset = ue.load_asset(path)
    if not asset:
        raise RuntimeError('Missing migration input: ' + path)
    return asset

def save(asset):
    if not assets.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Save failed: ' + asset.get_path_name())

def duplicate(source, path):
    return ue.load_asset(path) or assets.duplicate_asset(source.get_path_name(), path)

# Preserve current project packages before changing any of them. Vendor data
# is never modified by this migration. Do not overwrite the initial backup.
for path in (project / 'Content/EclipseRecoil').rglob('*.uasset'):
    if '/Characters/Mercenary/' in path.as_posix() or '/Characters/FirstPerson/' in path.as_posix():
        continue
    destination = backup / path.relative_to(project)
    if not destination.exists():
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, destination)

old_rig = load(legacy + '/SK_ER_Mercenary_Target')
old_skeleton = old_rig.skeleton
player = load('/Game/EclipseRecoil/Core/BP_ERCharacter')
catalog = ue.get_default_object(player.generated_class())
preview = catalog.alpha_skins[0].first_person_mesh
if not preview:
    raise RuntimeError('Generate skin forearms first')
old_postprocess_class = old_rig.get_editor_property('post_process_anim_blueprint')
old_postprocess = assets.load_asset(old_postprocess_class.get_path_name().removesuffix('_C')) if old_postprocess_class else None
assets.make_directory(root)

def grip_samples(mesh, animations):
    options = ue.AnimPoseEvaluationOptions()
    options.set_editor_property('optional_skeletal_mesh', mesh)
    result = []
    for anim in animations:
        for time in [0, 1/60, .05, .1]:
            pose = ue.AnimPoseExtensions.get_anim_pose_at_time(anim, time, options)
            result.append([ue.AnimPoseExtensions.get_socket_pose(pose, socket, ue.AnimPoseSpaces.WORLD)
                           for socket in ['HandGrip_R', 'HandGrip_L']])
    return result

old_animations = []
for data in catalog.arsenal:
    old_animations.extend([load(anim.get_path_name().replace(root + '/', legacy + '/'))
                           for anim in [data.idle_animation, data.fire_animation]])
before = grip_samples(old_rig, old_animations)
skeleton = duplicate(old_skeleton, root + '/SKEL_ER_FirstPerson')
postprocess = duplicate(old_postprocess, root + '/ABP_ER_FirstPerson_PostProcess') if old_postprocess else None
sources = [old_skeleton, old_rig]
replacements = [skeleton, preview]
if old_postprocess:
    sources.append(old_postprocess)
    replacements.append(postprocess)
vendor_skeleton = load('/Game/Mercenary_Male_WaveforceStudio/Mesh/SKEL_MER_ME_Body_Skeleton')
vendor_preview = load('/Game/Mercenary_Male_WaveforceStudio/Mesh/Modular/SK_MER_ME_Body')
sources += [vendor_skeleton, vendor_preview]
replacements += [skeleton, preview]

animations = {}
for path in assets.list_assets(legacy, recursive=True, include_folder=False):
    source = load(path)
    if isinstance(source, ue.AnimSequence):
        suffix = path.split('.', 1)[0].removeprefix(legacy + '/')
        suffix = suffix.replace('A_ER_Mercenary_Rifle_', 'A_ER_Rifle_')
        destination = duplicate(source, root + '/' + suffix)
        animations[source.get_path_name()] = destination
        sources.append(source)
        replacements.append(destination)

if not ue.ERCharacter.prepare_first_person_skeleton(skeleton, preview):
    raise RuntimeError('Cannot detach FPS skeleton metadata')
if postprocess:
    if not ue.ERCharacter.replace_owned_asset_references(postprocess, sources, replacements):
        raise RuntimeError('Cannot detach FPS post-process graph')
    ue.BlueprintEditorLibrary.compile_blueprint(postprocess)
    save(postprocess)

# Replace references in owned packages, including graph defaults/components,
# weapon data and animation retarget/preview metadata. Skip obsolete packages.
for path in assets.list_assets('/Game/EclipseRecoil', recursive=True, include_folder=False):
    if path.startswith(legacy + '/'):
        continue
    asset = load(path)
    assert ue.ERCharacter.replace_owned_asset_references(asset, sources, replacements), path
    if isinstance(asset, ue.AnimSequence) and asset.get_path_name().startswith(root + '/'):
        assert ue.ERCharacter.set_mercenary_animation_skeleton(asset, preview), path
    if isinstance(asset, ue.Blueprint):
        ue.BlueprintEditorLibrary.compile_blueprint(asset)
    save(asset)

assert ue.ERCharacter.prepare_first_person_skeleton(skeleton, preview)
save(skeleton)

for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget']:
    bp = load(path)
    defaults = ue.get_default_object(bp.generated_class())
    defaults.first_person_body.set_skeletal_mesh_asset(preview)
    defaults.first_person_body.set_editor_property('override_materials', [])
    defaults.set_editor_property('first_person_visible_material_slots', [])
    defaults.set_editor_property('alpha_first_person_materials', [])
    defaults.set_editor_property('omega_first_person_materials', [])
    defaults.set_editor_property('alpha_body_materials', [])
    defaults.set_editor_property('omega_body_materials', [])
    alpha = defaults.alpha_skins[0]
    defaults.mesh.set_skeletal_mesh_asset(alpha.mesh)
    defaults.mesh.set_physics_asset(alpha.mesh.physics_asset, True)
    defaults.mesh.set_editor_property('override_materials', [])
    defaults.mesh.set_anim_instance_class(alpha.animation_class)
    defaults.mesh.set_editor_property('relative_location', ue.Vector(0, 0, alpha.feet_offset))
    defaults.mesh.set_editor_property('relative_scale3d', ue.Vector(alpha.scale, alpha.scale, alpha.scale))
    defaults.mesh.set_editor_property('relative_rotation', ue.Rotator(pitch=0, yaw=-90, roll=0))
    animation = defaults.first_person_body.get_editor_property('animation_data')
    animation.set_editor_property('anim_to_play', animations[old_animations[0].get_path_name()])
    defaults.first_person_body.set_editor_property('animation_data', animation)
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    save(bp)

corpse = load('/Game/EclipseRecoil/Core/BP_ERCorpse')
defaults = ue.get_default_object(corpse.generated_class())
alpha = defaults.alpha_skins[0]
defaults.body.set_skeletal_mesh_asset(alpha.mesh)
defaults.body.set_physics_asset(alpha.mesh.physics_asset, True)
defaults.body.set_editor_property('override_materials', [])
animation = defaults.body.get_editor_property('animation_data')
animation.set_editor_property('anim_to_play', alpha.corpse_pose)
defaults.body.set_editor_property('animation_data', animation)
defaults.set_editor_property('alpha_materials', [])
defaults.set_editor_property('omega_materials', [])
ue.BlueprintEditorLibrary.compile_blueprint(corpse)
save(corpse)

after_animations = [animations[x.get_path_name()] for x in old_animations]
after = grip_samples(preview, after_animations)
maximum = 0
for prior, current in zip(before, after):
    for a, b in zip(prior, current):
        distance = math.dist((a.translation.x,a.translation.y,a.translation.z), (b.translation.x,b.translation.y,b.translation.z))
        maximum = max(maximum, distance)
        dot = abs(a.rotation.x*b.rotation.x+a.rotation.y*b.rotation.y+a.rotation.z*b.rotation.z+a.rotation.w*b.rotation.w)
        scale = math.dist((a.scale3d.x,a.scale3d.y,a.scale3d.z), (b.scale3d.x,b.scale3d.y,b.scale3d.z))
        assert distance < .01 and abs(1-dot) < .0001 and scale < .001, ('Grip changed', distance, dot, scale)
(project / 'Saved/AssetAudit/mercenary-detach.json').write_text(json.dumps({
    'animations': len(animations), 'samples': len(after), 'grips_per_sample': 2,
    'maximum_grip_difference_cm': maximum, 'skeleton': skeleton.get_path_name(),
    'postprocess': postprocess.get_path_name() if postprocess else None, 'backup': str(backup),
    'vendor_geometry_copied': False, 'vendor_materials_copied': False,
    'licensing_note': 'Preserved small fitted rig/post-process derivatives retain their original provenance; this migration does not grant new redistribution rights.'}, indent=2)+'\n')
ue.log('ER_MERCENARY_DETACHED animations=%d samples=%d grip_error_cm=%.6f' % (len(animations), len(after), maximum))
