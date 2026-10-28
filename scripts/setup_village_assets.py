"""Import supplied Village assets and migrate only its owned recipe once.

Run through full-editor -ExecutePythonScript after Blender and the editor build.
"""
import hashlib,json,zipfile
from pathlib import Path
import unreal as ue

root=Path(ue.Paths.project_dir()).resolve();src=root/'Assets/maps/SourceAssets/Village'
derived=src/'imported-derived';private=root/'Assets/maps/Content/EclipseRecoil'
a=ue.EditorAssetLibrary;e=ue.MaterialEditingLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
folder='/Game/EclipseRecoil/Art/Village/Imported'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
protected=list((private/'Art/MapTemplates').glob('*.uasset'))+list((private/'Maps').glob('*.umap'))+[private/'Art/Destruction/DA_ER_BlockMaterials.uasset']
protected=[p for p in protected if p.stem!='DA_Village'];before={str(p):sha(p) for p in protected}
def load(path):
    obj=a.load_asset(path)
    if not obj:raise RuntimeError('Missing '+path)
    return obj
def save(obj):
    if not a.save_loaded_asset(obj,False):raise RuntimeError('Save failed '+obj.get_path_name())
template=load('/Game/EclipseRecoil/Art/MapTemplates/DA_Village')
if template.get_editor_property('template_id')!='village.jungle':raise RuntimeError('Not an owned Village recipe')
package=private/'Art/MapTemplates/DA_Village.uasset'
backup=root/'Saved/VillageBackup/before-imported-assets.zip';backup.parent.mkdir(parents=True,exist_ok=True)
if not backup.exists():
    with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
        z.write(package,package.name);z.writestr('hashes.json',json.dumps({package.name:sha(package)}))
with zipfile.ZipFile(backup) as z:
    for name,digest in json.loads(z.read('hashes.json')).items():
        if hashlib.sha256(z.read(name)).hexdigest()!=digest:raise RuntimeError('Invalid rollback')
refresh_backup=root/'Saved/VillageBackup/before-playable-storage.zip'
if not refresh_backup.exists():
    owned=list((private/'Art/Village/Imported').glob('*.uasset'))+[package]
    with zipfile.ZipFile(refresh_backup,'w',zipfile.ZIP_DEFLATED) as z:
        hashes={}
        for p in owned:
            name=str(p.relative_to(private));z.write(p,name);hashes[name]=sha(p)
        z.writestr('hashes.json',json.dumps(hashes))
with zipfile.ZipFile(refresh_backup) as z:
    for name,digest in json.loads(z.read('hashes.json')).items():
        if hashlib.sha256(z.read(name)).hexdigest()!=digest:raise RuntimeError('Invalid UV rollback')

def material(name,masked=False):
    path=folder+'/M_'+name
    existing=load(path) if a.does_asset_exist(path) else None
    if existing and a.get_metadata_tag(existing,'ERVillageImportRevision')=='3':return existing
    texpath=folder+'/T_'+name
    if not a.does_asset_exist(texpath) or existing:
        task=ue.AssetImportTask();task.filename=str(derived/(name+'.png'));task.destination_path=folder;task.destination_name='T_'+name;task.automated=True;task.factory=ue.TextureFactory()
        task.replace_existing=True
        tools.import_asset_tasks([task]);tex=load(texpath);tex.set_editor_property('max_texture_size',2048);save(tex)
    else:tex=load(texpath)
    if existing:
        a.set_metadata_tag(existing,'ERVillageImportRevision','3');save(existing);return existing
    mat=tools.create_asset('M_'+name,folder,ue.Material,ue.MaterialFactoryNew())
    sample=e.create_material_expression(mat,ue.MaterialExpressionTextureSample,0,0);sample.set_editor_property('texture',tex)
    e.connect_material_property(sample,'RGB',ue.MaterialProperty.MP_BASE_COLOR)
    rough=e.create_material_expression(mat,ue.MaterialExpressionConstant,0,200);rough.set_editor_property('r',.88);e.connect_material_property(rough,'',ue.MaterialProperty.MP_ROUGHNESS)
    mat.set_editor_property('two_sided',True);mat.set_editor_property('used_with_instanced_static_meshes',True)
    if masked:
        mat.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED)
        mat.set_editor_property('opacity_mask_clip_value',.4)
        e.connect_material_property(sample,'A',ue.MaterialProperty.MP_OPACITY_MASK)
    a.set_metadata_tag(mat,'ERVillageImportRevision','3');e.recompile_material(mat);save(mat);return mat

