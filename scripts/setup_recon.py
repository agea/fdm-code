"""Import supplied recon art into the private maps mount; set Blueprint defaults.

Run bake_recon_models.py in Blender first. Reuses saved art on reruns; does not
regenerate levels, touch weapon/character art or replace existing materials.
"""
from pathlib import Path
import hashlib,json
import unreal as ue

root=Path(ue.Paths.project_dir());source=root/'Assets/maps/SourceAssets/ReconDrones'
folder='/Game/EclipseRecoil/Art/Recon';assets=ue.EditorAssetLibrary;tools=ue.AssetToolsHelpers.get_asset_tools();edit=ue.MaterialEditingLibrary
manifest=json.loads((source/'derived/manifest.json').read_text())
assets.make_directory(folder)
def save(asset):
    if not assets.save_loaded_asset(asset,False):raise RuntimeError('Cannot save '+asset.get_path_name())
def import_file(path,name,options=None):
    target=folder+'/'+name
    existing=assets.load_asset(target) if assets.does_asset_exist(target) else None
    digest=hashlib.sha256(path.read_bytes()).hexdigest() if options and path.suffix.lower()=='.fbx' else None
    if existing and (not digest or assets.get_metadata_tag(existing,'ERReconSourceSHA')==digest):return existing
    task=ue.AssetImportTask();task.set_editor_property('filename',str(path));task.set_editor_property('destination_path',folder)
    task.set_editor_property('destination_name',name);task.set_editor_property('automated',True);task.set_editor_property('save',True)
    task.set_editor_property('replace_existing',bool(existing))
    if options:task.set_editor_property('options',options)
    tools.import_asset_tasks([task]);asset=assets.load_asset(target)
    if not asset:raise RuntimeError('Import failed '+target+' '+str(task.get_editor_property('imported_object_paths')))
    if digest:assets.set_metadata_tag(asset,'ERReconSourceSHA',digest);save(asset)
    return asset
def scalar(m,value,x,y):
    node=edit.create_material_expression(m,ue.MaterialExpressionConstant,x,y);node.set_editor_property('r',value);return node
def material(name,color=None,case=False):
    existing=assets.load_asset(folder+'/'+name) if assets.does_asset_exist(folder+'/'+name) else None
    if existing:return existing
    m=tools.create_asset(name,folder,ue.Material,ue.MaterialFactoryNew())
    if case:
        for i,(suffix,prop,sampler) in enumerate([
            ('albedo',ue.MaterialProperty.MP_BASE_COLOR,ue.MaterialSamplerType.SAMPLERTYPE_COLOR),
            ('normal',ue.MaterialProperty.MP_NORMAL,ue.MaterialSamplerType.SAMPLERTYPE_NORMAL),
            ('roughness',ue.MaterialProperty.MP_ROUGHNESS,ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR),
            ('metallic',ue.MaterialProperty.MP_METALLIC,ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR),
            ('AO',ue.MaterialProperty.MP_AMBIENT_OCCLUSION,ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)]):
            tex=import_file(source/'pelican-case-game-ready/textures'/('Material__3_'+suffix+'.jpg'),'T_ReconCase_'+suffix)
            tex.set_editor_property('srgb',suffix=='albedo')
            if suffix=='normal':tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP)
            save(tex)
            node=edit.create_material_expression(m,ue.MaterialExpressionTextureSample,0,i*180);node.set_editor_property('texture',tex);node.set_editor_property('sampler_type',sampler)
            edit.connect_material_property(node,'RGB' if suffix in ['albedo','normal'] else 'R',prop)
    else:
        node=edit.create_material_expression(m,ue.MaterialExpressionConstant3Vector,0,0);node.set_editor_property('constant',ue.LinearColor(*color));edit.connect_material_property(node,'',ue.MaterialProperty.MP_BASE_COLOR)
        edit.connect_material_property(scalar(m,.58,0,160),'',ue.MaterialProperty.MP_ROUGHNESS)
    edit.recompile_material(m);save(m);return m
