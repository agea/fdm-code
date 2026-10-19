"""Version the denser industrial cover recipe; preserve art, Depot and physical profiles."""
from pathlib import Path
import hashlib,json,runpy
import unreal as ue
root=Path(ue.Paths.project_dir());assets=ue.EditorAssetLibrary
protected=['Assets/maps/Content/EclipseRecoil/Maps/L_Depot.umap','Assets/maps/Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset']
before={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in protected}
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
if not template or not template.get_editor_property('central_building').get_editor_property('steel_surface'):raise RuntimeError('Missing industrial recipe/palette')
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
 if not template.validate_layout(seed):raise RuntimeError('Invalid cover layout '+str(seed))
revision=max(6,template.get_editor_property('revision'));template.set_editor_property('revision',revision);assets.set_metadata_tag(template,'ERArchetypeRevision',str(revision))
if not assets.save_loaded_asset(template):raise RuntimeError('Cannot save cover recipe')
runpy.run_path(str(root/'scripts/setup_industrial_pcg_preview.py'))
for p,sha in before.items():
 if hashlib.sha256((root/p).read_bytes()).hexdigest()!=sha:raise RuntimeError('Preserved asset changed '+p)
report=dict(result='PASS',revision=revision,validated_seeds=516,preserved_sha256=before)
(root/'Saved/Testing/intermediate-cover-assets-report.json').write_text(json.dumps(report,indent=2)+'\n')
print('ER_INTERMEDIATE_COVER_SETUP_PASS '+json.dumps(report))