materials={n:material(n,n=='LeavesColor') for n in ['JungleColor','RustyColor','StorageColor','BridgeColor','BarkColor','LeavesColor']}
bindings={'SM_Village_JungleHouse':['JungleColor'],'SM_Village_RustyHouse':['RustyColor'],
          'SM_Village_StorageBody':['StorageColor'],'SM_Village_Bridge':['BridgeColor'],
          'SM_Village_Tree':['BarkColor','LeavesColor']}
meshes={};structures={}
for item in json.loads((derived/'manifest.json').read_text()):
    name=item['name'];path=folder+'/'+name
    mesh=load(path) if a.does_asset_exist(path) else None
    if not mesh or a.get_metadata_tag(mesh,'ERVillageImportRevision')!='3':
        task=ue.AssetImportTask();task.filename=str(derived/(name+'.fbx'));task.destination_path=folder;task.destination_name=name;task.automated=True
        task.replace_existing=True;task.replace_existing_settings=True
        opts=ue.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False
        opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False
        task.options=opts;task.factory=ue.FbxFactory();tools.import_asset_tasks([task]);mesh=load(path)
        for slot,mat in enumerate(bindings[name]):mesh.set_material(slot,materials[mat])
        a.set_metadata_tag(mesh,'ERSourceModel',item['source']);a.set_metadata_tag(mesh,'ERDerivedTriangles',str(item['triangles']));a.set_metadata_tag(mesh,'ERVillageImportRevision','3');save(mesh)
    meshes[name]=mesh
    if name in ['SM_Village_Bridge','SM_Village_StorageBody']:
        n=name.replace('SM_','DA_')+'_Structure';p=folder+'/'+n
        if a.does_asset_exist(p):obj=load(p)
        else:
            factory=ue.DataAssetFactory();factory.set_editor_property('data_asset_class',ue.ERDestructibleMesh)
            obj=tools.create_asset(n,folder,ue.ERDestructibleMesh,factory)
        if a.get_metadata_tag(obj,'ERVillageImportRevision')!='3':
            if not obj.bake(mesh):raise RuntimeError('Cannot bake cell skin '+name)
            a.set_metadata_tag(obj,'ERSourceGeometry',mesh.get_path_name());a.set_metadata_tag(obj,'ERVillageImportRevision','3');save(obj)
        structures[name]=obj

if a.get_metadata_tag(template,'ERVillageImportRevision')!='3':
    recipe=template.get_editor_property('village')
    recipe.set_editor_property('storage_structure',structures['SM_Village_StorageBody'])
    recipe.set_editor_property('bridge_structure',structures['SM_Village_Bridge'])
    recipe.set_editor_property('exterior_houses',[meshes['SM_Village_JungleHouse'],meshes['SM_Village_RustyHouse']])
    recipe.set_editor_property('exterior_trees',[meshes['SM_Village_Tree']])
    candidate=ue.new_object(ue.ERMapTemplate)
    for key in ['template_id','revision','archetype','symmetry','theme_library','source_reference']:
        candidate.set_editor_property(key,template.get_editor_property(key))
    candidate.set_editor_property('village',recipe)
    for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
        if not candidate.validate_layout(seed):raise RuntimeError('Rejected imported Village seed '+str(seed))
    template.set_editor_property('village',recipe);template.set_editor_property('revision',template.get_editor_property('revision')+1)
    a.set_metadata_tag(template,'ERVillageImportRevision','3');save(template)
for name,digest in before.items():
    if sha(Path(name))!=digest:raise RuntimeError('Protected asset changed '+name)
(src/'import-receipt.json').write_text(json.dumps(dict(revision=template.get_editor_property('revision'),protected_sha256=before,
    meshes={n:m.get_path_name() for n,m in meshes.items()},structures={n:m.get_path_name() for n,m in structures.items()},
    texture_max=2048,source_receipt='download-receipt.json',backup=str(backup),seeds=516),indent=2)+'\n')
receipt=json.loads((src/'recipe.json').read_text())
receipt.update(revision=template.get_editor_property('revision'),imported_assets='import-receipt.json',
    source='Original Shoots-inspired gameplay layout with derived Sketchfab bridge/storage skins',
    trees='64 exterior instances of the supplied Ricardo Sanchez 850-triangle tree; two original fallback meshes preserved')
(src/'recipe.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('ER_VILLAGE_IMPORT_COMPLETE seeds=516 protected='+str(len(before)))
