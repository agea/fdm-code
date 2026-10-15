"""Create a project-owned static hip-fire pose from the template animation.

Run inside Unreal Editor. Original template assets stay untouched. This is a
prototype pose; an animation blueprint can replace it when locomotion is added.
"""
import math
import unreal as ue

SOURCE = globals().get('ER_POSE_SOURCE','/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS')
DESTINATION = globals().get('ER_POSE_DESTINATION', '/Game/EclipseRecoil/Characters/A_ER_Rifle_HipPose')


def xyz(v):
    return (v.x, v.y, v.z)


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def scale(a, s):
    return tuple(x * s for x in a)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def unit(a):
    return scale(a, 1 / math.sqrt(dot(a, a)))


def quat(q):
    return (q.x, q.y, q.z, q.w)


def multiply(a, b):
    av, bv = a[:3], b[:3]
    return (*add(add(scale(bv, a[3]), scale(av, b[3])), cross(av, bv)), a[3]*b[3]-dot(av, bv))


def inverse(q):
    return (-q[0], -q[1], -q[2], q[3])


def rotate(q, v):
    return multiply(multiply(q, (*v, 0)), inverse(q))[:3]


def from_to(a, b):
    a, b = unit(a), unit(b)
    return unit((*cross(a, b), 1 + dot(a, b)))


def transform(position, rotation, bone_scale=(1, 1, 1)):
    result = ue.Transform()
    result.set_editor_property('translation', ue.Vector(*position))
    result.set_editor_property('rotation', ue.Quat(x=rotation[0], y=rotation[1], z=rotation[2], w=rotation[3]))
    result.set_editor_property('scale3d', ue.Vector(*bone_scale))
    return result


source = ue.load_asset(SOURCE)
if not source:
    raise RuntimeError('Missing template rifle idle')
ue.EditorAssetLibrary.make_directory('/Game/EclipseRecoil/Characters')
asset = ue.load_asset(DESTINATION) or ue.EditorAssetLibrary.duplicate_asset(SOURCE, DESTINATION)
evaluation = ue.AnimPoseEvaluationOptions()
target_mesh = globals().get('ER_POSE_MESH')
if target_mesh:
    if not ue.ERCharacter.set_mercenary_animation_skeleton(asset, target_mesh):
        raise RuntimeError('Cannot assign owned FPS animation skeleton')
    evaluation.set_editor_property('optional_skeletal_mesh', target_mesh)
pose = ue.AnimPoseExtensions.get_anim_pose_at_time(source, 0, evaluation)
pose_bones = set(str(n) for n in ue.AnimPoseExtensions.get_bone_names(pose))
# Presentation-only hand proportions; descendants (fingers) follow the same
# scale. Apply before IK so the solver accounts for the scaled socket offset.
hand_scale = globals().get('ER_POSE_HAND_SCALE', 1.0)
arm_thickness = globals().get('ER_POSE_ARM_THICKNESS', 1.0)
if arm_thickness != 1:
    for side in ['l', 'r']:
        name = 'upperarm_' + side
        local = ue.AnimPoseExtensions.get_bone_pose(pose, name, ue.AnimPoseSpaces.LOCAL)
        local.set_editor_property('scale3d', ue.Vector(1, arm_thickness, arm_thickness))
        pose = ue.AnimPoseExtensions.set_bone_pose(pose, local, name, ue.AnimPoseSpaces.LOCAL)
if hand_scale != 1:
    for side in ['l', 'r']:
        name = 'hand_' + side
        local = ue.AnimPoseExtensions.get_bone_pose(pose, name, ue.AnimPoseSpaces.LOCAL)
        # Cancel the arm's transverse scale so hand size stays independent.
        local.set_editor_property('scale3d', ue.Vector(hand_scale, hand_scale / arm_thickness, hand_scale / arm_thickness))
        pose = ue.AnimPoseExtensions.set_bone_pose(pose, local, name, ue.AnimPoseSpaces.LOCAL)
get_bone = lambda name: ue.AnimPoseExtensions.get_bone_pose(pose, name, ue.AnimPoseSpaces.WORLD)
get_socket = lambda name: ue.AnimPoseExtensions.get_socket_pose(pose, name, ue.AnimPoseSpaces.WORLD)
original_right_grip = quat(get_socket('HandGrip_R').rotation)
original_left_grip = quat(get_socket('HandGrip_L').rotation)
if globals().get('ER_POSE_MANNY_GRIP_ORIENTATION', False):
    reference_pose = ue.AnimPoseExtensions.get_anim_pose_at_time(source, 0, ue.AnimPoseEvaluationOptions())
    original_right_grip = quat(ue.AnimPoseExtensions.get_socket_pose(reference_pose, 'HandGrip_R', ue.AnimPoseSpaces.WORLD).rotation)
    original_left_grip = quat(ue.AnimPoseExtensions.get_socket_pose(reference_pose, 'HandGrip_L', ue.AnimPoseSpaces.WORLD).rotation)
