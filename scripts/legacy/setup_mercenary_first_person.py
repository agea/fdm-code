"""Use the owned mercenary for FPS arms without modifying vendor assets."""
import math
import runpy
from pathlib import Path
import unreal as ue

assets = ue.EditorAssetLibrary
mesh = ue.load_asset('/Game/EclipseRecoil/Characters/Mercenary/SK_ER_Mercenary_Target')
target = ue.load_asset('/Game/EclipseRecoil/Core/BP_ERTrainingTarget')
player = ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter')
idle = ue.load_asset('/Game/EclipseRecoil/Characters/A_ER_Rifle_HipPose')
fire = ue.load_asset('/Game/EclipseRecoil/Characters/A_ER_Rifle_Fire')
if not all([mesh, target, player, idle, fire]):
    raise RuntimeError('Generate mercenary and rifle feedback assets first')
folder = '/Game/EclipseRecoil/Characters/Mercenary'
idle_path = folder + '/A_ER_Mercenary_Rifle_HipPose'
runpy.run_path(str(Path(__file__).with_name('create_rifle_pose.py')),
               init_globals={'ER_POSE_MESH': mesh, 'ER_POSE_DESTINATION': idle_path,
                             'ER_POSE_HEIGHT_OFFSET': 15, 'ER_POSE_HAND_SCALE': .72,
                             'ER_POSE_ARM_THICKNESS': .6,
                             'ER_POSE_MANNY_GRIP_ORIENTATION': True,
                             'ER_POSE_LEFT_ELBOW_POLE': (0, 30, 125)})
idle = ue.load_asset(idle_path)
fire_path = folder + '/A_ER_Mercenary_Rifle_Fire'
fire = ue.load_asset(fire_path) or assets.duplicate_asset(idle_path, fire_path)
if not ue.ERCharacter.set_mercenary_animation_skeleton(fire, mesh):
    raise RuntimeError('Cannot assign firing animation skeleton')
raw_options = ue.AnimPoseEvaluationOptions()
raw_options.set_editor_property('should_retarget', False)
raw_pose = ue.AnimPoseExtensions.get_anim_pose_at_time(idle, 0, raw_options)
controller = fire.get_editor_property('controller')
controller.open_bracket('Mercenary FPS recoil with aligned grips', False)
try:
    controller.set_frame_rate(ue.FrameRate(numerator=60, denominator=1), False)
    controller.set_number_of_frames(ue.FrameNumber(value=6), False)
    for name in controller.get_model_interface().get_bone_track_names():
        local = ue.AnimPoseExtensions.get_bone_pose(raw_pose, name, ue.AnimPoseSpaces.LOCAL)
        positions = [local.translation] * 7
        if str(name) == 'root':
            p = local.translation
            positions = [ue.Vector(p.x, p.y - 1.4 * w, p.z + .18 * w)
                         for w in [0, 1, .72, .4, .18, .05, 0]]
        if not controller.set_bone_track_keys(name, positions, [local.rotation] * 7, [local.scale3d] * 7, False):
            raise RuntimeError('Cannot write firing track ' + str(name))
finally:
    controller.close_bracket(False)
fire.set_editor_property('enable_root_motion', False)
fire.set_editor_property('force_root_lock', False)
if not assets.save_loaded_asset(fire, only_if_is_dirty=False):
    raise RuntimeError('Cannot save mercenary firing animation')
slots = [i for i, s in enumerate(mesh.get_editor_property('materials'))
         if str(s.material_slot_name) in ['LowerArm', 'Hand', 'M_MER_ME_Gloves01']]
if len(slots) != 3:
    raise RuntimeError('Unexpected arm material sections: ' + str(slots))
palettes = ue.get_default_object(target.generated_class())
alpha = list(palettes.get_editor_property('alpha_body_materials'))
omega = list(palettes.get_editor_property('omega_body_materials'))
if len(alpha) != len(mesh.get_editor_property('materials')) or len(omega) != len(alpha):
    raise RuntimeError('Missing team palettes')
glove_slot = next(i for i, s in enumerate(mesh.get_editor_property('materials'))
                  if str(s.material_slot_name) == 'M_MER_ME_Gloves01')
