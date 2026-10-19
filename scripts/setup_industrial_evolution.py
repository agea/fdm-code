"""Version the industrial recipe and refresh its PCG authoring snapshot."""
from pathlib import Path
import runpy
import unreal as ue

assets=ue.EditorAssetLibrary
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
if not template or template.get_editor_property('template_id')!='industrial.central-building':
    raise RuntimeError('Missing industrial recipe')
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
    if not template.validate_layout(seed):raise RuntimeError('Invalid updated recipe '+str(seed))
if template.get_editor_property('revision')<2:
    template.set_editor_property('revision',2)
    assets.set_metadata_tag(template,'ERArchetypeRevision','2')
    if not assets.save_loaded_asset(template):raise RuntimeError('Cannot version recipe')
runpy.run_path(str(Path(ue.Paths.project_dir())/'scripts/setup_industrial_pcg_preview.py'))
print('ER_INDUSTRIAL_EVOLUTION_SETUP_PASS validated_seeds=516 revision='+str(template.get_editor_property('revision')))
