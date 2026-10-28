"""Migrate Village to the supplied Shoots plan, palisade and panorama once.

Run using UnrealEditor -ExecutePythonScript after the editor build and
Blender bake_village_backdrop.py. Preserve other maps/recipes and source art.
"""
from pathlib import Path
import hashlib,json,zipfile
import unreal as ue

root=Path(ue.Paths.project_dir()).resolve();private=root/'Assets/maps/Content/EclipseRecoil'
src=root/'Assets/maps/SourceAssets/Village';derived=src/'topology-derived'
a=ue.EditorAssetLibrary;e=ue.MaterialEditingLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
folder='/Game/EclipseRecoil/Art/Village/Topology'
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
if template.get_editor_property('template_id')!='village.jungle':raise RuntimeError('Unowned recipe')
backup=root/'Saved/VillageBackup/before-topology.zip';backup.parent.mkdir(parents=True,exist_ok=True)
if not backup.exists():
    package=private/'Art/MapTemplates/DA_Village.uasset'
    with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
        z.write(package,package.name);z.writestr('hashes.json',json.dumps({package.name:sha(package)}))
with zipfile.ZipFile(backup) as z:
    for name,digest in json.loads(z.read('hashes.json')).items():
        if hashlib.sha256(z.read(name)).hexdigest()!=digest:raise RuntimeError('Invalid rollback')
ground=load('/Game/EclipseRecoil/Art/Village/M_ER_Jungle_Ground')
if a.get_metadata_tag(ground,'ERVillageGroundExtentRevision')!='1':
    file=private/'Art/Village/M_ER_Jungle_Ground.uasset';rollback=root/'Saved/VillageBackup/before-ground-extent.zip'
    if not rollback.exists():
        digest=sha(file)
        with zipfile.ZipFile(rollback,'w',zipfile.ZIP_DEFLATED) as z:z.write(file,file.name);z.writestr('sha256.txt',digest)
    with zipfile.ZipFile(rollback) as z:
        if hashlib.sha256(z.read(file.name)).hexdigest()!=z.read('sha256.txt').decode():raise RuntimeError('Invalid ground rollback')
    blend=e.get_material_property_input_node(ground,ue.MaterialProperty.MP_BASE_COLOR)
    masks=[n for n in e.get_inputs_for_material_expression(ground,blend) if isinstance(n,ue.MaterialExpressionCustom) and 'float center=' in n.get_editor_property('code')]
    if len(masks)!=1:raise RuntimeError('Unexpected owned ground graph')
    masks[0].set_editor_property('code','float center=1-smoothstep(140.0,360.0,abs(P.y)); float flank=1-smoothstep(180.0,370.0,abs(abs(P.x)-1530.0)); float n=0.15*sin(P.x/71.0)*sin(P.y/83.0); float extent=(1-smoothstep(2300.0,2800.0,abs(P.x)))*(1-smoothstep(2600.0,3100.0,abs(P.y))); return saturate(max(center,flank)*0.8+n)*extent;')
    a.set_metadata_tag(ground,'ERVillageGroundExtentRevision','1');e.recompile_material(ground);save(ground)

path=folder+'/M_Village_GraySheet'
if a.does_asset_exist(path):gray=load(path)
else:
    gray=tools.create_asset('M_Village_GraySheet',folder,ue.Material,ue.MaterialFactoryNew())
    color=e.create_material_expression(gray,ue.MaterialExpressionConstant3Vector,0,0);color.set_editor_property('constant',ue.LinearColor(.30,.33,.34,1));e.connect_material_property(color,'',ue.MaterialProperty.MP_BASE_COLOR)
    rough=e.create_material_expression(gray,ue.MaterialExpressionConstant,0,200);rough.set_editor_property('r',.9);e.connect_material_property(rough,'',ue.MaterialProperty.MP_ROUGHNESS)
    gray.set_editor_property('two_sided',True);gray.set_editor_property('used_with_instanced_static_meshes',True);e.recompile_material(gray);save(gray)
if a.get_metadata_tag(gray,'ERVillageGraySheetRevision')!='1':
    file=private/'Art/Village/Topology/M_Village_GraySheet.uasset'
    rollback=root/'Saved/VillageBackup/before-gray-sheet-detail.zip'
    if file.exists() and not rollback.exists():
        with zipfile.ZipFile(rollback,'w',zipfile.ZIP_DEFLATED) as z:z.write(file,file.name)
    pos=e.create_material_expression(gray,ue.MaterialExpressionWorldPosition,0,400)
    normal=e.create_material_expression(gray,ue.MaterialExpressionVertexNormalWS,0,600)
    def custom(code,kind,names):
        node=e.create_material_expression(gray,ue.MaterialExpressionCustom,300,400)
        fields=[]
        for name in names:
            item=ue.CustomInput();item.set_editor_property('input_name',name);fields.append(item)
        node.set_editor_property('inputs',fields);node.set_editor_property('code',code);node.set_editor_property('output_type',kind);return node
    color=custom('float age=0.03*sin(P.x/21.0+sin(P.y/35.0))*sin((P.z+P.y)/15.0); float seam=step(0.98,frac(P.x/110.0)); return float3(0.22,0.25,0.26)*(1.0+age)*lerp(1.0,0.6,seam);',ue.CustomMaterialOutputType.CMOT_FLOAT3,['P'])
    e.connect_material_expressions(pos,'',color,'P');e.connect_material_property(color,'',ue.MaterialProperty.MP_BASE_COLOR)
    ripple=custom('float3 a=abs(N); float3 U=a.x>a.y&&a.x>a.z?float3(0,1,0):float3(1,0,0); return normalize(N+U*(0.18*cos(dot(P,U)*1.0472)));',ue.CustomMaterialOutputType.CMOT_FLOAT3,['P','N'])
    e.connect_material_expressions(pos,'',ripple,'P');e.connect_material_expressions(normal,'',ripple,'N');e.connect_material_property(ripple,'',ue.MaterialProperty.MP_NORMAL)
    gray.set_editor_property('tangent_space_normal',False);a.set_metadata_tag(gray,'ERVillageGraySheetRevision','1');e.recompile_material(gray);save(gray)
