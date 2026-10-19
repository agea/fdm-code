"""Version the ladder recipe and refresh the PCG authoring snapshot in Unreal."""
from pathlib import Path
import runpy
import unreal as ue
assets=ue.EditorAssetLibrary
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
if not template or template.get_editor_property('template_id')!='industrial.central-building':
    raise RuntimeError('Missing industrial recipe')
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
    if not template.validate_layout(seed):raise RuntimeError('Invalid ladder recipe '+str(seed))
if template.get_editor_property('revision')<4:
    template.set_editor_property('revision',4)
    assets.set_metadata_tag(template,'ERArchetypeRevision','4')
    if not assets.save_loaded_asset(template):raise RuntimeError('Cannot version recipe')
runpy.run_path(str(Path(ue.Paths.project_dir())/'scripts/setup_industrial_pcg_preview.py'))
print('ER_INDUSTRIAL_LADDERS_SETUP_PASS validated_seeds=516 revision='+str(template.get_editor_property('revision')))
