"""Build the hip-pose firing animation and assign presentation assets via Unreal.

Does not touch template animations, input, gameplay data or map geometry.
"""
import math
import unreal as ue

assets = ue.EditorAssetLibrary
tools = ue.AssetToolsHelpers.get_asset_tools()


def load(path):
    asset = ue.load_asset(path)
    if not asset:
        raise RuntimeError('Missing asset: ' + path)
    return asset


def save(asset):
    if not assets.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Cannot save ' + asset.get_path_name())


source = load('/Game/EclipseRecoil/Characters/A_ER_Rifle_HipPose')
destination = '/Game/EclipseRecoil/Characters/A_ER_Rifle_Fire'
animation = ue.load_asset(destination) or assets.duplicate_asset(source.get_path_name(), destination)
pose = ue.AnimPoseExtensions.get_anim_pose_at_time(source, 0, ue.AnimPoseEvaluationOptions())
raw_options = ue.AnimPoseEvaluationOptions()
raw_options.set_editor_property('should_retarget', False)
raw_pose = ue.AnimPoseExtensions.get_anim_pose_at_time(source, 0, raw_options)
controller = animation.get_editor_property('controller')
controller.open_bracket('Hip-fire recoil with both grips preserved', False)
try:
    controller.set_frame_rate(ue.FrameRate(numerator=60, denominator=1), False)
    controller.set_number_of_frames(ue.FrameNumber(value=6), False)
    # 100 ms cycle: fast 1.4 cm kick, followed by a smooth return to the exact idle pose.
    weights = [0., 1., .72, .40, .18, .05, 0.]
    for name in controller.get_model_interface().get_bone_track_names():
        # Copy raw authored tracks, not the evaluated/retargeted pose: writing
        # evaluated transforms back would apply retargeting twice and shift hands.
        local = ue.AnimPoseExtensions.get_bone_pose(raw_pose, name, ue.AnimPoseSpaces.LOCAL)
        position, rotation, scale = local.translation, local.rotation, local.scale3d
        positions = [position] * 7
        rotations = [rotation] * 7
        if str(name) == 'root':
            positions = [ue.Vector(position.x, position.y - 1.4 * w,
                                   position.z + .18 * w) for w in weights]
        if not controller.set_bone_track_keys(name, positions, rotations, [scale] * 7, False):
            raise RuntimeError('Cannot write firing track ' + str(name))
finally:
    controller.close_bracket(False)
animation.set_editor_property('enable_root_motion', False)
animation.set_editor_property('force_root_lock', False)
save(animation)
# Verify both grip sockets move together and recover the exact source pose.
source_grips = [ue.AnimPoseExtensions.get_socket_pose(pose, n, ue.AnimPoseSpaces.WORLD)
                for n in ['HandGrip_R', 'HandGrip_L']]
for t in [0., 1./60, .05, .1]:
    sample = ue.AnimPoseExtensions.get_anim_pose_at_time(animation, t, ue.AnimPoseEvaluationOptions())
    grips = [ue.AnimPoseExtensions.get_socket_pose(sample, n, ue.AnimPoseSpaces.WORLD)
             for n in ['HandGrip_R', 'HandGrip_L']]
    delta = lambda a, b: (a.x-b.x, a.y-b.y, a.z-b.z)
    source_span = delta(source_grips[1].translation, source_grips[0].translation)
    shot_span = delta(grips[1].translation, grips[0].translation)
    span_error = math.dist(source_span, shot_span)
    recoil = math.dist(delta(grips[0].translation, source_grips[0].translation), (0, 0, 0))
    ue.log('ER_FIRE_POSE_SAMPLE t=%.4f span_error_cm=%.4f displacement_cm=%.4f' % (t, span_error, recoil))
    if span_error > .1:
        raise RuntimeError('Firing animation separates the hands: %.4f cm' % span_error)
    if t in [0., .1] and recoil > .1:
        raise RuntimeError('Firing animation does not recover the idle pose')
    if t == 1./60 and recoil < 1.:
        raise RuntimeError('Firing animation has no recoil motion')
ue.log('ER_FIRE_ANIMATION_VERIFIED: 0.1 s, aligned grips, idle endpoints')

folder = '/Game/EclipseRecoil/Weapons'
material_path = folder + '/M_ER_MuzzleFlash'
material = ue.load_asset(material_path) or tools.create_asset('M_ER_MuzzleFlash', folder, ue.Material, ue.MaterialFactoryNew())
# This script owns this generated material; rebuild its three-node graph on rerun.
ue.MaterialEditingLibrary.delete_all_material_expressions(material)
material.set_editor_property('shading_model', ue.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('blend_mode', ue.BlendMode.BLEND_ADDITIVE)
color = ue.MaterialEditingLibrary.create_material_expression(material, ue.MaterialExpressionConstant3Vector, -400, 0)
color.set_editor_property('constant', ue.LinearColor(12., 4., .5, 1.))
if not ue.MaterialEditingLibrary.connect_material_property(color, '', ue.MaterialProperty.MP_EMISSIVE_COLOR):
    raise RuntimeError('Cannot connect muzzle emission')
# Fade the sphere edges; the small elongated glow reads as a muzzle flash.
fresnel = ue.MaterialEditingLibrary.create_material_expression(material, ue.MaterialExpressionFresnel, -400, 160)
fresnel.set_editor_property('exponent', 2.)
invert = ue.MaterialEditingLibrary.create_material_expression(material, ue.MaterialExpressionOneMinus, -180, 160)
if not ue.MaterialEditingLibrary.connect_material_expressions(fresnel, '', invert, ''):
    raise RuntimeError('Cannot connect muzzle edge fade')
if not ue.MaterialEditingLibrary.connect_material_property(invert, '', ue.MaterialProperty.MP_OPACITY):
    raise RuntimeError('Cannot connect muzzle opacity')
ue.MaterialEditingLibrary.recompile_material(material)
save(material)

attenuation_path = folder + '/SA_ER_Rifle'
attenuation = ue.load_asset(attenuation_path) or tools.create_asset('SA_ER_Rifle', folder, ue.SoundAttenuation, ue.SoundAttenuationFactory())
settings = attenuation.get_editor_property('attenuation')
settings.set_editor_property('attenuate', True)
settings.set_editor_property('spatialize', True)
settings.set_editor_property('attenuation_shape_extents', ue.Vector(300, 0, 0))
settings.set_editor_property('falloff_distance', 3700.)
attenuation.set_editor_property('attenuation', settings)
save(attenuation)

blueprint = load('/Game/EclipseRecoil/Core/BP_ERCharacter')
defaults = ue.get_default_object(blueprint.generated_class())
defaults.set_editor_property('first_person_fire_animation', animation)
defaults.set_editor_property('fire_sound', load('/Game/Weapons/GrenadeLauncher/Audio/FirstPersonTemplateWeaponFire02'))
defaults.set_editor_property('fire_attenuation', attenuation)
defaults.set_editor_property('muzzle_flash_mesh', load('/Engine/BasicShapes/Sphere'))
defaults.set_editor_property('muzzle_flash_material', material)
save(blueprint)
ue.log('ER_SHOT_ASSETS_SAVED: BP defaults, template sound, spatial attenuation, additive muzzle flash')
