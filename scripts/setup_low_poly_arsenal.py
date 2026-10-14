"""Configure project-local weapon data, grip poses and Blueprint defaults.

Imported Fab meshes/template assets stay unchanged. Art references live in data
assets and Blueprint defaults; no runtime paths into Downloads.
"""
from pathlib import Path
import json
import runpy
import unreal as ue

assets = ue.EditorAssetLibrary
tools = ue.AssetToolsHelpers.get_asset_tools()
folder = '/Game/EclipseRecoil/Weapons/LowPoly'
pose_folder = '/Game/EclipseRecoil/Characters/FirstPerson/Weapons'
body = ue.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/FirstPerson/SK_ER_Twinblast_Alpha_Officer_Forearms')
if not body:
    raise RuntimeError('Missing FPS mercenary')

# Roles and base ballistics follow the local original's arsenal.cfg/tdm.cfg.
# M4 is an additional assault rifle; spread/reload/rocket values are port choices.
profiles = [
    dict(name='AK74', mesh='SK_AK74', damage=36, interval=.1, ammo=30, reserve=90, auto=True, left=(0,28,0), spread=.18, pitch=1.),
    dict(name='M4', mesh='SK_M4_8', damage=33, interval=.09, ammo=30, reserve=90, auto=True, left=(0,24,0), spread=.16, pitch=1.08, sight='SK_Optic_010', sight_x=-6, sight_z=5),
    dict(name='Uzi', mesh='SK_Uzi', damage=26, interval=.07, ammo=30, reserve=60, auto=True, left=(0,14,-2), spread=.35, pitch=1.3, view=(6,0,8)),
    dict(name='Shotgun M4', mesh='SK_SG_M4', damage=20, interval=.35, ammo=7, reserve=32, auto=True, left=(0,27,0), spread=3.2, pellets=6, pitch=.8),
    dict(name='M249', mesh='SK_M249', damage=32, interval=.08, ammo=100, reserve=200, auto=True, left=(0,28,0), spread=.3, reload=3., pitch=.9),
    dict(name='M107', mesh='SK_M107', damage=115, interval=1.455, ammo=5, reserve=10, auto=False, left=(0,32,0), spread=.12, zoom=20, reload=2.5, pitch=.7, sight='SK_Optic_02_Black', sight_x=-20, sight_z=4),
    dict(name='RPG7', mesh='SK_RPG7', damage=140, interval=1.2, ammo=1, reserve=4, auto=False, left=(0,20,0), spread=.3, speed=2200, payload=ue.ERPayload.ROCKET, reload=2.5, pitch=.6),
    dict(name='M1911', mesh='SK_M1911', damage=53, interval=.225, ammo=7, reserve=21, auto=False, left=(0,3,-3), spread=.25, pitch=1.25, one=True, view=(20,-3,10), zoom=75),
    dict(name='RGD5 · HE', mesh='SK_Grenade_RGD5', damage=140, interval=.8, ammo=2, reserve=0, auto=False, left=(0,8,-2), spread=0, speed=1100, payload=ue.ERPayload.HE, one=True, pitch=.7),
    dict(name='Fumogena', mesh='SK_Grenade_Smoke', damage=0, interval=.8, ammo=1, reserve=0, auto=False, left=(0,8,-2), spread=0, speed=1100, payload=ue.ERPayload.SMOKE, one=True, pitch=.7),
    dict(name='Stun', mesh='SK_Grenade_Stun', damage=0, interval=.8, ammo=1, reserve=0, auto=False, left=(0,8,-2), spread=0, speed=1100, payload=ue.ERPayload.STUN, one=True, pitch=.7),
]

def save(asset):
    if not assets.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Save failed: ' + asset.get_path_name())

