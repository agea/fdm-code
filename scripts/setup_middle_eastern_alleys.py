"""Unreal-only import and creation of the new Middle Eastern Alleys template.

Preserves completed art/level edits and existing maps/catalogs. Original downloads
and credits live in the private maps mount; no runtime external-file dependency.
"""
import hashlib
import json
from pathlib import Path
import unreal as ue

project=Path(ue.Paths.project_dir()).resolve()
source=project/'Assets/maps/SourceAssets/MiddleEasternAlleys'
folder='/Game/EclipseRecoil/Art/MiddleEasternAlleys'
assets=ue.EditorAssetLibrary;edit=ue.MaterialEditingLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
protected=[project/'Assets/maps/Content/EclipseRecoil'/p for p in [
    'Maps/L_Depot.umap','Maps/L_CentralBuilding.umap','Art/MapTemplates/DA_CentralBuilding.uasset',
    'Art/Destruction/DA_ER_BlockMaterials.uasset']]
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before={str(p):digest(p) for p in protected}
manifest=json.loads((source/'manifest.json').read_text())
recipe=json.loads((source/'recipe.json').read_text())

def load(path):
    obj=assets.load_asset(path)
    if not obj:raise RuntimeError('Missing '+path)
    return obj

def save(obj):
    if not assets.save_loaded_asset(obj,False):raise RuntimeError('Cannot save '+obj.get_path_name())

def texture(path,name,normal=False,linear=False):
    target=folder+'/Textures/'+name
    if assets.does_asset_exist(target):return load(target)
    task=ue.AssetImportTask();task.filename=str(path);task.destination_path=folder+'/Textures';task.destination_name=name
    task.automated=True;task.factory=ue.TextureFactory();tools.import_asset_tasks([task])
    obj=load(target);obj.set_editor_property('srgb',not(normal or linear))
    obj.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP if normal else ue.TextureCompressionSettings.TC_MASKS if linear else ue.TextureCompressionSettings.TC_DEFAULT)
    save(obj);return obj

def node(mat,cls):return edit.create_material_expression(mat,cls,0,0)
def wire(a,pin,b,target):
    if not edit.connect_material_expressions(a,pin,b,target):raise RuntimeError('Material connection '+target)
def output(a,pin,prop):
    if not edit.connect_material_property(a,pin,prop):raise RuntimeError('Material output '+str(prop))
def scalar(mat,value):
    n=node(mat,ue.MaterialExpressionConstant);n.set_editor_property('r',value);return n
def custom(mat,code,inputs,kind):
    n=node(mat,ue.MaterialExpressionCustom);n.set_editor_property('code',code);n.set_editor_property('output_type',kind)
    fields=[]
    for name in inputs:
        field=ue.CustomInput();field.set_editor_property('input_name',name);fields.append(field)
    n.set_editor_property('inputs',fields)
    for name,value in inputs.items():wire(value,'',n,name)
    return n
def material(name):
    m=tools.create_asset(name,folder,ue.Material,ue.MaterialFactoryNew());m.set_editor_property('used_with_instanced_static_meshes',True);return m

