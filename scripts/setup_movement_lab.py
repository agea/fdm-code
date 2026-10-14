"""Update movement defaults/input and create the dedicated 20-obstacle laboratory."""
import unreal as ue
assets = ue.EditorAssetLibrary
levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
character = assets.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter')
cdo = ue.get_default_object(character.generated_class())
cdo.get_editor_property('capsule_component').set_capsule_size(34,92.5)
cdo.get_editor_property('camera').set_relative_location(ue.Vector(0,0,77.5),False,False)
cdo.get_editor_property('mesh').set_relative_location(ue.Vector(0,0,-92.5),False,False)
cdo.set_editor_property('base_eye_height',77.5)
cdo.set_editor_property('crouched_eye_height',50)
movement = cdo.get_editor_property('character_movement')
for prop,value in dict(crouched_half_height=62.5,max_walk_speed=1000/3.6,max_walk_speed_crouched=100,max_acceleration=1000,
                       ground_friction=3,braking_deceleration_walking=850,braking_friction=0,
                       use_separate_braking_friction=True,max_step_height=30,jump_z_velocity=460,
                       gravity_scale=1.8,air_control=.2).items():
    movement.set_editor_property(prop,value)
assets.save_loaded_asset(character,False)
path='/Game/EclipseRecoil/Input/IA_Walk'
action=assets.load_asset(path) if assets.does_asset_exist(path) else ue.AssetToolsHelpers.get_asset_tools().create_asset('IA_Walk','/Game/EclipseRecoil/Input',ue.InputAction,ue.InputAction_Factory())
action.set_editor_property('value_type',ue.InputActionValueType.BOOLEAN)
assets.save_loaded_asset(action,False)
context=assets.load_asset('/Game/EclipseRecoil/Input/IMC_Gameplay')
data=context.get_editor_property('default_key_mappings')
mappings=list(data.get_editor_property('mappings'))
for key in ['LeftShift','RightShift']:
    if not any(m.get_editor_property('action')==action and str(m.get_editor_property('key').get_editor_property('key_name'))==key for m in mappings):
        m=ue.EnhancedActionKeyMapping(); m.set_editor_property('action',action)
        k=ue.Key(); k.set_editor_property('key_name',key); m.set_editor_property('key',k); mappings.append(m)