def create_fire(idle, destination):
    fire = ue.load_asset(destination) or assets.duplicate_asset(idle.get_path_name(), destination)
    raw = ue.AnimPoseEvaluationOptions(); raw.set_editor_property('should_retarget', False)
    pose = ue.AnimPoseExtensions.get_anim_pose_at_time(idle, 0, raw)
    controller = fire.get_editor_property('controller')
    controller.open_bracket('Weapon hip-fire recoil', False)
    try:
        controller.set_frame_rate(ue.FrameRate(numerator=60, denominator=1), False)
        controller.set_number_of_frames(ue.FrameNumber(value=6), False)
        weights = [0, 1, .72, .4, .18, .05, 0]
        for bone in controller.get_model_interface().get_bone_track_names():
            local = ue.AnimPoseExtensions.get_bone_pose(pose, bone, ue.AnimPoseSpaces.LOCAL)
            p = local.translation
            positions = [ue.Vector(p.x, p.y-1.4*w, p.z+.18*w) for w in weights] if str(bone)=='root' else [p]*7
            if not controller.set_bone_track_keys(bone, positions, [local.rotation]*7, [local.scale3d]*7, False):
                raise RuntimeError('Fire track failed: ' + str(bone))
    finally:
        controller.close_bracket(False)
    fire.set_editor_property('enable_root_motion', False)
    save(fire)
    return fire

data_assets = []
report = []
for p in profiles:
    mesh = ue.load_asset(folder + '/' + p['mesh'])
    if not mesh:
        raise RuntimeError('Missing imported mesh ' + p['mesh'])
    key = p['mesh'].removeprefix('SK_')
    idle_path = pose_folder + '/A_' + key + '_Idle'
    runpy.run_path(str(Path(__file__).with_name('create_rifle_pose.py')), init_globals={
        'ER_POSE_SOURCE':'/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS' if p.get('one') else '/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS',
        'ER_POSE_MESH':body, 'ER_POSE_DESTINATION':idle_path,
        'ER_POSE_HEIGHT_OFFSET':15, 'ER_POSE_HAND_SCALE':.5 if p.get('one') else .72,
        'ER_POSE_ARM_THICKNESS':.5 if p.get('one') else .6, 'ER_POSE_MANNY_GRIP_ORIENTATION':True,
        'ER_POSE_LEFT_ELBOW_POLE':(0,30,125), 'ER_POSE_LEFT_GRIP_OFFSET':p['left']})
    idle = ue.load_asset(idle_path)
    fire = create_fire(idle, pose_folder + '/A_' + key + '_Fire')
    data = ue.load_asset(folder + '/DA_' + key)
    if not data:
        factory = ue.DataAssetFactory(); factory.set_editor_property('data_asset_class', ue.ERWeaponData)
        data = tools.create_asset('DA_' + key, folder, ue.ERWeaponData, factory)
    trigger_name = key + '_Trigger'
    trigger = ue.ERCharacter.get_weapon_reference_bone(mesh, trigger_name).translation
    # Gun models point +X; the established hand socket points +Y.
    grip = trigger + ue.Vector(-3,0,-4)
    if p.get('one'):
        grip = ue.Vector(0,0,0)
    if key=='RPG7':
        grip = ue.Vector(-5,0,-9)
    if key=='M1911':
        grip = ue.Vector(-5,0,-12)
    grip_transform = ue.Transform(location=ue.Vector(grip.y,-grip.x,-grip.z),
                                  rotation=ue.Rotator(pitch=0,yaw=90,roll=0), scale=ue.Vector(1,1,1))
    bounds = mesh.get_imported_bounds()
    muzzle = ue.Vector(bounds.origin.x + bounds.box_extent.x, 0, 0)
    settings = dict(display_name=p['name'], mesh=mesh, grip_transform=grip_transform, muzzle_offset=muzzle,
                    view_offset=ue.Vector(*(p.get('view',(18,-3,10) if p.get('one') else (0,0,0)))),
                    idle_animation=idle, fire_animation=fire, damage=p['damage'], shot_interval=p['interval'],
                    magazine_size=p['ammo'], initial_reserve=p['reserve'], automatic=p['auto'],
                    pellets=p.get('pellets',1), payload=p.get('payload',ue.ERPayload.BULLET),
                    one_handed=p.get('one',False), sound_pitch=p['pitch'], reload_duration=p.get('reload',1.5),
                    projectile_speed=p.get('speed',30000), projectile_life=8 if 'payload' in p else 2,
                    standing_spread_degrees=p['spread'], zoom_fov=p.get('zoom',60 if not p.get('one') else 90),
                    explosion_radius=600 if p.get('payload')==ue.ERPayload.STUN else 350,
                    fuse_duration=2.5, bloom_per_shot_degrees=.12 if not p.get('one') else 0)
    for name,value in settings.items():
        data.set_editor_property(name, value)
    data.set_editor_property('sight_mesh',ue.load_asset(folder+'/'+p['sight']) if p.get('sight') else None)
    data.set_editor_property('sight_transform',ue.Transform(location=ue.Vector(p.get('sight_x',0),0,p.get('sight_z',0))))
    save(data)
    data_assets.append(data)
    report.append({'name':p['name'],'mesh':mesh.get_path_name(),'data':data.get_path_name(),
                   'trigger':str(trigger),'grip':str(grip),'muzzle':str(muzzle)})
    ue.log('ER_ARSENAL_DATA ' + json.dumps(report[-1]))

