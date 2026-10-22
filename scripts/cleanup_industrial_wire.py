"""Remove the retired industrial wire packages through Unreal, with verified rollback."""
from pathlib import Path
import hashlib,json,zipfile
import unreal as ue
root=Path(ue.Paths.project_dir()).resolve();private=root/'Assets/maps'
a=ue.EditorAssetLibrary;r=ue.AssetRegistryHelpers.get_asset_registry();r.search_all_assets(True)
o=ue.AssetRegistryDependencyOptions(True,True,True,True,True)
base='/Game/EclipseRecoil/Art/'
names=['Industrial/SM_ER_Chainlink','Industrial/M_ER_Chainlink']
names+=['FactoryEnvironment/Meshes/'+n for n in ['SM_Fence_01','SM_Fence05','SM_Fence09','SM_FencePart_01']]
names+=['FactoryEnvironment/Materials/MI_Fence_01']
names+=['FactoryEnvironment/Textures/T_ChainlinkFence_'+n for n in ['BC','N','ORM','MSK']]
candidates={base+n for n in names if a.does_asset_exist(base+n)}
parent=base+'FactoryEnvironment/Materials/M_MetalGrid01'
if a.does_asset_exist(parent) and all(str(x) in candidates or str(x)==parent for x in r.get_referencers(parent,o)):candidates.add(parent)
template_path=base+'MapTemplates/DA_CentralBuilding'
template=a.load_asset(template_path)
recipe=template.get_editor_property('central_building')
if not recipe.get_editor_property('fence_module'):raise RuntimeError('Replacement steel barrier missing')
refs={p:[str(x) for x in r.get_referencers(p,o)] for p in candidates}
for p,items in refs.items():
 outside=set(items)-candidates-{p,template_path}
 if outside:raise RuntimeError('Outside references prevent removal: '+p+' '+str(outside))
def file_for(p):return private/'Content'/(p.removeprefix('/Game/')+'.uasset')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
backup=root/'Saved/AssetCleanupBackups/industrial-wire-20261022.zip';backup.parent.mkdir(parents=True,exist_ok=True)
files=[file_for(p) for p in sorted(candidates)]+[file_for(template_path),private/'SourceAssets/FactoryEnvironment/selection.json']
if not backup.exists():
 original={str(p.relative_to(root)):sha(p) for p in files}
 with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
  for p in files:z.write(p,str(p.relative_to(root)))
  z.writestr('sha256.json',json.dumps(original))
with zipfile.ZipFile(backup) as z:
 for p,h in json.loads(z.read('sha256.json')).items():
  if hashlib.sha256(z.read(p)).hexdigest()!=h:raise RuntimeError('Rollback verification failed '+p)
protected=[private/'Content/EclipseRecoil'/n for n in ['Maps/L_Depot.umap','Maps/L_CentralBuilding.umap','Maps/L_MiddleEasternAlleys.umap','Art/MapTemplates/DA_MiddleEasternAlleys.uasset','Art/Destruction/DA_ER_BlockMaterials.uasset','Art/Industrial/SM_ER_AluminiumFenceModule.uasset']]
before={str(p.relative_to(root)):sha(p) for p in protected}
recipe.set_editor_property('fence_mesh',None);template.set_editor_property('central_building',recipe)
a.set_metadata_tag(template,'ERIndustrialWireRevision','retired');a.set_metadata_tag(template,'ERIndustrialWireCleanup','1')
if not a.save_loaded_asset(template):raise RuntimeError('Cannot clear retired recipe reference')
# Confirm saved references are gone before deleting any asset.
for p in candidates:
 remaining=set(str(x) for x in a.find_package_referencers_for_asset(p,True))-candidates-{p}
 if remaining:raise RuntimeError('Live references prevent removal: '+p+' '+str(remaining))
# Delete leaves first; each remaining dependency must have no outside referencers.
remaining=set(candidates);deleted=[]
while remaining:
 leaves=[p for p in sorted(remaining) if not (set(str(x) for x in a.find_package_referencers_for_asset(p,True))-{p})]
 if not leaves:raise RuntimeError('Unexpected dependency cycle '+str(remaining))
 for p in leaves:
  if not a.delete_asset(p):raise RuntimeError('Cannot delete '+p)
  deleted.append(p);remaining.remove(p)
for p in candidates:
 if a.does_asset_exist(p) or file_for(p).exists():raise RuntimeError('Retired asset remains '+p)
selection=private/'SourceAssets/FactoryEnvironment/selection.json';data=json.loads(selection.read_text())
data['source_meshes']=[p for p in data['source_meshes'] if p.rsplit('/',1)[-1] not in {'SM_Fence_01','SM_Fence05','SM_Fence09','SM_FencePart_01'}]
data['purpose']='Industrial cargo and structural details; retired wire fence assets removed on 22 October 2026'
selection.write_text(json.dumps(data,indent=2)+'\n')
for p,h in before.items():
 if sha(root/p)!=h:raise RuntimeError('Protected asset changed '+p)
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
 if not template.validate_layout(seed):raise RuntimeError('Invalid layout after removal '+str(seed))
report=dict(result='PASS',deleted=deleted,previous_referencers=refs,rollback_archive=str(backup),rollback_sha256=sha(backup),preserved_sha256=before,validated_seeds=516)
(private/'SourceAssets/FactoryEnvironment/industrial-wire-cleanup.json').write_text(json.dumps(report,indent=2)+'\n')
(root/'Saved/Testing/industrial-wire-cleanup.json').write_text(json.dumps(report,indent=2)+'\n')
print('ER_INDUSTRIAL_WIRE_REMOVED '+json.dumps(dict(result='PASS',deleted=deleted,validated_seeds=516)))
