"""Import only zhezhela's CC BY4 Claymore into the private weapons mount."""
import unreal as ue
import zipfile
from datetime import datetime
from pathlib import Path
root=Path(ue.Paths.project_dir())/'Assets/weapons/SourceAssets/ClaymoreM18'
folder='/Game/EclipseRecoil/Weapons/Sketchfab/Claymore'
reimport='-ERReimportClaymore' in ue.SystemLibrary.get_command_line()
if reimport:
    backup=Path(ue.Paths.project_dir())/'Saved/LoadoutBackups'/('claymore-meshes-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
    backup.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
        for name in ('SM_Claymore','SK_Claymore','SK_Claymore_Skeleton','SK_Claymore_PhysicsAsset','M_Claymore'):
            file=Path(ue.Paths.project_dir())/'Content'/folder.removeprefix('/Game/')/(name+'.uasset')
            if file.exists():z.write(file,file.name)
assets=ue.EditorAssetLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
def save(a):
    assert assets.save_loaded_asset(a,only_if_is_dirty=False),a.get_path_name()
def load_file(file,name,options=None):
    result=ue.load_asset(folder+'/'+name)
    if result and not (options and reimport):return result
    task=ue.AssetImportTask()
    for k,v in dict(filename=str(file),destination_path=folder,destination_name=name,automated=True,save=True,replace_existing=bool(options and reimport)).items():task.set_editor_property(k,v)
    if options:task.set_editor_property('options',options)
    tools.import_asset_tasks([task]);result=ue.load_asset(folder+'/'+name);assert result,name
    return result
textures={}
for kind,file in dict(albedo='1001_albedo.jpeg',normal='1001_normal.png',roughness='1001_roughness.jpeg',metallic='1001_metallic.jpeg',ao='1001_AO.jpeg').items():
    tex=load_file(root/'original/textures'/file,'T_Claymore_'+kind)
    tex.set_editor_property('srgb',kind=='albedo')
    if kind=='normal':
        tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP)
        # Sketchfab's normal texture uses OpenGL green; UE expects DirectX.
        tex.set_editor_property('flip_green_channel',True)
    elif kind!='albedo':tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_MASKS)
    tex.set_editor_property('max_texture_size',2048);save(tex);textures[kind]=tex
material=ue.load_asset(folder+'/M_Claymore')
if not material or reimport:
    if material:ue.MaterialEditingLibrary.delete_all_material_expressions(material)
    else:material=tools.create_asset('M_Claymore',folder,ue.Material,ue.MaterialFactoryNew())
    material.set_editor_property('used_with_skeletal_mesh',True)
    for kind,prop in dict(albedo=ue.MaterialProperty.MP_BASE_COLOR,normal=ue.MaterialProperty.MP_NORMAL,roughness=ue.MaterialProperty.MP_ROUGHNESS,metallic=ue.MaterialProperty.MP_METALLIC,ao=ue.MaterialProperty.MP_AMBIENT_OCCLUSION).items():
        sample=ue.MaterialEditingLibrary.create_material_expression(material,ue.MaterialExpressionTextureSample)
        sample.set_editor_property('texture',textures[kind])
        sample.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='normal' else ue.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='albedo' else ue.MaterialSamplerType.SAMPLERTYPE_MASKS)
        ue.MaterialEditingLibrary.connect_material_property(sample,'RGB' if kind in ('albedo','normal') else 'R',prop)
    ue.MaterialEditingLibrary.recompile_material(material);save(material)
for skeletal in [False,True]:
    name='SK_Claymore' if skeletal else 'SM_Claymore'
    opts=ue.FbxImportUI()
    for k,v in dict(import_mesh=True,import_materials=False,import_textures=False,import_as_skeletal=skeletal,import_animations=False,automated_import_should_detect_type=False,mesh_type_to_import=ue.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else ue.FBXImportType.FBXIT_STATIC_MESH).items():opts.set_editor_property(k,v)
    imp=opts.get_editor_property('skeletal_mesh_import_data' if skeletal else 'static_mesh_import_data')
    imp.set_editor_property('import_uniform_scale',1.)
    if not skeletal:
        imp.set_editor_property('combine_meshes',True);imp.set_editor_property('auto_generate_collision',True)
    mesh=load_file(root/'derived'/(name+'.fbx'),name,opts)
    if skeletal:
        slots=list(mesh.get_editor_property('materials'))
        for slot in slots:slot.set_editor_property('material_interface',material)
        mesh.set_editor_property('materials',slots)
    else:
        for i in range(len(mesh.get_editor_property('static_materials'))):mesh.set_material(i,material)
    assets.set_metadata_tag(mesh,'SourceURL','https://sketchfab.com/3d-models/claymore-m18-0e44b2adeb974830804c3ec68c0920f0')
    assets.set_metadata_tag(mesh,'License','CC BY 4.0; zhezhela; normalized units/origin, rigid single-bone derivative')
    save(mesh)
    ue.log('ER_CLAYMORE_IMPORTED '+mesh.get_path_name())
ue.log('ER_CLAYMORE_IMPORT_READY')