# Own cosmetic materials for grenade smoke and explosion/projectile glow.
for name, color, transparent in [('M_ER_Smoke',(.25,.27,.28),True), ('M_ER_Explosion',(8.,2.,.15),False)]:
    material = ue.load_asset(folder + '/' + name)
    if not material:
        material = tools.create_asset(name, folder, ue.Material, ue.MaterialFactoryNew())
    ue.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property('shading_model', ue.MaterialShadingModel.MSM_UNLIT)
    if transparent:
        material.set_editor_property('blend_mode', ue.BlendMode.BLEND_TRANSLUCENT)
    rgb = ue.MaterialEditingLibrary.create_material_expression(material, ue.MaterialExpressionConstant3Vector, -300,0)
    rgb.set_editor_property('constant', ue.LinearColor(*color,1))
    ue.MaterialEditingLibrary.connect_material_property(rgb,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
    if transparent:
        fresnel = ue.MaterialEditingLibrary.create_material_expression(material,ue.MaterialExpressionFresnel,-500,160)
        fresnel.set_editor_property('exponent',.5)
        invert = ue.MaterialEditingLibrary.create_material_expression(material,ue.MaterialExpressionOneMinus,-300,160)
        ue.MaterialEditingLibrary.connect_material_expressions(fresnel,'',invert,'')
        ue.MaterialEditingLibrary.connect_material_property(invert,'',ue.MaterialProperty.MP_OPACITY)
    ue.MaterialEditingLibrary.recompile_material(material); save(material)

for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget']:
    bp = ue.load_asset(path); defaults=ue.get_default_object(bp.generated_class())
    defaults.set_editor_property('arsenal',data_assets)
    ue.BlueprintEditorLibrary.compile_blueprint(bp); save(bp)
bp=ue.load_asset('/Game/EclipseRecoil/Core/BP_ERPlayerController')
ue.get_default_object(bp.generated_class()).set_editor_property('primary_options',data_assets[:7])
ue.BlueprintEditorLibrary.compile_blueprint(bp); save(bp)
bp=ue.load_asset('/Game/EclipseRecoil/Weapons/BP_ERProjectile')
defaults=ue.get_default_object(bp.generated_class())
defaults.set_editor_property('effect_mesh',ue.load_asset('/Engine/BasicShapes/Sphere'))
defaults.set_editor_property('smoke_material',ue.load_asset(folder+'/M_ER_Smoke'))
defaults.set_editor_property('explosion_material',ue.load_asset(folder+'/M_ER_Explosion'))
ue.BlueprintEditorLibrary.compile_blueprint(bp); save(bp)
(Path(ue.Paths.project_dir())/'SourceAssets/LowPolyWeapons/arsenal-report.json').write_text(json.dumps(report,indent=2)+'\n')
ue.log('ER_ARSENAL_READY primaries=7 sidearm=1 utilities=3 project_local=1')
