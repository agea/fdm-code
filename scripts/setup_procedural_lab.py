"""Create the standalone procedural lab; preserve existing map edits on reruns."""
import unreal as ue
assets = ue.EditorAssetLibrary
levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
path = '/Game/EclipseRecoil/Maps/L_ProceduralLab'
if assets.does_asset_exist(path):
    ue.log('ER_PROCEDURAL_MAP_PRESERVED: existing map')
else:
    if not levels.new_level(path): raise RuntimeError('Cannot create procedural lab')
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERGameMode'))
    world.get_world_settings().set_editor_property('kill_z', -800)
    def spawn(cls, label, position, yaw=0):
        actor = actors.spawn_actor_from_class(cls, ue.Vector(*position), ue.Rotator(pitch=0,yaw=yaw,roll=0))
        if not actor: raise RuntimeError('Cannot spawn ' + label)
        actor.set_actor_label(label); return actor
    generator = spawn(ue.ERProceduralMap, 'ProceduralFloor', (0,0,0))
    generator.set_editor_property('material_catalog', assets.load_asset('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials'))
    root = '/Game/EclipseRecoil/Art/ProceduralTextures/M_ER_PBR_'
    for prop, name in [('floor_surface','floor_tiles_02'),('wood_floor_surface','wood_floor'),('ceiling_surface','OfficeCeiling001'),('exterior_floor_surface','asphalt_02')]:
        material = assets.load_asset(root+name)
        if not material: raise RuntimeError('Run import_procedural_textures.py first')
        generator.set_editor_property(prop, material)
    generator.set_editor_property('explosion_effect_class', assets.load_blueprint_class('/Game/EclipseRecoil/Effects/Grenade/BP_ERGrenadeExplosion'))
    for team, label in [(ue.ERTeam.ALPHA,'Alpha'),(ue.ERTeam.OMEGA,'Omega')]:
        start = spawn(ue.ERTeamPlayerStart, 'ProceduralStart_'+label, (0,600,100), -90)
        start.set_editor_property('team', team)
    sun = spawn(ue.DirectionalLight, 'ProceduralSun', (0,0,1800))
    sun.set_actor_rotation(ue.Rotator(pitch=-60,yaw=-30,roll=0),False)
    sun.light_component.set_editor_property('intensity',3)
    spawn(ue.SkyLight,'ProceduralSky',(0,0,1800))
    spawn(ue.SkyAtmosphere,'ProceduralAtmosphere',(0,0,0))
    # Managed ambient fill in each slot plus the hall; covers all supported seeds.
    for col in range(5):
        for y in [-800,0,800]:
            fill=spawn(ue.PointLight,'ProceduralFill_'+str(col)+'_'+str(y),(col*1000,y,240))
            light=fill.get_editor_property('light_component')
            light.set_editor_property('intensity',3); light.set_editor_property('attenuation_radius',1300)
            light.set_editor_property('cast_shadows',False)
    if not levels.save_current_level(): raise RuntimeError('Cannot save procedural lab')
    ue.log('ER_PROCEDURAL_MAP_SAVED: L_ProceduralLab; 8 rooms; seed42; runtime walls/doors/windows/floors/ceilings')

# Migration of this managed lab: preserve assigned surfaces, initialize the new
# foundation field, and apply the initial PBR lighting pass only once.
if not levels.load_level(path): raise RuntimeError('Cannot load procedural lab')
for actor in actors.get_all_level_actors():
    if isinstance(actor,ue.ERProceduralMap) and not actor.get_editor_property('exterior_floor_surface'):
        actor.set_editor_property('exterior_floor_surface',assets.load_asset('/Game/EclipseRecoil/Art/ProceduralTextures/M_ER_PBR_asphalt_02'))
level_asset=assets.load_asset(path)
if assets.get_metadata_tag(level_asset,'ERPBRLightingVersion')!='1':
    for actor in actors.get_all_level_actors():
        if isinstance(actor,ue.PointLight) and actor.get_actor_label().startswith('ProceduralFill_'):
            actor.get_editor_property('light_component').set_editor_property('intensity',3)
    assets.set_metadata_tag(level_asset,'ERPBRLightingVersion','1')
if not levels.save_current_level(): raise RuntimeError('Cannot save foundation/PBR lighting setup')
ue.log('ER_PROCEDURAL_FOUNDATION_SAVED: asphalt apron; managed PBR lighting')