ground_path=folder+'/M_ER_ThreeSands'
if assets.does_asset_exist(ground_path):ground=load(ground_path)
else:
    ground=material('M_ER_ThreeSands');ground.set_editor_property('tangent_space_normal',False)
    pos=node(ground,ue.MaterialExpressionWorldPosition)
    uv=custom(ground,'return float2(P.x,-P.y)/250.0;',{'P':pos},ue.CustomMaterialOutputType.CMOT_FLOAT2)
    weights=custom(ground,'float a=0.5+0.5*sin(P.x/1173.0+sin(P.y/821.0)); float b=0.5+0.5*cos(P.y/1031.0+sin(P.x/1571.0)); return float3(0.25+a,0.25+b,0.25+(1-a)*(1-b));',{'P':pos},ue.CustomMaterialOutputType.CMOT_FLOAT3)
    samples={}
    for entry in manifest:
        if entry['key']=='houses':continue
        for role,relative in entry['maps'].items():
            tex=texture(source/relative,'T_ER_'+entry['key']+'_'+role,role=='Normal',role not in {'Normal','Color'})
            for key,value in [('ERSourceURL',entry['source']),('ERAuthor',entry['author']),('ERLicenseURL',entry['license']['url'])]:assets.set_metadata_tag(tex,key,value)
            save(tex)
            s=node(ground,ue.MaterialExpressionTextureSample);s.set_editor_property('texture',tex)
            s.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if role=='Normal' else ue.MaterialSamplerType.SAMPLERTYPE_COLOR if role=='Color' else ue.MaterialSamplerType.SAMPLERTYPE_MASKS)
            wire(uv,'',s,'UVs');samples.setdefault(role,[]).append(s)
    for role,ss in samples.items():
        kind=ue.CustomMaterialOutputType.CMOT_FLOAT3 if role in {'Normal','Color'} else ue.CustomMaterialOutputType.CMOT_FLOAT1
        blend=custom(ground,'return '+('normalize(float3(((A.xy*W.x+B.xy*W.y+C.xy*W.z)/(W.x+W.y+W.z))*float2(0.55,-0.55),1.0));' if role=='Normal' else '(A.rgb*W.x+B.rgb*W.y+C.rgb*W.z)/(W.x+W.y+W.z);' if role=='Color' else '(A.r*W.x+B.r*W.y+C.r*W.z)/(W.x+W.y+W.z);'),{'A':ss[0],'B':ss[1],'C':ss[2],'W':weights},kind)
        output(blend,'',{'Color':ue.MaterialProperty.MP_BASE_COLOR,'Normal':ue.MaterialProperty.MP_NORMAL,'Roughness':ue.MaterialProperty.MP_ROUGHNESS,'AO':ue.MaterialProperty.MP_AMBIENT_OCCLUSION}[role])
    edit.recompile_material(ground);assets.set_metadata_tag(ground,'ERThreeSandSources',','.join(e['uid'] for e in manifest if e['key']!='houses'));save(ground)

house_materials={}
house_entry=next(e for e in manifest if e['key']=='houses')
for name,image in [('Concrete','City'),('Desert','Desert')]:
    target=folder+'/M_ER_House'+name
    if assets.does_asset_exist(target):m=load(target)
    else:
        m=material('M_ER_House'+name)
        tex=texture(source/house_entry['uid']/'original/textures'/(image+'.png'),'T_ER_House'+image)
        s=node(m,ue.MaterialExpressionTextureSample);s.set_editor_property('texture',tex)
        output(s,'RGB',ue.MaterialProperty.MP_BASE_COLOR);output(scalar(m,.92),'',ue.MaterialProperty.MP_ROUGHNESS)
        edit.recompile_material(m)
        for key,value in [('ERSourceURL',house_entry['source']),('ERAuthor',house_entry['author']),('ERLicenseURL',house_entry['license']['url'])]:assets.set_metadata_tag(m,key,value)
        save(m)
    house_materials[name]=m

