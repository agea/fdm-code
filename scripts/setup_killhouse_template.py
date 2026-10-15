"""Unreal-only, additive creation of a reusable template and a separate map.

Run analyze_source_template.py first. All binary writes use Unreal and stay
in the private maps mount. Existing templates and edited maps are preserved.
"""
import hashlib,json
from pathlib import Path
import unreal as ue
assets=ue.EditorAssetLibrary
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
root=Path(ue.Paths.project_dir())
source=root/'Assets/maps/SourceAssets/MapTemplates/killhouse_template.json'
data=json.loads(source.read_text())
folder='/Game/EclipseRecoil/Art/MapTemplates'
path=folder+'/DA_Killhouse'
template=assets.load_asset(path) if assets.does_asset_exist(path) else None
def transform(position,yaw=0):
    return ue.Transform(location=ue.Vector(*position),rotation=ue.Rotator(pitch=0,yaw=yaw,roll=0),scale=ue.Vector(1,1,1))
if not template:
    factory=ue.DataAssetFactory();factory.set_editor_property('data_asset_class',ue.ERMapTemplate)
    template=ue.AssetToolsHelpers.get_asset_tools().create_asset('DA_Killhouse',folder,ue.ERMapTemplate,factory)
    if not template:raise RuntimeError('Cannot create template')
    template.set_editor_property('template_id',data['id'])
    template.set_editor_property('source_reference',data['source'])
    template.set_editor_property('revision',data['revision'])
    template.set_editor_property('arena_size',ue.Vector(*data['arena_size']))
    if data.get('symmetry')=='HalfTurn':template.set_editor_property('symmetry',ue.ERTemplateSymmetry.HALF_TURN)
    for field in ['shell_wall_mesh','roof_mesh']:
        if data.get(field):
            mesh=assets.load_asset(data[field])
            if not mesh:raise RuntimeError('Run Factory migration first: '+data[field])
            template.set_editor_property(field,mesh)
    pieces=[]
    for item in data['pieces']:
        p=ue.ERMapTemplatePiece()
        for name,value in [('center',ue.Vector(*item['center'])),('size',ue.Vector(*item['size'])),('jitter',ue.Vector2D(*item['jitter'])),('group',item['group']),('destructible',item['destructible']),('material',getattr(ue.ERBlockMaterial,item['material'].upper()))]:
            p.set_editor_property(name,value)
        for prop in ['mirrored','anchored']:
            if prop in item:p.set_editor_property(prop,item[prop])
        rotation=item.get('mesh_rotation',[0,0,0]);p.set_editor_property('mesh_rotation',ue.Rotator(pitch=rotation[0],yaw=rotation[1],roll=rotation[2]))
        if item.get('fixed_mesh'):
            if item['destructible']:raise RuntimeError('Vendor art cannot replace a destructible block')
            mesh=assets.load_asset(item['fixed_mesh'])
            if not mesh:raise RuntimeError('Run Factory migration first: '+item['fixed_mesh'])
            p.set_editor_property('fixed_mesh',mesh)
        pieces.append(p)
    template.set_editor_property('pieces',pieces)
    for team in ['alpha','omega']:
        template.set_editor_property(team+'_spawns',[transform(s['position'],s['yaw']) for s in data['spawns'][team]])
    template.set_editor_property('patrol_route',[ue.Vector(*p) for p in data['patrol_route']])
    assets.set_metadata_tag(template,'ERSourceBspSHA256',json.loads((source.parent/'killhouse_analysis.json').read_text())['bsp_sha256'])
    if not template.validate_layout(42):raise RuntimeError('Invalid template manifest')
    if data.get('revision')==2:
        assets.set_metadata_tag(template,'ERFactoryBalancedRevision','2')
        assets.set_metadata_tag(template,'ERFactoryManifestSHA256',hashlib.sha256(source.read_bytes()).hexdigest())
    if not assets.save_loaded_asset(template):raise RuntimeError('Cannot save template')
else:ue.log('ER_KILLHOUSE_TEMPLATE_PRESERVED')
map_path='/Game/EclipseRecoil/Maps/L_Killhouse'
if assets.does_asset_exist(map_path):
    ue.log('ER_KILLHOUSE_MAP_PRESERVED')
else:
    if not levels.new_level(map_path):raise RuntimeError('Cannot create killhouse')
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERGameMode'))
    world.get_world_settings().set_editor_property('kill_z',-800)
    def spawn(cls,label,pos,yaw=0):
        a=actors.spawn_actor_from_class(cls,ue.Vector(*pos),ue.Rotator(pitch=0,yaw=yaw,roll=0))
        if not a:raise RuntimeError(label)
        a.set_actor_label(label);return a
    generator=spawn(ue.ERProceduralMap,'Killhouse_Procedural',(0,0,0))
    config=generator.get_editor_property('layout_config');config.set_editor_property('template_asset',template);generator.set_editor_property('layout_config',config)
    catalog=assets.load_asset('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials')
    if not catalog:raise RuntimeError('Missing project destruction material catalog')
    generator.set_editor_property('material_catalog',catalog)
    art='/Game/EclipseRecoil/Art/ProceduralTextures/'
    for prop,name in [('floor_surface','M_ER_PBR_concrete_wall_006'),('ceiling_surface','M_ER_PBR_rusty_metal_04')]:
        material=assets.load_asset(art+name)
        if not material:
            material=assets.load_asset(art+('M_ER_PBR_asphalt_02' if prop=='floor_surface' else 'M_ER_PBR_rusty_metal_04'))
        if not material:raise RuntimeError('Missing surface '+name)
        generator.set_editor_property(prop,material)
    generator.set_editor_property('explosion_effect_class',assets.load_blueprint_class('/Game/EclipseRecoil/Effects/Grenade/BP_ERGrenadeExplosion'))
    for team in ['alpha','omega']:
        for i,s in enumerate(data['spawns'][team]):
            a=spawn(ue.ERTeamPlayerStart,team+'_Spawn_'+str(i),s['position'],s['yaw']);a.set_editor_property('team',getattr(ue.ERTeam,team.upper()))
    for i,p in enumerate(data['patrol_route']):
        a=spawn(ue.ERBotPatrolPoint,'Killhouse_Patrol_'+str(i),p);a.set_editor_property('route_index',i)
    sun=spawn(ue.DirectionalLight,'Killhouse_Sun',(0,0,1800));sun.set_actor_rotation(ue.Rotator(pitch=-60,yaw=-30,roll=0),False);sun.light_component.set_editor_property('intensity',3)
    sky=spawn(ue.SkyLight,'Killhouse_Sky',(0,0,1800));sky.light_component.set_editor_property('intensity',1)
    spawn(ue.SkyAtmosphere,'Killhouse_Atmosphere',(0,0,0))
    for y in range(-3200,3201,800):
        for x in [-1350,0,1350]:
            light=spawn(ue.PointLight,'Killhouse_Fill_'+str(x)+'_'+str(y),(x,y,700)).light_component
            light.set_editor_property('intensity',18);light.set_editor_property('attenuation_radius',1800);light.set_editor_property('cast_shadows',False)
    # Runtime generation owns ISMs; do not serialize their transient mappings.
    if not levels.save_current_level():raise RuntimeError('Cannot save killhouse map')
    ue.log('ER_KILLHOUSE_MAP_SAVED')
ue.log('ER_KILLHOUSE_SETUP_COMPLETE')