# GripPoint at weapon origin, GripPoint_002 on the foregrip (template mesh).
grips = {
    'r': ((-10, 25, 125), (0, 0, 0, 1), (-35, 10, 115)),
    'l': ((-10, 53.522, 128.003), multiply(inverse(original_right_grip), original_left_grip), (35, 25, 115)),
}
if 'ER_POSE_RIGHT_GRIP' in globals():grips['r']=globals()['ER_POSE_RIGHT_GRIP']
if 'ER_POSE_LEFT_GRIP_OFFSET' in globals():
    offset = globals()['ER_POSE_LEFT_GRIP_OFFSET']
    p, r, pole = grips['l']
    grips['l'] = (add(grips['r'][0], offset), r, pole)
if 'ER_POSE_LEFT_ELBOW_POLE' in globals():
    p, r, _ = grips['l']
    grips['l'] = (p, r, globals()['ER_POSE_LEFT_ELBOW_POLE'])
height_offset = globals().get('ER_POSE_HEIGHT_OFFSET', 0)
if height_offset:
    grips = {side: ((p[0], p[1], p[2] + height_offset), r,
                    (pole[0], pole[1], pole[2] + height_offset))
             for side, (p, r, pole) in grips.items()}
for side, (grip_position, grip_rotation, pole) in grips.items():
    names = ['upperarm_' + side, 'lowerarm_' + side, 'hand_' + side]
    bones = [get_bone(name) for name in names]
    shoulder, elbow, hand = [xyz(b.translation) for b in bones]
    hand_rotation = quat(bones[2].rotation)
    socket_name=globals().get('ER_POSE_RIGHT_SOCKET','HandGrip_R') if side=='r' else 'HandGrip_L'
    socket = get_socket(socket_name)
    socket_local_position = rotate(inverse(hand_rotation), sub(xyz(socket.translation), hand))
    socket_local_rotation = multiply(inverse(hand_rotation), quat(socket.rotation))
    target_rotation = multiply(grip_rotation, inverse(socket_local_rotation))
    target = sub(grip_position, rotate(target_rotation, socket_local_position))
    upper_length = math.dist(shoulder, elbow)
    lower_length = math.dist(elbow, hand)
    source_upper = sub(elbow, shoulder)
    if side=='r' and 'ER_POSE_RIGHT_ELBOW_TARGET' in globals():
        # FPS contains forearms only. Move the hidden shoulder to preserve both
        # segment lengths while choosing where the visible arm enters the view.
        preferred = add(globals()['ER_POSE_RIGHT_ELBOW_TARGET'], (0,0,height_offset))
        target_elbow = add(target, scale(unit(sub(preferred,target)),lower_length))
        shoulder = sub(target_elbow,scale(unit(source_upper),upper_length))
    else:
        distance = math.dist(shoulder, target)
        if distance >= upper_length + lower_length or distance <= abs(upper_length - lower_length):
            raise RuntimeError('Unreachable %s hand target: shoulder=%s target=%s lengths=%.3f/%.3f distance=%.3f' % (side, shoulder, target, upper_length, lower_length, distance))
        direction = unit(sub(target, shoulder))
        pole_direction = sub(pole, shoulder)
        bend = unit(sub(pole_direction, scale(direction, dot(pole_direction, direction))))
        along = (upper_length**2 - lower_length**2 + distance**2) / (2 * distance)
        height = math.sqrt(max(0, upper_length**2 - along**2))
        target_elbow = add(add(shoulder, scale(direction, along)), scale(bend, height))
    if target_mesh:
        ue.log('ER_FPS_ARM side=%s elbow=%s wrist=%s hand_scale=%.2f' % (side, target_elbow, target, hand_scale))
    upper_rotation = multiply(from_to(source_upper, sub(target_elbow, shoulder)), quat(bones[0].rotation))
    lower_rotation = multiply(from_to(sub(hand, elbow), sub(target, target_elbow)), quat(bones[1].rotation))
    for name, position, rotation in zip(names, [shoulder, target_elbow, target], [upper_rotation, lower_rotation, target_rotation]):
        bone_scale = xyz(get_bone(name).scale3d)
        pose = ue.AnimPoseExtensions.set_bone_pose(pose, transform(position, rotation, bone_scale), name, ue.AnimPoseSpaces.WORLD)
    actual = xyz(get_socket(socket_name).translation)
    if math.dist(actual, grip_position) > .01:
        raise RuntimeError('Hand grip alignment failed: ' + str(actual))

if 'ER_POSE_FINALIZE' in globals():
    pose=globals()['ER_POSE_FINALIZE'](pose,target_mesh)
controller = asset.get_editor_property('controller')
model = controller.get_model_interface()
key_count = model.get_number_of_frames() + 1
controller.open_bracket('Respawn & Rage prototype hip-fire pose', False)
try:
    for name in model.get_bone_track_names():
        if str(name) not in pose_bones:
            continue
        local = ue.AnimPoseExtensions.get_bone_pose(pose, name, ue.AnimPoseSpaces.LOCAL)
        if not controller.set_bone_track_keys(name, [local.translation]*key_count, [local.rotation]*key_count, [local.scale3d]*key_count, False):
            raise RuntimeError('Cannot write bone ' + str(name))
finally:
    controller.close_bracket(False)
if not ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
    raise RuntimeError('Cannot save hip-fire pose')
ue.log('ER_HIP_POSE_SAVED: both grip sockets aligned; arm segment lengths preserved')
