"""Run inside UE 5.8: py /absolute/path/scripts/setup_editor.py.

Creates the prototype assets once. Existing assets/maps are preserved on later runs.
All gameplay belongs to native classes, not the Shooter template Blueprints.
"""
from pathlib import Path
import runpy
import unreal as ue

ROOT = "/Game/EclipseRecoil"
tools = ue.AssetToolsHelpers.get_asset_tools()
assets = ue.EditorAssetLibrary
levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)


def load(path):
    value = assets.load_asset(path)
    if not value:
        raise RuntimeError("Missing asset: " + path)
    return value


def create(name, folder, cls, factory):
    path = ROOT + "/" + folder + "/" + name
    if assets.does_asset_exist(path):
        return load(path)
    value = tools.create_asset(name, ROOT + "/" + folder, cls, factory)
    if not value:
        raise RuntimeError("Cannot create: " + path)
    return value


def blueprint(name, folder, parent):
    factory = ue.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    return create(name, folder, ue.Blueprint, factory)


def cdo(bp):
    return ue.get_default_object(bp.generated_class())


def save(value):
    if not assets.save_loaded_asset(value, only_if_is_dirty=False):
        raise RuntimeError("Cannot save: " + value.get_path_name())


actions = {}
for name in ["Move", "Look", "Jump", "Crouch", "Fire", "Reload", "Scoreboard", "Menu", "Walk"]:
    action = create("IA_" + name, "Input", ue.InputAction, ue.InputAction_Factory())
    action.set_editor_property("value_type", ue.InputActionValueType.AXIS2D if name in ["Move", "Look"] else ue.InputActionValueType.BOOLEAN)
    if name == "Look":
        scalar = ue.InputModifierScalar(outer=action)
        scalar.set_editor_property("scalar", ue.Vector(.18, .18, .18))
        action.set_editor_property("modifiers", [scalar])
    # Avoid key repetition translating into multiple Started events.
    actions[name] = action
    save(action)

context = create("IMC_Gameplay", "Input", ue.InputMappingContext, ue.InputMappingContext_Factory())
context.unmap_all()
mappings = []


def key(action, key_name, negate=False, swizzle=False):
    mapping = ue.EnhancedActionKeyMapping()
    mapping.set_editor_property("action", actions[action])
    binding = ue.Key()
    binding.set_editor_property("key_name", key_name)
    mapping.set_editor_property("key", binding)
    modifiers = []
    if negate:
        modifiers.append(ue.InputModifierNegate(outer=context))
    if swizzle:
        modifier = ue.InputModifierSwizzleAxis(outer=context)
        modifier.set_editor_property("order", ue.InputAxisSwizzle.YXZ)
        modifiers.append(modifier)
    mapping.set_editor_property("modifiers", modifiers)
    mappings.append(mapping)


key("Move", "W", swizzle=True)
key("Move", "S", negate=True, swizzle=True)
key("Move", "D")
key("Move", "A", negate=True)
key("Look", "MouseX")
key("Look", "MouseY", swizzle=True)
for action, binding in [("Fire", "LeftMouseButton"), ("Jump", "SpaceBar"), ("Crouch", "LeftControl"), ("Reload", "R"), ("Scoreboard", "Tab"), ("Menu", "Escape")]:
    key(action, binding)
key("Crouch", "C")
key("Walk", "LeftShift")
key("Walk", "RightShift")
data = context.get_editor_property("default_key_mappings")
data.set_editor_property("mappings", mappings)
context.set_editor_property("default_key_mappings", data)
save(context)

data_factory = ue.DataAssetFactory()
data_factory.set_editor_property("data_asset_class", ue.ERWeaponData)
rifle_data = create("DA_Rifle_A", "Weapons", ue.ERWeaponData, data_factory)
save(rifle_data)
projectile = blueprint("BP_ERProjectile", "Weapons", ue.ERProjectile)
save(projectile)
runpy.run_path(str(Path(__file__).with_name("setup_bullet_impact.py")))
weapon = blueprint("BP_ERWeapon_Rifle", "Weapons", ue.ERWeapon)
cdo(weapon).set_editor_property("data", rifle_data)
cdo(weapon).set_editor_property("projectile_class", projectile.generated_class())
save(weapon)

