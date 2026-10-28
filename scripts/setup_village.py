"""Create Village and append Jungle without changing existing recipes or art.
Run through UnrealEditor -run=pythonscript -script=/absolute/path/setup_village.py.
"""
from pathlib import Path
import hashlib
import json
import zipfile
import unreal as ue

root=Path(ue.Paths.project_dir()).resolve()
a=ue.EditorAssetLibrary;e=ue.MaterialEditingLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
folder='/Game/EclipseRecoil/Art/Village'
base='/Game/EclipseRecoil/Art/MapTemplates'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
protected=list((root/'Assets/maps/Content/EclipseRecoil/Art/MapTemplates').glob('DA_*.uasset'))
protected=[p for p in protected if p.stem not in {'DA_MapThemes','DA_Village'}]
protected+=list((root/'Assets/maps/Content/EclipseRecoil/Maps').glob('*.umap'))
protected=[p for p in protected if p.stem!='L_Village']
protected+=[root/'Assets/maps/Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset']
before={str(p):sha(p) for p in protected}
def load(path):
    obj=a.load_asset(path)
    if not obj:raise RuntimeError('Missing '+path)
    return obj
def save(obj):
    if not a.save_loaded_asset(obj,False):raise RuntimeError('Cannot save '+obj.get_path_name())
def output(node,pin,prop):
    if not e.connect_material_property(node,pin,prop):raise RuntimeError('Material output failed')
def wire(node,pin,target,name):
    if not e.connect_material_expressions(node,pin,target,name):raise RuntimeError('Material connection failed')
def tint_material(name,source,tint,corrugated=False):
    path=folder+'/'+name
    if a.does_asset_exist(path):return load(path)
    original=load(source)
    mat=a.duplicate_asset(source,path)
    if not mat:raise RuntimeError('Cannot duplicate material')
    color=e.get_material_property_input_node(mat,ue.MaterialProperty.MP_BASE_COLOR)
    pin=e.get_material_property_input_node_output_name(mat,ue.MaterialProperty.MP_BASE_COLOR)
    tint_node=e.create_material_expression(mat,ue.MaterialExpressionConstant3Vector,400,0)
    tint_node.set_editor_property('constant',ue.LinearColor(*tint,1))
    multiply=e.create_material_expression(mat,ue.MaterialExpressionMultiply,600,0)
    wire(color,pin,multiply,'A');wire(tint_node,'',multiply,'B');output(multiply,'',ue.MaterialProperty.MP_BASE_COLOR)
    rough=e.create_material_expression(mat,ue.MaterialExpressionConstant,600,300)
    rough.set_editor_property('r',.88);output(rough,'',ue.MaterialProperty.MP_ROUGHNESS)
    if corrugated:
        # World-space normal ripple follows the sheet's dominant face. The
        # broad rusty texture remains original; no runtime displacement.
        pos=e.create_material_expression(mat,ue.MaterialExpressionWorldPosition,200,500)
        normal=e.create_material_expression(mat,ue.MaterialExpressionVertexNormalWS,200,650)
        custom=e.create_material_expression(mat,ue.MaterialExpressionCustom,600,500)
        custom.set_editor_property('code','float3 a=abs(N); float3 U=a.x>a.y&&a.x>a.z?float3(0,1,0):float3(1,0,0); return normalize(N+U*(0.22*cos(dot(P,U)*0.785398)));')
        custom.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT3)
        fields=[]
        for key in ['P','N']:
            field=ue.CustomInput();field.set_editor_property('input_name',key);fields.append(field)
        custom.set_editor_property('inputs',fields);wire(pos,'',custom,'P');wire(normal,'',custom,'N');output(custom,'',ue.MaterialProperty.MP_NORMAL)
        mat.set_editor_property('tangent_space_normal',False);mat.set_editor_property('two_sided',True)
    mat.set_editor_property('used_with_instanced_static_meshes',True)
    a.set_metadata_tag(mat,'ERVillageOwned','1');a.set_metadata_tag(mat,'ERSourceMaterial',source)
    e.recompile_material(mat);save(mat);return mat
wood=tint_material('M_ER_Jungle_Timber','/Game/EclipseRecoil/Art/Industrial/SketchfabTextures/M_ER_SF_wood_05',(.85,.75,.57))
metal=tint_material('M_ER_Jungle_Corrugated','/Game/EclipseRecoil/Art/Industrial/SketchfabTextures/M_ER_SF_steel_irregular',(.9,.68,.45),True)
if a.get_metadata_tag(metal,'ERVillageRustRevision')!='1':
    color=e.get_material_property_input_node(metal,ue.MaterialProperty.MP_BASE_COLOR)
    pin=e.get_material_property_input_node_output_name(metal,ue.MaterialProperty.MP_BASE_COLOR)
    pos=e.create_material_expression(metal,ue.MaterialExpressionWorldPosition,500,1100)
    mask=e.create_material_expression(metal,ue.MaterialExpressionCustom,700,1100)
    mask.set_editor_property('code','return 0.75*smoothstep(-0.1,0.7,sin(P.x/43.0+sin(P.y/71.0))*sin((P.y+P.z)/53.0)+0.18*sin(P.x/6.0));')
    mask.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT1)
    field=ue.CustomInput();field.set_editor_property('input_name','P');mask.set_editor_property('inputs',[field]);wire(pos,'',mask,'P')
    rust=e.create_material_expression(metal,ue.MaterialExpressionConstant3Vector,700,1300);rust.set_editor_property('constant',ue.LinearColor(.18,.065,.018,1))
    blend=e.create_material_expression(metal,ue.MaterialExpressionLinearInterpolate,1000,1100);wire(color,pin,blend,'A');wire(rust,'',blend,'B');wire(mask,'',blend,'Alpha');output(blend,'',ue.MaterialProperty.MP_BASE_COLOR)
    a.set_metadata_tag(metal,'ERVillageRustRevision','1');e.recompile_material(metal);save(metal)