data.set_editor_property('mappings',mappings); context.set_editor_property('default_key_mappings',data)
assets.save_loaded_asset(context,False)
pc=assets.load_asset('/Game/EclipseRecoil/Core/BP_ERPlayerController')
ue.get_default_object(pc.generated_class()).set_editor_property('walk_action',action)
assets.save_loaded_asset(pc,False)
map_path='/Game/EclipseRecoil/Maps/L_MovementLab'
# Only create geometry on first run; preserve subsequent edits to the lab.
if not assets.does_asset_exist(map_path):
    if not levels.new_level(map_path): raise RuntimeError('Cannot create movement laboratory')
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERGameMode'))
    world.get_world_settings().set_editor_property('kill_z',-800)
    def spawn(cls,label,pos,yaw=0):
        a=actors.spawn_actor_from_class(cls,ue.Vector(*pos),ue.Rotator(pitch=0,yaw=yaw,roll=0)); a.set_actor_label(label); return a
    def block(label,pos,size,material):
        a=spawn(ue.StaticMeshActor,label,pos)
        a.static_mesh_component.set_static_mesh(assets.load_asset('/Engine/BasicShapes/Cube'))
        a.static_mesh_component.set_collision_profile_name('BlockAll')
        a.static_mesh_component.set_material(0,assets.load_asset('/Game/EclipseRecoil/Art/'+material))
        a.set_actor_scale3d(ue.Vector(*(v/100 for v in size)))
    def text(label,value,pos,yaw=180,size=28):
        a=spawn(ue.TextRenderActor,label,pos,yaw)
        t=a.get_editor_property('text_render'); t.set_text(value); t.set_world_size(size)
        t.set_text_render_color(ue.Color(255,240,210,255))
        t.set_horizontal_alignment(ue.HorizTextAligment.EHTA_CENTER)
    block('Lab_Floor',(500,0,-25),(3400,4600,50),'MI_Floor')
    for index in range(20):
        height=(index+1)*10; row=index//5; col=index%5
        x=row*650; y=(col-2)*650
        material='MI_Alpha' if height<=30 else ('MI_Cover' if height<=80 else 'MI_Omega')
        block('Lab_Block_%03dcm'%height,(x,y,height/2),(200,200,height),material)
        text('Height_%03dcm'%height,'%d cm'%height,(x-102,y,max(25,height/2)),180,26)
        # Upright repeat sign remains legible for the smallest obstacles.
        text('Sign_%03dcm'%height,'%d cm  |  2 x 2 m'%height,(x-235,y,105),180,22)
    for team,y in [(ue.ERTeam.ALPHA,0),(ue.ERTeam.OMEGA,300)]:
        a=spawn(ue.ERTeamPlayerStart,'Lab_Start_'+str(team),(-700,y,100)); a.set_editor_property('team',team)
    text('Instructions','MOVEMENT LAB\n10 km/h | SHIFT: 5 km/h\n<=30 cm: STEP   <=80 cm: SLOW VAULT\nSPACE + FORWARD: CLIMB (150 cm FROM FEET)',(-1050,0,220),0,26)
    sun=spawn(ue.DirectionalLight,'LabSun',(0,0,900)); sun.set_actor_rotation(ue.Rotator(pitch=-55,yaw=-25,roll=0),False); sun.light_component.set_editor_property('intensity',3)
    spawn(ue.SkyLight,'LabSky',(0,0,800)); spawn(ue.SkyAtmosphere,'LabAtmosphere',(0,0,0))
    if not levels.save_current_level(): raise RuntimeError('Cannot save lab')
ue.log('ER_MOVEMENT_LAB_SAVED: 20 blocks, 200x200cm, heights 10..200; capsule 185cm; Shift walk')
# Add a perimeter on reruns too, keeping authored blocks untouched.
if not levels.load_level(map_path): raise RuntimeError('Cannot reload laboratory')
existing={a.get_actor_label():a for a in actors.get_all_level_actors()}
for label,pos,size in [
    ('LabBoundary_West',(-1200,0,125),(50,4600,250)),
    ('LabBoundary_East',(2200,0,125),(50,4600,250)),
    ('LabBoundary_South',(500,-2300,125),(3400,50,250)),
    ('LabBoundary_North',(500,2300,125),(3400,50,250))]:
    if label not in existing:
        a=actors.spawn_actor_from_class(ue.StaticMeshActor,ue.Vector(*pos),ue.Rotator(pitch=0,yaw=0,roll=0))
        a.set_actor_label(label); a.static_mesh_component.set_static_mesh(assets.load_asset('/Engine/BasicShapes/Cube'))
        a.static_mesh_component.set_collision_profile_name('BlockAll')
        a.static_mesh_component.set_material(0,assets.load_asset('/Game/EclipseRecoil/Art/MI_Wall'))
        a.set_actor_scale3d(ue.Vector(*(v/100 for v in size)))
for height in range(10,201,10):
    a=existing.get('Lab_Block_%03dcm'%height)
    if not a: raise RuntimeError('Missing block %d'%height)
    s=a.get_actor_scale3d()
    if abs(s.x-2)>.001 or abs(s.y-2)>.001 or abs(s.z-height/100)>.001:
        raise RuntimeError('Wrong block dimensions %d'%height)
levels.save_current_level()
ue.log('ER_MOVEMENT_GEOMETRY_PASS: all 20 block dimensions verified; perimeter saved')

for a in actors.get_all_level_actors():
    if isinstance(a,ue.TextRenderActor):
        a.get_editor_property('text_render').set_text_render_color(ue.Color(10,18,28,255))
levels.save_current_level()
ue.log('ER_MOVEMENT_LABELS_SAVED: dark height labels for bright laboratory surfaces')