# One material parameter supports both faction colours and environment blocks.
material_path = ROOT + "/Art/M_Team"
material = create("M_Team", "Art", ue.Material, ue.MaterialFactoryNew())
if not ue.MaterialEditingLibrary.get_num_material_expressions(material):
    expression = ue.MaterialEditingLibrary.create_material_expression(material, ue.MaterialExpressionVectorParameter, -300, 0)
    expression.set_editor_property("parameter_name", "Color")
    expression.set_editor_property("default_value", ue.LinearColor(.15, .3, .5, 1))
    ue.MaterialEditingLibrary.connect_material_property(expression, "", ue.MaterialProperty.MP_BASE_COLOR)
    ue.MaterialEditingLibrary.connect_material_property(expression, "", ue.MaterialProperty.MP_EMISSIVE_COLOR)
    ue.MaterialEditingLibrary.recompile_material(material)
save(material)

character = blueprint("BP_ERCharacter", "Core", ue.ERCharacter)
defaults = cdo(character)
defaults.set_editor_property("weapon_class", weapon.generated_class())
manny = load("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
rifle = load("/Game/Weapons/Rifle/Meshes/SKM_Rifle")
mesh = defaults.get_editor_property("mesh")
mesh.set_skeletal_mesh_asset(manny)
mesh.set_anim_instance_class(assets.load_blueprint_class("/Game/Variant_Shooter/Anims/ABP_TP_Rifle"))
first = defaults.get_editor_property("first_person_body")
first.set_editor_property("relative_location", ue.Vector(-10, 0, -152))
first.set_editor_property("relative_rotation", ue.Rotator(pitch=0, yaw=-90, roll=0))
first.set_skeletal_mesh_asset(manny)
first.set_editor_property("animation_mode", ue.AnimationMode.ANIMATION_SINGLE_NODE)
anim = first.get_editor_property("animation_data")
hip_pose = "/Game/EclipseRecoil/Characters/A_ER_Rifle_HipPose"
if not assets.does_asset_exist(hip_pose):
    runpy.run_path(str(Path(__file__).with_name("create_rifle_pose.py")))
anim.set_editor_property("anim_to_play", load(hip_pose))
anim.set_editor_property("saved_looping", True)
first.set_editor_property("animation_data", anim)
for name in ["first_person_rifle", "world_rifle"]:
    defaults.get_editor_property(name).set_skeletal_mesh_asset(rifle)
for name in ["first_person_rifle", "world_rifle"]:
    component = defaults.get_editor_property(name)
    component.set_editor_property("relative_location", ue.Vector(0, 0, 0))
    component.set_editor_property("relative_rotation", ue.Rotator(pitch=0, yaw=0, roll=0))
marker = defaults.get_editor_property("team_marker")
marker.set_static_mesh(load("/Engine/BasicShapes/Sphere"))
marker.set_material(0, material)
save(character)
runpy.run_path(str(Path(__file__).with_name("setup_shot_feedback.py")))

controller = blueprint("BP_ERPlayerController", "Core", ue.ERPlayerController)
defaults = cdo(controller)
defaults.set_editor_property("gameplay_mapping", context)
for name, action in actions.items():
    defaults.set_editor_property(name.lower() + "_action", action)
save(controller)
mode = blueprint("BP_ERGameMode", "Core", ue.ERGameMode)
cdo(mode).set_editor_property("default_pawn_class", character.generated_class())
cdo(mode).set_editor_property("player_controller_class", controller.generated_class())
save(mode)


def spawn(cls, location, rotation=(0, 0, 0), label=None):
    actor = actors.spawn_actor_from_class(cls, ue.Vector(*location), ue.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    if label:
        actor.set_actor_label(label)
    return actor


def block(label, location, size, colour):
    actor = spawn(ue.StaticMeshActor, location, label=label)
    component = actor.static_mesh_component
    component.set_static_mesh(load("/Engine/BasicShapes/Cube"))
    actor.set_actor_scale3d(ue.Vector(*(x / 100 for x in size)))
    component.set_material(0, colour)
    component.set_collision_profile_name("BlockAll")
    return actor


def instance(name, colour):
    factory = ue.MaterialInstanceConstantFactoryNew()
    value = create(name, "Art", ue.MaterialInstanceConstant, factory)
    ue.MaterialEditingLibrary.set_material_instance_parent(value, material)
    ue.MaterialEditingLibrary.set_material_instance_vector_parameter_value(value, "Color", ue.LinearColor(*colour, 1))
    save(value)
    return value


floor_material = instance("MI_Floor", (.06, .09, .12))
wall_material = instance("MI_Wall", (.14, .18, .22))
cover_material = instance("MI_Cover", (.24, .3, .32))
alpha_material = instance("MI_Alpha", (.02, .18, .5))
omega_material = instance("MI_Omega", (.5, .055, .015))

depot_path = ROOT + "/Maps/L_Depot"
if not assets.does_asset_exist(depot_path):
    if not levels.new_level(depot_path):
        raise RuntimeError("Cannot create depot")
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", mode.generated_class())
    world.get_world_settings().set_editor_property("kill_z", -800)
    block("Depot_Floor", (0,0,-25), (5600,3600,50), floor_material)
    for label, loc, size in [("NorthWall",(0,1800,200),(5600,50,450)), ("SouthWall",(0,-1800,200),(5600,50,450)), ("AlphaWall",(-2800,0,200),(50,3600,450)), ("OmegaWall",(2800,0,200),(50,3600,450))]:
        block(label,loc,size,wall_material)
    # Central stack blocks direct fire between spawn zones; side lanes remain traversable.
    block("CentralDepot", (0,0,180), (1000,1300,360), cover_material)
    for x in [-1650,1650]:
        for y in [-800,800]:
            block("Container", (x,y,100), (700,350,200), cover_material)
    for x in [-700,700]:
        for y in [-1350,1350]:
            block("LowCover", (x,y,55), (350,180,110), cover_material)
    block("AlphaSpawnStripe",(-2450,0,2),(400,3000,4),alpha_material)
    block("OmegaSpawnStripe",(2450,0,2),(400,3000,4),omega_material)
    for team,x,yaw in [(ue.ERTeam.ALPHA,-2400,0),(ue.ERTeam.OMEGA,2400,180)]:
        for y in [-550,550]:
            start = spawn(ue.ERTeamPlayerStart,(x,y,100),(0,yaw,0),"Spawn_" + str(team) + "_" + str(y))
            start.set_editor_property("team",team)
    sun = spawn(ue.DirectionalLight,(0,0,800),(-50,-25,0),"DepotSun")
    sun.light_component.set_editor_property("intensity",3)
    spawn(ue.SkyLight,(0,0,600),label="DepotSky")
    spawn(ue.SkyAtmosphere,(0,0,0),label="Atmosphere")
    levels.save_current_level()

menu_path = ROOT + "/Maps/L_MainMenu"
if not assets.does_asset_exist(menu_path):
    if not levels.new_level(menu_path):
        raise RuntimeError("Cannot create menu")
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", mode.generated_class())
    levels.save_current_level()
runpy.run_path(str(Path(__file__).with_name("setup_bot_routes.py")))
runpy.run_path(str(Path(__file__).with_name("setup_movement_lab.py")))
assets.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
ue.log("ER_SETUP_SUCCESS: Blueprints, input, rifle data and both maps saved.")
