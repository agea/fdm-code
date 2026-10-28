"""Revision6: bake original LOD0/UVs and enable authoritative architecture damage."""
import hashlib,json,zipfile
from pathlib import Path
import unreal as ue

root=Path(ue.Paths.project_dir()).resolve();assets=ue.EditorAssetLibrary
private=root/'Assets/maps/Content/EclipseRecoil'
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_MiddleEasternAlleys')
if not template or assets.get_metadata_tag(template,'ERAlleyDistrictRevision') not in ('5','6','7'):
    raise RuntimeError('Prepare revision5 passages first')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
protected=[private/p for p in ['Maps/L_Depot.umap','Maps/L_CentralBuilding.umap','Maps/L_MiddleEasternAlleys.umap','Art/MapTemplates/DA_CentralBuilding.uasset','Art/Destruction/DA_ER_BlockMaterials.uasset']]
protected+=list((private/'Art/MiddleEasternAlleys').glob('*.uasset'))
before={str(p):sha(p) for p in protected}
package=private/'Art/MapTemplates/DA_MiddleEasternAlleys.uasset'
if assets.get_metadata_tag(template,'ERAlleyDistrictRevision')in ('6','7'):
    print('ER_ALLEY_DESTRUCTION_PRESERVED')
else:
    backup=root/'Saved/AlleyTraversalBackup/revision5.zip';backup.parent.mkdir(parents=True,exist_ok=True)
    if not backup.exists():
        with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
            z.write(package,package.name);z.writestr('hashes.json',json.dumps({package.name:sha(package)}))
    with zipfile.ZipFile(backup) as z:
        for p,h in json.loads(z.read('hashes.json')).items():
            if hashlib.sha256(z.read(p)).hexdigest()!=h:raise RuntimeError('Invalid revision5 rollback')
    town=template.get_editor_property('town');structures=[]
    meshes=list(town.get_editor_property('building_meshes'))+[town.get_editor_property('arch_mesh')]
    tools=ue.AssetToolsHelpers.get_asset_tools();folder='/Game/EclipseRecoil/Art/MiddleEasternAlleys/Structures'
    for mesh in meshes:
        name='DA_'+mesh.get_name().removeprefix('SM_')+'_Structure';path=folder+'/'+name
        if assets.does_asset_exist(path):
            obj=assets.load_asset(path)
            if assets.get_metadata_tag(obj,'ERSourceGeometry')!=mesh.get_path_name():raise RuntimeError('Incompatible existing structure '+path)
        else:
            factory=ue.DataAssetFactory();factory.set_editor_property('data_asset_class',ue.ERDestructibleMesh)
            obj=tools.create_asset(name,folder,ue.ERDestructibleMesh,factory)
            if not obj.bake(mesh):raise RuntimeError('Cannot bake original triangles '+mesh.get_path_name())
            assets.set_metadata_tag(obj,'ERSourceGeometry',mesh.get_path_name())
            assets.set_metadata_tag(obj,'ERAlleyStructureRevision','1')
            if not assets.save_loaded_asset(obj,False):raise RuntimeError('Cannot save '+path)
        structures.append(obj)
    candidate=ue.new_object(ue.ERMapTemplate)
    for key in ['template_id','source_reference','revision','archetype','symmetry','arena_size','town']:
        candidate.set_editor_property(key,template.get_editor_property(key))
    town.set_editor_property('destructible_architecture',True);town.set_editor_property('building_structures',structures)
    candidate.set_editor_property('town',town)
    invalid=[s for s in list(range(512))+[-17,7919,2147483647,-2147483648] if not candidate.validate_layout(s)]
    if invalid:raise RuntimeError('Invalid destructive recipe seeds '+str(invalid))
    template.set_editor_property('town',town);template.set_editor_property('revision',6)
    assets.set_metadata_tag(template,'ERAlleyDistrictRevision','6')
    if not assets.save_loaded_asset(template,False):raise RuntimeError('Cannot save revision6')
    print('ER_ALLEY_DESTRUCTION_SAVED structures='+str(len(structures))+' seeds=516')
for p,h in before.items():
    if sha(Path(p))!=h:raise RuntimeError('Protected original changed '+p)
print('ER_ALLEY_DESTRUCTION_COMPLETE')