path=folder+'/M_Village_GrayPark'
if a.does_asset_exist(path):sky_material=load(path)
else:
    task=ue.AssetImportTask();task.filename=str(derived/'T_Village_GrayPark.png');task.destination_path=folder;task.destination_name='T_Village_GrayPark';task.automated=True;task.factory=ue.TextureFactory();tools.import_asset_tasks([task])
    texture=load(folder+'/T_Village_GrayPark');texture.set_editor_property('max_texture_size',4096);save(texture)
    sky_material=tools.create_asset('M_Village_GrayPark',folder,ue.Material,ue.MaterialFactoryNew())
    sample=e.create_material_expression(sky_material,ue.MaterialExpressionTextureSample,0,0);sample.set_editor_property('texture',texture);e.connect_material_property(sample,'RGB',ue.MaterialProperty.MP_EMISSIVE_COLOR)
    sky_material.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT);sky_material.set_editor_property('two_sided',True);sky_material.set_editor_property('is_sky',True);sky_material.set_editor_property('used_with_instanced_static_meshes',True)
    e.recompile_material(sky_material);save(sky_material)
meshes={}
for name,material in [('SM_Village_PalisadeLog',load('/Game/EclipseRecoil/Art/Village/M_ER_Jungle_Timber')),('SM_Village_SkyDome',sky_material)]:
    path=folder+'/'+name
    if a.does_asset_exist(path):mesh=load(path)
    else:
        task=ue.AssetImportTask();task.filename=str(derived/(name+'.fbx'));task.destination_path=folder;task.destination_name=name;task.automated=True
        opts=ue.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False;opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False
        task.options=opts;task.factory=ue.FbxFactory();tools.import_asset_tasks([task]);mesh=load(path);mesh.set_material(0,material);a.set_metadata_tag(mesh,'ERVillageTopologyOwned','1');save(mesh)
    meshes[name]=mesh
if a.get_metadata_tag(template,'ERVillageTopologyRevision')!='1':
    recipe=template.get_editor_property('village');recipe.set_editor_property('exterior_houses',[])
    recipe.set_editor_property('palisade_pole_mesh',meshes['SM_Village_PalisadeLog']);recipe.set_editor_property('sky_dome_mesh',meshes['SM_Village_SkyDome']);recipe.set_editor_property('cover_surface',gray)
    candidate=ue.new_object(ue.ERMapTemplate)
    for key in ['template_id','revision','archetype','symmetry','theme_library','source_reference']:candidate.set_editor_property(key,template.get_editor_property(key))
    candidate.set_editor_property('village',recipe)
    for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
        if not candidate.validate_layout(seed):raise RuntimeError('Invalid topology seed '+str(seed))
    template.set_editor_property('village',recipe);template.set_editor_property('revision',template.get_editor_property('revision')+1);a.set_metadata_tag(template,'ERVillageTopologyRevision','1');save(template)
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
    if not template.validate_layout(seed):raise RuntimeError('Invalid saved topology seed '+str(seed))
for name,digest in before.items():
    if sha(Path(name))!=digest:raise RuntimeError('Protected asset changed '+name)
(src/'topology-receipt.json').write_text(json.dumps(dict(revision=template.get_editor_property('revision'),marker=1,seeds=516,protected_sha256=before,backup=str(backup),background='Greg Zaal / Poly Haven J&E Gray Park CC0, original project sky sphere',reference='Owner-supplied top-down and game screenshot; four climbable spawn structures, side shacks, gray central cover, log palisade'),indent=2)+'\n')
receipt=json.loads((src/'recipe.json').read_text());receipt.update(revision=template.get_editor_property('revision'),topology='topology-receipt.json',source='Owner supplied revised Shoots topology: staggered central huts, raised rear galleries and corner lookouts, paired side shacks, gray cover, log palisade',exterior_houses=False,background='Poly Haven J&E Gray Park CC0, see panorama source.json')
(src/'recipe.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('ER_VILLAGE_TOPOLOGY_COMPLETE seeds=516 protected='+str(len(before)))