ground=tint_material('M_ER_Jungle_Ground','/Game/EclipseRecoil/Art/Industrial/SketchfabTextures/M_ER_SF_grass_02',(.72,.7,.5))
if a.get_metadata_tag(ground,'ERVillagePathsRevision')!='1':
    color=e.get_material_property_input_node(ground,ue.MaterialProperty.MP_BASE_COLOR)
    pin=e.get_material_property_input_node_output_name(ground,ue.MaterialProperty.MP_BASE_COLOR)
    pos=e.create_material_expression(ground,ue.MaterialExpressionWorldPosition,500,700)
    mask=e.create_material_expression(ground,ue.MaterialExpressionCustom,700,700)
    mask.set_editor_property('code','float center=1-smoothstep(140.0,360.0,abs(P.y)); float flank=1-smoothstep(180.0,370.0,abs(abs(P.x)-1530.0)); float n=0.15*sin(P.x/71.0)*sin(P.y/83.0); return saturate(max(center,flank)*0.8+n);')
    mask.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT1)
    field=ue.CustomInput();field.set_editor_property('input_name','P');mask.set_editor_property('inputs',[field]);wire(pos,'',mask,'P')
    dirt=e.create_material_expression(ground,ue.MaterialExpressionConstant3Vector,700,900);dirt.set_editor_property('constant',ue.LinearColor(.16,.105,.055,1))
    blend=e.create_material_expression(ground,ue.MaterialExpressionLinearInterpolate,1000,700);wire(color,pin,blend,'A');wire(dirt,'',blend,'B');wire(mask,'',blend,'Alpha');output(blend,'',ue.MaterialProperty.MP_BASE_COLOR)
    a.set_metadata_tag(ground,'ERVillagePathsRevision','1');e.recompile_material(ground);save(ground)


def flat(name,color):
    path=folder+'/'+name
    if a.does_asset_exist(path):return load(path)
    mat=tools.create_asset(name,folder,ue.Material,ue.MaterialFactoryNew())
    c=e.create_material_expression(mat,ue.MaterialExpressionConstant3Vector,0,0);c.set_editor_property('constant',ue.LinearColor(*color,1));output(c,'',ue.MaterialProperty.MP_BASE_COLOR)
    r=e.create_material_expression(mat,ue.MaterialExpressionConstant,0,200);r.set_editor_property('r',.95);output(r,'',ue.MaterialProperty.MP_ROUGHNESS)
    mat.set_editor_property('used_with_instanced_static_meshes',True);e.recompile_material(mat);save(mat);return mat
trunk=flat('M_ER_Jungle_Trunk',(.16,.085,.035));leaves=flat('M_ER_Jungle_Leaves',(.07,.19,.065))
trees=[]
for i in range(2):
    name='SM_ER_VillageTree_'+str(i);path=folder+'/Meshes/'+name
    if a.does_asset_exist(path):mesh=load(path)
    else:
        task=ue.AssetImportTask();task.filename=str(root/'Assets/maps/SourceAssets/Village/derived'/(name+'.fbx'));task.destination_path=folder+'/Meshes';task.destination_name=name;task.automated=True;task.save=False
        options=ue.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False;options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.auto_generate_collision=False
        task.options=options;task.factory=ue.FbxFactory();tools.import_asset_tasks([task]);mesh=load(path)
        for slot in range(len(mesh.get_editor_property('static_materials'))):mesh.set_material(slot,trunk if slot==0 else leaves)
        a.set_metadata_tag(mesh,'ERSource','Original project-generated backdrop tree, bake_village_trees.py; no third-party geometry');save(mesh)
    trees.append(mesh)

library=load(base+'/DA_MapThemes')
if not any(str(p.get_editor_property('name'))=='Jungle' for p in library.get_editor_property('themes')):
    file=root/'Assets/maps/Content/EclipseRecoil/Art/MapTemplates/DA_MapThemes.uasset'
    backup=root/'Saved/VillageBackup/before-jungle.zip';backup.parent.mkdir(parents=True,exist_ok=True)
    if not backup.exists():
        with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:z.write(file,str(file.relative_to(root)))
        with zipfile.ZipFile(backup) as z:
            if hashlib.sha256(z.read(str(file.relative_to(root)))).hexdigest()!=sha(file):raise RuntimeError('Library rollback mismatch')
    palettes=list(library.get_editor_property('themes'))
    theme=ue.ERMapThemePalette();theme.set_editor_property('name','Jungle');theme.set_editor_property('materials',[wood,wood,wood,wood,wood,metal,wood]);theme.set_editor_property('wall',wood);theme.set_editor_property('floor',wood);theme.set_editor_property('ceiling',metal);theme.set_editor_property('grounds',[ground])
    library.set_editor_property('themes',palettes+[theme]);library.set_editor_property('revision',library.get_editor_property('revision')+1)
    a.set_metadata_tag(library,'ERJungleRevision','1');save(library)