for team, palette, colour in [('Alpha', alpha, ue.LinearColor(.025, .24, .70, 1)),
                              ('Omega', omega, ue.LinearColor(.75, .085, .012, 1))]:
    path = folder + '/MI_ER_' + team + '_FPS_Gloves'
    material = ue.load_asset(path) or assets.duplicate_asset(palette[glove_slot].get_path_name(), path)
    ue.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material, 'TintColor_01', colour)
    ue.MaterialEditingLibrary.update_material_instance(material)
    if not assets.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError('Cannot save FPS glove palette')
    palette[glove_slot] = material
for bp in [player, target]:
    defaults = ue.get_default_object(bp.generated_class())
    body = defaults.get_editor_property('first_person_body')
    body.set_skeletal_mesh_asset(mesh)
    body.set_editor_property('relative_scale3d', ue.Vector(1, 1, 1))
    # Raise the authored grips to stay within this rig's reach, then compensate
    # the whole viewmodel so the rifle keeps its previous camera-space position.
    # Align the loaded/retargeted pose with the measured mannequin rifle frame,
    # rather than assuming the authored IK frame survives retargeting unchanged.
    body.set_editor_property('relative_location', ue.Vector(-11.2263, 1.4908, -159.9167))
    body.set_editor_property('relative_rotation', ue.Rotator(pitch=0, yaw=-90, roll=0))
    body.set_editor_property('animation_mode', ue.AnimationMode.ANIMATION_SINGLE_NODE)
    anim = body.get_editor_property('animation_data')
    anim.set_editor_property('anim_to_play', idle)
    anim.set_editor_property('saved_looping', True)
    body.set_editor_property('animation_data', anim)
    # The rifle keeps its authored size even though hand bones are smaller.
    rifle = defaults.get_editor_property('first_person_rifle')
    rifle.set_editor_property('absolute_scale', True)
    rifle.set_editor_property('relative_scale3d', ue.Vector(1, 1, 1))
    defaults.set_editor_property('first_person_fire_animation', fire)
    defaults.set_editor_property('first_person_visible_material_slots', slots)
    defaults.set_editor_property('alpha_first_person_materials', alpha)
    defaults.set_editor_property('omega_first_person_materials', omega)
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    if not assets.save_loaded_asset(bp, only_if_is_dirty=False):
        raise RuntimeError('Cannot save ' + bp.get_path_name())
    ue.log('ER_FPS_ASSET_READY class=%s mesh=%s visible=%s idle=%s fire=%s' %
           (bp.generated_class().get_path_name(), mesh.get_path_name(), slots, idle.get_path_name(), fire.get_path_name()))

# Evaluate the actual viewmodel proportions, including its mesh grip sockets.
options = ue.AnimPoseEvaluationOptions()
options.set_editor_property('optional_skeletal_mesh', mesh)
def grip_span(sequence, time):
    pose = ue.AnimPoseExtensions.get_anim_pose_at_time(sequence, time, options)
    right = ue.AnimPoseExtensions.get_socket_pose(pose, 'HandGrip_R', ue.AnimPoseSpaces.WORLD)
    left = ue.AnimPoseExtensions.get_socket_pose(pose, 'HandGrip_L', ue.AnimPoseSpaces.WORLD)
    # The rifle uses absolute scale: compare in its rigid frame, without the
    # smaller hand/socket scale inherited from the FPS pose.
    rigid_right = ue.Transform()
    rigid_right.set_editor_property('rotation', right.rotation)
    rigid_right.set_editor_property('translation', right.translation)
    relative = rigid_right.inverse().transform_location(left.translation)
    return relative, right.translation
base_span, base_right = grip_span(idle, 0)
foregrip_error = math.dist((base_span.x, base_span.y, base_span.z), (0, 28.522, 3.003))
ue.log('ER_FPS_FOREGRIP span=%.4f,%.4f,%.4f error_cm=%.4f' % (base_span.x, base_span.y, base_span.z, foregrip_error))
if foregrip_error > .1:
    raise RuntimeError('Mercenary left hand misses the rifle foregrip')
for time in [0, 1 / 60, .05, .1]:
    span, right = grip_span(fire, time)
    error = math.dist((base_span.x, base_span.y, base_span.z), (span.x, span.y, span.z))
    recoil = math.dist((base_right.x, base_right.y, base_right.z), (right.x, right.y, right.z))
    ue.log('ER_FPS_GRIPS t=%.4f span=%.3f,%.3f,%.3f error=%.4f recoil=%.4f' % (time, span.x, span.y, span.z, error, recoil))
    if error > .1 or (time in [0, .1] and recoil > .1):
        raise RuntimeError('Mercenary grips drift during the firing cycle')
