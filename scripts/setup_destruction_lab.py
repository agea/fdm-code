"""Create the standalone block room laboratory without changing existing maps."""
import unreal as ue
assets = ue.EditorAssetLibrary
levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
tools = ue.AssetToolsHelpers.get_asset_tools()
root = '/Game/EclipseRecoil/Art/Destruction'

def save(asset):
    if not assets.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Cannot save ' + asset.get_path_name())

# New materials use project-owned expressions; reruns preserve artist edits.
materials = []
for name, color in [('Drywall', (.65, .72, .78)), ('Brick', (.5, .16, .08)), ('Concrete', (.26, .28, .3))]:
    path = root + '/M_ER_Block_' + name
    material = assets.load_asset(path) if assets.does_asset_exist(path) else None
    if not material:
        material = tools.create_asset('M_ER_Block_' + name, root, ue.Material, ue.MaterialFactoryNew())
        material.set_editor_property('used_with_instanced_static_meshes', True)
        base = ue.MaterialEditingLibrary.create_material_expression(material, ue.MaterialExpressionConstant3Vector, -300, 0)
        base.set_editor_property('constant', ue.LinearColor(*color, 1))
        roughness = ue.MaterialEditingLibrary.create_material_expression(material, ue.MaterialExpressionConstant, -300, 100)
        roughness.set_editor_property('r', .9)
        ue.MaterialEditingLibrary.connect_material_property(base, '', ue.MaterialProperty.MP_BASE_COLOR)
        ue.MaterialEditingLibrary.connect_material_property(roughness, '', ue.MaterialProperty.MP_ROUGHNESS)
        ue.MaterialEditingLibrary.recompile_material(material)
        save(material)
    materials.append(material)

catalog_path = root + '/DA_ER_BlockMaterials'
catalog = assets.load_asset(catalog_path) if assets.does_asset_exist(catalog_path) else None
if not catalog:
    factory = ue.DataAssetFactory()
    factory.set_editor_property('data_asset_class', ue.ERBlockMaterialCatalog)
    catalog = tools.create_asset('DA_ER_BlockMaterials', root, ue.ERBlockMaterialCatalog, factory)
    entries = list(catalog.get_editor_property('materials'))
    for entry, material in zip(entries, materials):
        entry.set_editor_property('surface', material)
    catalog.set_editor_property('materials', entries)
    save(catalog)

path = '/Game/EclipseRecoil/Maps/L_DestructionLab'
if not assets.does_asset_exist(path):
    if not levels.new_level(path):
        raise RuntimeError('Cannot create destruction laboratory')
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERGameMode'))
    world.get_world_settings().set_editor_property('kill_z', -800)
    def spawn(cls, label, location, yaw=0):
        actor = actors.spawn_actor_from_class(cls, ue.Vector(*location), ue.Rotator(pitch=0, yaw=yaw, roll=0))
        if not actor:
            raise RuntimeError('Cannot spawn ' + label)
        actor.set_actor_label(label)
        return actor
    room = spawn(ue.ERBlockRoom, 'DestructibleRoom', (0, 0, 0))
    room.set_editor_property('material_catalog', catalog)
    # Engine geometry is generated at BeginPlay, not saved as hundreds of actors.
    for team, y in [(ue.ERTeam.ALPHA, -100), (ue.ERTeam.OMEGA, 100)]:
        start = spawn(ue.ERTeamPlayerStart, 'Destruction_Start_' + str(team), (0, y, 100))
        start.set_editor_property('team', team)
    light = spawn(ue.DirectionalLight, 'DestructionSun', (0, 0, 900))
    light.set_actor_rotation(ue.Rotator(pitch=-60, yaw=-30, roll=0), False)
    light.light_component.set_editor_property('intensity', 3)
    spawn(ue.SkyLight, 'DestructionSky', (0, 0, 800))
    spawn(ue.SkyAtmosphere, 'DestructionAtmosphere', (0, 0, 0))
    if not levels.save_current_level():
        raise RuntimeError('Cannot save destruction laboratory')
else:
    ue.log('ER_BLOCK_LAB_PRESERVED: existing map left unchanged')
# A managed fill light makes all three profiles readable with the low-GI
# development launch settings. Add it once without replacing edited geometry.
if not levels.load_level(path):
    raise RuntimeError('Cannot load destruction laboratory')
fill = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == 'DestructionFill'), None)
if not fill:
    fill = actors.spawn_actor_from_class(ue.PointLight, ue.Vector(0, 0, 250), ue.Rotator(pitch=0, yaw=0, roll=0))
    fill.set_actor_label('DestructionFill')
light = fill.get_editor_property('light_component')
light.set_editor_property('intensity', 12)
light.set_editor_property('attenuation_radius', 1000)
light.set_editor_property('cast_shadows', False)
if not levels.save_current_level():
    raise RuntimeError('Cannot save laboratory fill light')