def mesh(path,name):
    target=folder+'/Meshes/'+name
    if assets.does_asset_exist(target):return load(target)
    task=ue.AssetImportTask();task.filename=str(path);task.destination_path=folder+'/Meshes';task.destination_name=name
    task.automated=True;task.save=False
    opts=ue.FbxImportUI();opts.set_editor_property('import_as_skeletal',False);opts.set_editor_property('import_materials',False);opts.set_editor_property('import_textures',False)
    opts.set_editor_property('mesh_type_to_import',ue.FBXImportType.FBXIT_STATIC_MESH)
    opts.static_mesh_import_data.set_editor_property('combine_meshes',True)
    opts.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False)
    opts.static_mesh_import_data.set_editor_property('normal_import_method',ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    if path.suffix=='.obj':
        opts.static_mesh_import_data.set_editor_property('convert_scene',False);opts.static_mesh_import_data.set_editor_property('convert_scene_unit',False)
    task.options=opts;tools.import_asset_tasks([task]);obj=load(target)
    assets.set_metadata_tag(obj,'ERSourceSHA256',digest(path));save(obj);return obj

houses=[]
for entry in json.loads((source/'prepared_houses/manifest.json').read_text()):
    obj=mesh(source/'prepared_houses'/(entry['name']+'.fbx'),entry['name'])
    if not assets.get_metadata_tag(obj,'ERHouseMaterial'):
        for slot in range(len(obj.get_editor_property('static_materials'))):obj.set_material(slot,house_materials[entry['material']])
        assets.set_metadata_tag(obj,'ERHouseMaterial','1');save(obj)
    houses.append(obj)
arches=[mesh(source/(name+'.obj'),name) for name in ['SM_ER_DustArch','SM_ER_DustDoorLeft','SM_ER_DustDoorRight']]

def tinted(name,tint):
    target=folder+'/'+name
    if assets.does_asset_exist(target):return load(target)
    m=assets.duplicate_asset('/Game/EclipseRecoil/Art/Industrial/SketchfabTextures/M_ER_SF_concrete_seamless',target)
    previous=edit.get_material_property_input_node(m,ue.MaterialProperty.MP_BASE_COLOR)
    color=node(m,ue.MaterialExpressionConstant3Vector);color.set_editor_property('constant',ue.LinearColor(*tint,1))
    multiply=node(m,ue.MaterialExpressionMultiply);wire(previous,'RGB',multiply,'A');wire(color,'',multiply,'B')
    output(multiply,'',ue.MaterialProperty.MP_BASE_COLOR);edit.recompile_material(m);save(m);return m
wall=tinted('M_ER_AlleyPlaster',(1.13,.85,.58));roof=tinted('M_ER_AlleyRoof',(.83,.68,.47))
wood=load('/Game/EclipseRecoil/Art/Industrial/SketchfabTextures/M_ER_SF_wood_05')
iron=load('/Game/EclipseRecoil/Art/Industrial/SketchfabTextures/M_ER_SF_steel_irregular')

template_path='/Game/EclipseRecoil/Art/MapTemplates/DA_MiddleEasternAlleys'
if assets.does_asset_exist(template_path):
    template=load(template_path)
    if assets.get_metadata_tag(template,'ERAlleyRecipeSHA256')!=digest(source/'recipe.json'):
        raise RuntimeError('Completed alley recipe changed; use an explicit versioned migration')
else:
    candidate=ue.new_object(ue.ERMapTemplate)
    for key,value in [('template_id',recipe['id']),('revision',1),('source_reference',recipe['reference']),
                      ('archetype',ue.ERMapArchetype.MIDDLE_EASTERN_ALLEYS),('symmetry',ue.ERTemplateSymmetry.NONE),('arena_size',ue.Vector(7500,8400,1200))]:candidate.set_editor_property(key,value)
    town=ue.ERDustTownVariation();town.set_editor_property('enabled',True)
    for key in ['arena_min','arena_max','building_height','door_gap','crossing']:town.set_editor_property(key,ue.Vector2D(*recipe[key]))
    town.set_editor_property('axis_warp',recipe['axis_warp']);town.set_editor_property('covered_tunnels',True)
    streets=[]
    for entry in recipe['streets']:
        s=ue.ERPlanStreet();s.set_editor_property('name',entry['name']);s.set_editor_property('points',[ue.Vector2D(*p) for p in entry['points']]);s.set_editor_property('width',ue.Vector2D(*entry['width']));s.set_editor_property('elevated',entry['elevated']);streets.append(s)
    town.set_editor_property('streets',streets)
    courts=[]
    for entry in recipe['courts']:
        c=ue.ERPlanCourt();c.set_editor_property('name',entry['name'])
        for key in ['center','size']:c.set_editor_property(key,ue.Vector2D(*entry[key]))
        courts.append(c)
    town.set_editor_property('courts',courts)
    gates=[]
    for entry in recipe['gates']:
        g=ue.ERPlanGate();g.set_editor_property('street',entry['street']);g.set_editor_property('position',ue.Vector2D(*entry['position']));gates.append(g)
    town.set_editor_property('gates',gates);town.set_editor_property('patrol',[ue.Vector2D(*p) for p in recipe['patrol']])
    for key,value in [('wall_surface',wall),('roof_surface',roof),('road_surface',roof),('wood_surface',wood),('iron_surface',iron),('ground_surface',ground),('arch_mesh',arches[0]),('left_door_mesh',arches[1]),('right_door_mesh',arches[2]),('building_meshes',houses)]:town.set_editor_property(key,value)
    candidate.set_editor_property('town',town)
    for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
        if not candidate.validate_layout(seed):raise RuntimeError('Invalid alley seed '+str(seed))
    factory=ue.DataAssetFactory();factory.set_editor_property('data_asset_class',ue.ERMapTemplate)
    template=tools.create_asset('DA_MiddleEasternAlleys','/Game/EclipseRecoil/Art/MapTemplates',ue.ERMapTemplate,factory)
    for key in ['template_id','revision','source_reference','archetype','symmetry','arena_size','town']:template.set_editor_property(key,candidate.get_editor_property(key))
    assets.set_metadata_tag(template,'ERAlleyRecipeSHA256',digest(source/'recipe.json'));save(template)
    print('ER_ALLEYS_TEMPLATE_SAVED seeds=516')

map_path='/Game/EclipseRecoil/Maps/L_MiddleEasternAlleys'
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
if not assets.does_asset_exist(map_path):
    if not levels.new_level(map_path):raise RuntimeError('Cannot create alley level')
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERGameMode'))
    world.get_world_settings().set_editor_property('kill_z',-800)
    def spawn(cls,label,pos):
        a=actors.spawn_actor_from_class(cls,ue.Vector(*pos),ue.Rotator(pitch=0,yaw=0,roll=0));a.set_actor_label(label);return a
    generator=spawn(ue.ERProceduralMap,'MiddleEasternAlleys',(0,0,0))
    config=generator.get_editor_property('layout_config');config.set_editor_property('template_asset',template);generator.set_editor_property('layout_config',config)
    generator.set_editor_property('ceilings',False);generator.set_editor_property('floor_surface',ground)
    generator.set_editor_property('material_catalog',load('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials'))
    generator.set_editor_property('explosion_effect_class',assets.load_blueprint_class('/Game/EclipseRecoil/Effects/Grenade/BP_ERGrenadeExplosion'))
    for team in ['ALPHA','OMEGA']:
        for i in range(10):spawn(ue.ERTeamPlayerStart,team+'_Start_'+str(i),(0,0,100)).set_editor_property('team',getattr(ue.ERTeam,team))
    sun=spawn(ue.DirectionalLight,'Alley_DesertSun',(0,0,2500));sun.set_actor_rotation(ue.Rotator(pitch=-55,yaw=-30,roll=0),False)
    sun.light_component.set_mobility(ue.ComponentMobility.MOVABLE);sun.light_component.set_editor_property('intensity',3.5);sun.light_component.set_editor_property('atmosphere_sun_light',True)
    sky=spawn(ue.SkyLight,'Alley_Sky',(0,0,2200));sky.light_component.set_mobility(ue.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property('real_time_capture',True);sky.light_component.set_editor_property('lower_hemisphere_is_black',False);sky.light_component.set_editor_property('intensity',1.5)
    spawn(ue.SkyAtmosphere,'Alley_Atmosphere',(0,0,0))
    if not levels.save_current_level():raise RuntimeError('Cannot save alley level')
credits=project/'Branding/Credits.txt';text=credits.read_text();heading='MIDDLE EASTERN ALLEYS — SKETCHFAB'
if heading not in text:
    text=text.replace('THANK YOU\n',heading+'\n'+(source/'CREDITS.txt').read_text()+'\nTHANK YOU\n');credits.write_text(text)
for path,sha in before.items():
    if digest(Path(path))!=sha:raise RuntimeError('Protected asset changed '+path)
(source/'import-receipt.json').write_text(json.dumps(dict(template=template_path,map=map_path,houses=[h.get_path_name() for h in houses],ground=ground.get_path_name(),preserved_sha256=before),indent=2)+'\n')
print('ER_ALLEYS_SETUP_COMPLETE')
