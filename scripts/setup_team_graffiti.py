"""Import user team emblems and source-animated recon case; preserve other art."""
from pathlib import Path
import json,math,hashlib
import unreal as ue
root=Path(ue.Paths.project_dir()); art='/Game/EclipseRecoil/Art/Recon'; folder='/Game/EclipseRecoil/Art/TeamGraffiti'
a=ue.EditorAssetLibrary;tools=ue.AssetToolsHelpers.get_asset_tools();e=ue.MaterialEditingLibrary
source=root/'Assets/maps/SourceAssets';a.make_directory(folder)
def save(obj):
    if not a.save_loaded_asset(obj,False):raise RuntimeError('Save failed '+obj.get_path_name())
def imp(path,dest,name,options=None):
    task=ue.AssetImportTask()
    for k,v in dict(filename=str(path),destination_path=dest,destination_name=name,automated=True,save=True,replace_existing=True).items():task.set_editor_property(k,v)
    if options:task.set_editor_property('options',options)
    tools.import_asset_tasks([task]);asset=a.load_asset(dest+'/'+name)
    if not asset:raise RuntimeError('Missing '+name)
    return asset
materials=[]
for team in ['Alpha','Omega']:
    tex=imp(source/'TeamGraffiti'/(team.lower()+'.png'),folder,'T_Graffiti'+team)
    tex.set_editor_property('srgb',True);tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_DEFAULT);save(tex)
    name='M_Graffiti'+team;m=a.load_asset(folder+'/'+name) if a.does_asset_exist(folder+'/'+name) else tools.create_asset(name,folder,ue.Material,ue.MaterialFactoryNew())
    m.set_editor_property('material_domain',ue.MaterialDomain.MD_DEFERRED_DECAL);m.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT)
    e.delete_all_material_expressions(m)
    node=e.create_material_expression(m,ue.MaterialExpressionTextureSample,-300,0);node.set_editor_property('texture',tex)
    e.connect_material_property(node,'RGB',ue.MaterialProperty.MP_BASE_COLOR);e.connect_material_property(node,'A',ue.MaterialProperty.MP_OPACITY)
    rough=e.create_material_expression(m,ue.MaterialExpressionConstant,-300,200);rough.set_editor_property('r',.9);e.connect_material_property(rough,'',ue.MaterialProperty.MP_ROUGHNESS)
    e.recompile_material(m);save(m);materials.append(m)
case_material=a.load_asset(art+'/M_ReconCase');case_material.set_editor_property('two_sided',True);e.recompile_material(case_material);save(case_material)
case=a.load_asset(art+'/BP_ReconBase');c=ue.get_default_object(case.generated_class())
anim=json.loads((source/'ReconDrones/derived/case-animation.json').read_text())
for key,component in [('Lower','case'),('Upper','lid'),('Lock','lock')]:
    options=ue.FbxImportUI()
    for k,v in dict(automated_import_should_detect_type=False,mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH,import_as_skeletal=False,import_mesh=True,import_materials=False,import_textures=False,import_animations=False).items():options.set_editor_property(k,v)
    data=options.get_editor_property('static_mesh_import_data');data.set_editor_property('combine_meshes',True);data.set_editor_property('auto_generate_collision',key=='Lower')
    name='SM_ReconCase'+key;mesh=imp(source/'ReconDrones/derived'/(name+'.fbx'),art,name,options)
    for i in range(len(mesh.get_editor_property('static_materials'))):mesh.set_material(i,a.load_asset(art+'/M_ReconCase'))
    a.set_metadata_tag(mesh,'Attribution',(source/'ReconDrones/CREDITS.txt').read_text()+'\nAnimated derivative: split original lower shell, lid and lock; fitted closed hinges, original FBX angular keys retained; exploded translation removed.');save(mesh);part=c.get_editor_property(component);part.set_static_mesh(mesh)
    if key!='Lower':part.set_editor_property('relative_location',ue.Vector(*anim[key+'Pivot']))
c.set_editor_property('lid_opening',anim['Upper']);c.set_editor_property('lock_opening',anim['Lock'])
# Lid mesh coordinates share their imported pivot; decal follows the moving lid.
mesh=c.get_editor_property('lid').get_editor_property('static_mesh');bounds=mesh.get_bounds()
c.get_editor_property('emblem').set_editor_property('relative_location',ue.Vector(bounds.origin.x,bounds.origin.y,bounds.origin.z+bounds.box_extent.z+1))
for bp in [case,a.load_asset(art+'/BP_ReconDirector')]:
    default=ue.get_default_object(bp.generated_class());default.set_editor_property('alpha_graffiti',materials[0]);default.set_editor_property('omega_graffiti',materials[1]);ue.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
receipt={'source':'User supplied alpha.png / omega.png','sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (source/'TeamGraffiti').glob('*.png')},'case':'Original FBX angular keys 1–101 at 25fps, fitted closed hinge; source exploded translation removed.'}
(source/'TeamGraffiti/import-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('ER_TEAM_GRAFFITI_ASSETS_PASS '+json.dumps(receipt))