ue.log('ER_BLOCK_LAB_SAVED: L_DestructionLab; standalone; 500x500x300cm interior; 3 materials')

# Add owned material-specific impact decals; preserve existing art expressions.
decals = []
for name, color in [('Drywall', (.16, .18, .2)), ('Brick', (.23, .055, .02)), ('Concrete', (.1, .11, .12))]:
    decal_path = root + '/M_ER_BlockImpact_' + name
    decal = assets.load_asset(decal_path) if assets.does_asset_exist(decal_path) else None
    if not decal:
        decal = tools.create_asset('M_ER_BlockImpact_' + name, root, ue.Material, ue.MaterialFactoryNew())
        decal.set_editor_property('material_domain', ue.MaterialDomain.MD_DEFERRED_DECAL)
        decal.set_editor_property('blend_mode', ue.BlendMode.BLEND_TRANSLUCENT)
        base = ue.MaterialEditingLibrary.create_material_expression(decal, ue.MaterialExpressionConstant3Vector, -300, 0)
        base.set_editor_property('constant', ue.LinearColor(*color, 1))
        uv = ue.MaterialEditingLibrary.create_material_expression(decal, ue.MaterialExpressionTextureCoordinate, -500, 150)
        mask = ue.MaterialEditingLibrary.create_material_expression(decal, ue.MaterialExpressionCustom, -300, 150)
        mask.set_editor_property('output_type', ue.CustomMaterialOutputType.CMOT_FLOAT1)
        mask.set_editor_property('code', 'return saturate((0.45-length(UV-0.5))*12)*0.65;')
        arg = ue.CustomInput(); arg.set_editor_property('input_name', 'UV'); mask.set_editor_property('inputs', [arg])
        ue.MaterialEditingLibrary.connect_material_expressions(uv, '', mask, 'UV')
        ue.MaterialEditingLibrary.connect_material_property(base, '', ue.MaterialProperty.MP_BASE_COLOR)
        ue.MaterialEditingLibrary.connect_material_property(mask, '', ue.MaterialProperty.MP_OPACITY)
        ue.MaterialEditingLibrary.recompile_material(decal); save(decal)
    decals.append(decal)

# Versioned migration touches only new gameplay/FX fields, preserving HP and surfaces.
if assets.get_metadata_tag(catalog, 'ERBlockCombatVersion') != '2':
    entries = list(catalog.get_editor_property('materials'))
    if not all(any(row.get_editor_property('type')==kind for row in entries) for kind in [ue.ERBlockMaterial.DRYWALL,ue.ERBlockMaterial.BRICK,ue.ERBlockMaterial.CONCRETE]):
        raise RuntimeError('Missing original block profiles')
    types = [ue.ERBlockMaterial.DRYWALL, ue.ERBlockMaterial.BRICK, ue.ERBlockMaterial.CONCRETE]
    for entry in entries:
        if entry.get_editor_property('type') not in types: continue
        index = types.index(entry.get_editor_property('type'))
        for prop, values in {
            'bullet_damage_scale': [.5, 1, 1], 'entry_energy_cost': [5, 20, 40],
            'energy_cost_per_cm': [.5, 5, 15], 'explosion_damage_scale': [2, .8, .25]}.items():
            entry.set_editor_property(prop, values[index])
        entry.set_editor_property('impact_decal', decals[index])
        entry.set_editor_property('impact_system', assets.load_asset('/Game/EclipseRecoil/Effects/Shots/NS_ER_Impact'))
    catalog.set_editor_property('materials', entries)
    assets.set_metadata_tag(catalog, 'ERBlockCombatVersion', '2')
    save(catalog)

existing = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
for actor in existing.values():
    if isinstance(actor, ue.ERBlockRoom) and not actor.get_editor_property('explosion_effect_class'):
        actor.set_editor_property('explosion_effect_class', assets.load_blueprint_class('/Game/EclipseRecoil/Effects/Grenade/BP_ERGrenadeExplosion'))
for name, position, yaw, material in [
    ('Drywall', (0, -400, 170), 180, materials[0]),
    ('Brick', (400, 0, 170), -90, materials[1]),
    ('Concrete', (0, 400, 170), 0, materials[2])]:
    label = 'DestructionTarget_' + name
    if label not in existing:
        target = actors.spawn_actor_from_class(ue.ERBlockTarget, ue.Vector(*position), ue.Rotator(pitch=0, yaw=yaw, roll=0))
        target.set_actor_label(label); target.set_editor_property('tags', ['ERBlock' + name])
        target.get_editor_property('mesh').set_material(0, material)
if not levels.save_current_level():
    raise RuntimeError('Cannot save block combat target setup')
ue.log('ER_BLOCK_COMBAT_SAVED: penetration profiles, three targets, material impacts, standalone explosions')
