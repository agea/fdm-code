"""Create only absent Dust Mirror template/map assets through Unreal.

Validates 512 seeds before save. Preserves existing maps, catalog and subsequent
artist edits. Run build_dust_mirror_manifest.py and Arabian migration first.
"""
import hashlib, json
from pathlib import Path
import unreal as ue

root = Path(ue.Paths.project_dir())
private = root / 'Assets/maps'
source = private / 'SourceAssets/MapTemplates/dust_mirror_template.json'
data = json.loads(source.read_text())
assets = ue.EditorAssetLibrary
folder = '/Game/EclipseRecoil/Art/MapTemplates'
path = folder + '/DA_DustMirror'
catalog_file = private / 'Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset'
before = hashlib.sha256(catalog_file.read_bytes()).hexdigest()


def load(path):
    obj = assets.load_asset(path)
    if not obj: raise RuntimeError('Missing asset ' + path)
    return obj


def transform(s):
    return ue.Transform(location=ue.Vector(*s['position']),
                        rotation=ue.Rotator(pitch=0, yaw=s['yaw'], roll=0))


template = load(path) if assets.does_asset_exist(path) else None
if template and template.get_editor_property('revision') >= data['revision']:
    if assets.get_metadata_tag(template, 'ERDustManifestSHA256') != hashlib.sha256(source.read_bytes()).hexdigest():
        raise RuntimeError('Completed manifest changed; use an explicit new revision')
    ue.log('ER_DUST_TEMPLATE_PRESERVED')
else:
    candidate = ue.new_object(ue.ERMapTemplate)
    for k, v in [('template_id', data['id']), ('source_reference', data['source']),
                 ('revision', data['revision']), ('arena_size', ue.Vector(*data['arena_size'])),
                 ('symmetry', ue.ERTemplateSymmetry.HALF_TURN),
                 ('shell_wall_mesh', load(data['shell_wall_mesh']))]:
        candidate.set_editor_property(k, v)
    pieces = []
    for item in data['pieces']:
        p = ue.ERMapTemplatePiece()
        for k, v in [('center', ue.Vector(*item['center'])), ('size', ue.Vector(*item['size'])),
                     ('jitter', ue.Vector2D(*item['jitter'])), ('group', item['group']),
                     ('material', getattr(ue.ERBlockMaterial, item['material'].upper())),
                     ('destructible', item['destructible']), ('mirrored', item['mirrored']),
                     ('walkable_surface', item['walkable_surface'])]:
            p.set_editor_property(k, v)
        r = item['mesh_rotation']
        p.set_editor_property('mesh_rotation', ue.Rotator(pitch=r[0], yaw=r[1], roll=r[2]))
        if item['fixed_mesh']: p.set_editor_property('fixed_mesh', load(item['fixed_mesh']))
        pieces.append(p)
    candidate.set_editor_property('pieces', pieces)
    for team in ['alpha', 'omega']:
        candidate.set_editor_property(team + '_spawns', [transform(s) for s in data['spawns'][team]])
    candidate.set_editor_property('patrol_route', [ue.Vector(*p) for p in data['patrol_route']])
    for seed in list(range(512)) + [-17, 7919, 2147483647]:
        if not candidate.validate_layout(seed): raise RuntimeError('Invalid Dust layout seed ' + str(seed))
    if not template:
        factory = ue.DataAssetFactory()
        factory.set_editor_property('data_asset_class', ue.ERMapTemplate)
        template = ue.AssetToolsHelpers.get_asset_tools().create_asset('DA_DustMirror', folder, ue.ERMapTemplate, factory)
    if not template: raise RuntimeError('Cannot create template')
    for field in ['template_id', 'source_reference', 'revision', 'arena_size', 'symmetry',
                  'shell_wall_mesh', 'pieces', 'alpha_spawns', 'omega_spawns', 'patrol_route']:
        template.set_editor_property(field, candidate.get_editor_property(field))
    assets.set_metadata_tag(template, 'ERDustManifestSHA256', hashlib.sha256(source.read_bytes()).hexdigest())
    if not assets.save_loaded_asset(template): raise RuntimeError('Cannot save template')
    ue.log('ER_DUST_TEMPLATE_SAVED seeds=515')

map_path = '/Game/EclipseRecoil/Maps/L_DustMirror'
levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
if assets.does_asset_exist(map_path):
    ue.log('ER_DUST_MAP_PRESERVED')
else:
    if not levels.new_level(map_path): raise RuntimeError('Cannot create level')
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERGameMode'))
    world.get_world_settings().set_editor_property('kill_z', -800)

    def spawn(cls, label, pos, yaw=0):
        a = actors.spawn_actor_from_class(cls, ue.Vector(*pos), ue.Rotator(pitch=0, yaw=yaw, roll=0))
        if not a: raise RuntimeError(label)
        a.set_actor_label(label)
        return a

    generator = spawn(ue.ERProceduralMap, 'DustMirror_Procedural', (0, 0, 0))
    config = generator.get_editor_property('layout_config')
    config.set_editor_property('template_asset', template)
    generator.set_editor_property('layout_config', config)
    generator.set_editor_property('ceilings', False)
    generator.set_editor_property('material_catalog', load('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials'))
    generator.set_editor_property('floor_surface', load(data['floor_surface']))
    generator.set_editor_property('explosion_effect_class', assets.load_blueprint_class('/Game/EclipseRecoil/Effects/Grenade/BP_ERGrenadeExplosion'))
    for team in ['alpha', 'omega']:
        for i, s in enumerate(data['spawns'][team]):
            a = spawn(ue.ERTeamPlayerStart, team + '_Spawn_' + str(i), s['position'], s['yaw'])
            a.set_editor_property('team', getattr(ue.ERTeam, team.upper()))
    for i, point in enumerate(data['patrol_route']):
        a = spawn(ue.ERBotPatrolPoint, 'Dust_Patrol_' + str(i), point)
        a.set_editor_property('route_index', i)
    sun = spawn(ue.DirectionalLight, 'Dust_DesertSun', (0, 0, 1800))
    sun.set_actor_rotation(ue.Rotator(pitch=-55, yaw=-30, roll=0), False)
    sun.light_component.set_editor_property('intensity', 4)
    sky = spawn(ue.SkyLight, 'Dust_Sky', (0, 0, 1800))
    sky.light_component.set_editor_property('intensity', 1)
    spawn(ue.SkyAtmosphere, 'Dust_Atmosphere', (0, 0, 0))
    # A broad, shadow-free fill keeps the low-shadow local profile readable.
    for y in [-2500, 0, 2500]:
        light = spawn(ue.PointLight, 'Dust_Fill_' + str(y), (0, y, 950)).light_component
        light.set_editor_property('intensity', 25)
        light.set_editor_property('attenuation_radius', 3500)
        light.set_editor_property('cast_shadows', False)
    if not levels.save_current_level(): raise RuntimeError('Cannot save map')
    ue.log('ER_DUST_MAP_SAVED')
if hashlib.sha256(catalog_file.read_bytes()).hexdigest() != before:
    raise RuntimeError('Destruction catalog changed')
ue.log('ER_DUST_SETUP_COMPLETE catalog_unchanged=' + before)