meshes={};measurements={}
for key in ['Drone','Case']:
    options=ue.FbxImportUI()
    for prop,value in dict(automated_import_should_detect_type=False,mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH,import_as_skeletal=False,import_mesh=True,import_materials=False,import_textures=False,import_animations=False).items():options.set_editor_property(prop,value)
    data=options.get_editor_property('static_mesh_import_data');data.set_editor_property('combine_meshes',True);data.set_editor_property('auto_generate_collision',True)
    mesh=import_file(source/'derived'/('SM_Recon'+key+'.fbx'),'SM_Recon'+key,options)
    extent=mesh.get_bounds().box_extent
    width=max(extent.x,extent.y)*2
    expected=85 if key=='Drone' else 70
    if abs(width-expected)>1:raise RuntimeError('Unexpected mesh size '+key+' '+str(width))
    measurements[key]={'width_cm':width,'height_cm':extent.z*2}
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        raw=str(slot.get_editor_property('material_slot_name'))
        if key=='Case':surface=material('M_ReconCase',case=True)
        else:
            names=manifest[key]['diffuse'];match=next((n for n in names if n.replace('.','_')==raw or n==raw),None)
            surface=material('M_ReconDrone_'+str(i),names.get(match,[.1,.1,.1,1]))
        mesh.set_material(i,surface)
    save(mesh);meshes[key]=mesh
debris=[]
for part in manifest.get('Debris',[]):
    name=part['mesh'];mesh=import_file(source/'derived'/(name+'.fbx'),name,options)
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        raw=str(slot.get_editor_property('material_slot_name'))
        index=next((j for j,n in enumerate(manifest['Drone']['materials']) if n==raw or n.replace('.','_')==raw),0)
        mesh.set_material(i,assets.load_asset(folder+'/M_ReconDrone_'+str(index)))
    save(mesh);debris.append(mesh)
def blueprint(name,parent):
    bp=assets.load_asset(folder+'/'+name) if assets.does_asset_exist(folder+'/'+name) else None
    if not bp:
        factory=ue.BlueprintFactory();factory.set_editor_property('parent_class',parent);bp=tools.create_asset(name,folder,ue.Blueprint,factory)
    return bp,ue.get_default_object(bp.generated_class())
drone,d=blueprint('BP_ReconDrone',ue.ERReconDrone);d.get_editor_property('body').set_static_mesh(meshes['Drone']);ue.BlueprintEditorLibrary.compile_blueprint(drone);save(drone)
case,c=blueprint('BP_ReconBase',ue.ERReconBase);c.get_editor_property('case').set_static_mesh(meshes['Case']);ue.BlueprintEditorLibrary.compile_blueprint(case);save(case)
director,r=blueprint('BP_ReconDirector',ue.ERReconDirector)
r.set_editor_property('drone_class',drone.generated_class());r.set_editor_property('base_class',case.generated_class());r.set_editor_property('debris_meshes',debris)
ue.BlueprintEditorLibrary.compile_blueprint(director);save(director)
if len(ue.get_default_object(director.generated_class()).get_editor_property('debris_meshes'))!=5:raise RuntimeError('Missing saved drone debris defaults')
gm=assets.load_asset('/Game/EclipseRecoil/Core/BP_ERGameMode');ue.get_default_object(gm.generated_class()).set_editor_property('recon_class',director.generated_class());ue.BlueprintEditorLibrary.compile_blueprint(gm);save(gm)
credits='''Reconnaissance assets — original attribution supplied in the user's archives:
"S9 drone" by blockv_lb — https://skfb.ly/6SSyD
"Pelican Case Game Ready" by polbrainstorm — https://skfb.ly/6xwPJ
Both archives identify Creative Commons Attribution 4.0:
https://creativecommons.org/licenses/by/4.0/
Changes: evaluated static meshes, centered/rescaled, lid/lock aligned closed;
drone split into a hull and four arm/rotor fragments for cosmetic debris;
drone diffuse materials recreated from source colors; case PBR textures retained.
Original archives, models and credits.txt retained alongside derived/.
'''
(source/'CREDITS.txt').write_text(credits)
for asset in [*meshes.values(),*debris,drone,case,director]:assets.set_metadata_tag(asset,'Attribution',credits);save(asset)
report={'result':'PASS','assets':{k:v.get_path_name() for k,v in meshes.items()},'debris':[m.get_path_name() for m in debris],'measurements':measurements,'archives':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in source.glob('*.zip')},'credits':credits}
(source/'import-receipt.json').write_text(json.dumps(report,indent=2)+'\n')
print('ER_RECON_ASSETS_PASS '+json.dumps(report))
