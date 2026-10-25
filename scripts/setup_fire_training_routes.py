"""Fire Training route revision2: fortified bases and high steel routes just below the lighting.

Run via UnrealEditor -run=pythonscript. Preserve all existing art, theme bindings
and size tuning; back up the current asset before saving only this
recipe and its original industrial parapet reference.
"""
import hashlib
import json
import shutil
from pathlib import Path
import unreal as ue

project = Path(ue.Paths.project_dir()).resolve()
assets = ue.EditorAssetLibrary
base = '/Game/EclipseRecoil/Art/MapTemplates/'
template = assets.load_asset(base + 'DA_FireTraining')
central = assets.load_asset(base + 'DA_CentralBuilding')
if not template or not central:
    raise RuntimeError('Missing Fire Training / industrial recipe')
revision = template.get_editor_property('revision')
if assets.get_metadata_tag(template, 'ERFireTrainingRoutesRevision') == '2':
    print('ER_FIRE_TRAINING_ROUTES already revision', revision)
elif revision < 3:
    raise RuntimeError('Apply setup_fire_training_scale.py (revision3) first')
else:
    file = project/'Assets/maps/Content/EclipseRecoil/Art/MapTemplates/DA_FireTraining.uasset'
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    backup = project/('Saved/FireTrainingBackup/before-routes-revision' + str(revision))
    backup.mkdir(parents=True, exist_ok=True)
    target = backup/file.name
    if not target.exists():
        shutil.copy2(file, target)
    if sha(target) != sha(file):
        raise RuntimeError('Backup does not match the current recipe')
    module = central.get_editor_property('central_building').get_editor_property('fence_module')
    if not module:
        raise RuntimeError('Missing original industrial steel barrier')
    candidate = ue.new_object(ue.ERMapTemplate)
    for key in ['template_id','source_reference','archetype','symmetry','arena_size','fire_training','theme_library']:
        candidate.set_editor_property(key, template.get_editor_property(key))
    recipe = candidate.get_editor_property('fire_training')
    recipe.set_editor_property('parapet_module', module)
    candidate.set_editor_property('fire_training', recipe)
    candidate.set_editor_property('revision', revision + 1)
    for seed in list(range(512)) + [7919,-17,2147483647,-2147483648]:
        if not candidate.validate_layout(seed):
            raise RuntimeError('Invalid Fire Training seed ' + str(seed))
    template.set_editor_property('fire_training', recipe)
    template.set_editor_property('revision', revision + 1)
    assets.set_metadata_tag(template, 'ERFireTrainingRoutesRevision', '2')
    if not assets.save_loaded_asset(template, False):
        raise RuntimeError('Cannot save Fire Training')
    (backup/'receipt.json').write_text(json.dumps(dict(before_revision=revision,after_revision=revision+1,before_sha256=sha(target),after_sha256=sha(file),
        parapet=module.get_path_name(),validated_seeds=516), indent=2) + '\n')
    print('ER_FIRE_TRAINING_ROUTES_SAVED seeds=516')