path=base+'/DA_Village'
if a.does_asset_exist(path):
    template=load(path)
    if template.get_editor_property('template_id')!='village.jungle':raise RuntimeError('Unowned Village recipe')
else:
    recipe=ue.ERVillageRecipe();recipe.set_editor_property('exterior_trees',trees);recipe.set_editor_property('foliage_surface',leaves);recipe.set_editor_property('trunk_surface',trunk)
    candidate=ue.new_object(ue.ERMapTemplate)
    for key,value in [('template_id','village.jungle'),('revision',1),('archetype',ue.ERMapArchetype.VILLAGE),('symmetry',ue.ERTemplateSymmetry.HALF_TURN),('theme_library',library),('village',recipe),('source_reference','Shoots screenshots/red playable rectangle and owner-supplied ar-shoots-csgo-map.zip; original timber/metal gameplay geometry')]:candidate.set_editor_property(key,value)
    for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
        if not candidate.validate_layout(seed):raise RuntimeError('Village rejected seed='+str(seed))
    factory=ue.DataAssetFactory();factory.set_editor_property('data_asset_class',ue.ERMapTemplate)
    template=tools.create_asset('DA_Village',base,ue.ERMapTemplate,factory)
    for key in ['template_id','revision','archetype','symmetry','theme_library','village','source_reference']:template.set_editor_property(key,candidate.get_editor_property(key))
    a.set_metadata_tag(template,'ERVillageRevision','1');save(template)

levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
map_path='/Game/EclipseRecoil/Maps/L_Village'
if not a.does_asset_exist(map_path):
    if not levels.new_level(map_path):raise RuntimeError('Cannot create Village level')
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world();world.get_world_settings().set_editor_property('default_game_mode',a.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERGameMode'));world.get_world_settings().set_editor_property('kill_z',-800)
    def spawn(cls,label,point=(0,0,100)):
        obj=actors.spawn_actor_from_class(cls,ue.Vector(*point),ue.Rotator(pitch=0,yaw=0,roll=0));obj.set_actor_label(label);return obj
    generator=spawn(ue.ERProceduralMap,'Village_Generator',(0,0,0));config=generator.get_editor_property('layout_config');config.set_editor_property('template_asset',template);generator.set_editor_property('layout_config',config)
    generator.set_editor_property('ceilings',False);generator.set_editor_property('material_catalog',load('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials'));generator.set_editor_property('explosion_effect_class',a.load_blueprint_class('/Game/EclipseRecoil/Effects/Grenade/BP_ERGrenadeExplosion'))
    for team in ['ALPHA','OMEGA']:
        for i in range(10):spawn(ue.ERTeamPlayerStart,team+'_Start_'+str(i)).set_editor_property('team',getattr(ue.ERTeam,team))
    for i in range(7):spawn(ue.ERBotPatrolPoint,'Village_Patrol_'+str(i)).set_editor_property('route_index',i)
    sun=spawn(ue.DirectionalLight,'Village_Sun',(0,0,2500));sun.set_actor_rotation(ue.Rotator(pitch=-60,yaw=-35,roll=0),False);sun.light_component.set_editor_property('intensity',4)
    sky=spawn(ue.SkyLight,'Village_Sky',(0,0,2200));sky.light_component.set_mobility(ue.ComponentMobility.MOVABLE);sky.light_component.set_editor_property('intensity',1.2)
    spawn(ue.SkyAtmosphere,'Village_Atmosphere',(0,0,0))
    if not levels.save_current_level():raise RuntimeError('Cannot save Village map')
for filename,digest in before.items():
    if sha(Path(filename))!=digest:raise RuntimeError('Existing protected asset changed: '+filename)
receipt=root/'Assets/maps/SourceAssets/Village/recipe.json'
data=dict(template=path,map=map_path,theme='Jungle',source='Original geometry inspired by supplied Shoots references',source_archive_sha256=sha(root/'Assets/maps/SourceAssets/Village/ar-shoots-csgo-map.zip'),protected_sha256=before,trees='Two original fallback 270-polygon meshes; 64 exterior instances, no collision',license_note='Shoots archive is reference-only with unspecified redistribution licence; never imported into runtime')
if (receipt.parent/'import-receipt.json').exists():
    data.update(imported_assets='import-receipt.json',revision=template.get_editor_property('revision'),trees='Imported exterior tree; see import-receipt.json; original fallback meshes preserved')
receipt.write_text(json.dumps(data,indent=2)+'\n')
print('ER_VILLAGE_SETUP_COMPLETE seeds=516 protected='+str(len(before)))
