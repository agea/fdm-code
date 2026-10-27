"""Seat the knife inside the fist and pivot its FPS forearm below the viewport.

Only owned animation keys and the FPS attachment change. Keep the attack's
timing/anchor trajectory while fitting a smaller knife inside the fist.
"""
from pathlib import Path
import math
import json
import runpy
import unreal as ue


def fit(settings):
    h = runpy.run_path(str(Path(__file__).with_name('retarget_weapon_hands.py')))
    xyz, quat, rotate = h['xyz'], h['quat'], h['rotate']
    add, sub, mul = h['add'], h['sub'], h['mul']
    qmul, inverse = h['qmul'], h['inverse']
    skin = ue.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/FirstPerson/SK_ER_Twinblast_Alpha_Officer_Forearms')
    defaults = ue.get_default_object(ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter').generated_class())
    body = ue.Transform(location=defaults.first_person_body.get_editor_property('relative_location')+settings['view_offset'],
                        rotation=ue.Rotator(pitch=0,yaw=-90,roll=0))
    options = ue.AnimPoseEvaluationOptions()
    options.should_retarget = False
    options.optional_skeletal_mesh = skin
    options.evaluation_type = ue.AnimDataEvalType.RAW
    space = ue.AnimPoseSpaces.WORLD
    local = ue.AnimPoseSpaces.LOCAL
    bone = lambda p,n:ue.AnimPoseExtensions.get_bone_pose(p,n,space)
    socket = lambda p,n:ue.AnimPoseExtensions.get_socket_pose(p,n,space)
    put = lambda p,n,t:ue.AnimPoseExtensions.set_bone_pose(p,t,n,space)
    original_attachment = ue.Transform(rotation=ue.Rotator(pitch=0,yaw=90,roll=0))
    root = Path(ue.Paths.project_dir())/'Assets/weapons/SourceAssets/SketchfabWeapons'
    source = json.loads(next(root.glob('f3140*/derived/knife-source-grip.json')).read_text())
    source_frame = quat(ue.MathLibrary.make_rot_from_xy(
        ue.Vector(*sub(source['fingers']['middle'][0],source['wrist'])),
        ue.Vector(*sub(source['fingers']['index'][0],source['fingers']['pinky'][0]))).quaternion())
    finger_fit = runpy.run_path(str(Path(__file__).with_name('fit_knife_fingers.py')))['fit']
    finger_keys = None
    finger_receipt = None
    corrected_attachment = None
    knife_scale = .65
    upward_tilt = None
    max_error = max_wrist_error = max_length_error = 0.
    exit_uv = []
    for clip in (settings['idle_animation'],settings['fire_animation']):
        controller = clip.get_editor_property('controller')
        model = controller.get_model_interface()
        names = [str(n) for n in model.get_bone_track_names()]
        tracks = {n:[] for n in names}
        for i in range(model.get_number_of_frames()+1):
            pose = ue.AnimPoseExtensions.get_anim_pose_at_time(clip,i/30,options)
            old_weapon = ue.MathLibrary.compose_transforms(original_attachment,socket(pose,'HandGrip_R'))
            # This component has absolute scale: shrink the knife, not the hand.
            old_weapon.set_editor_property('scale3d',ue.Vector(knife_scale,knife_scale,knife_scale))
            if upward_tilt is None:
                axis = rotate(inverse(quat(body.rotation)),(0,-1,0))
                axis = rotate(inverse(quat(old_weapon.rotation)),axis)
                upward_tilt = (*mul(axis,math.sin(math.radians(4))),math.cos(math.radians(4)))
            old_hand = bone(pose,'hand_r')
            hand_position = xyz(old_hand.translation)
            # Align wrist/knuckles and index/pinky to the original closed fist,
            # including its cant along the authored handle's -Y centreline.
            original_frame = quat(ue.MathLibrary.make_rot_from_xy(
                bone(pose,'middle_01_r').translation-old_hand.translation,
                bone(pose,'index_01_r').translation-bone(pose,'pinky_01_r').translation).quaternion())
            rotation = qmul(qmul(qmul(quat(old_weapon.rotation),upward_tilt),source_frame),inverse(original_frame))
            new_rotation = qmul(rotation,quat(old_hand.rotation))
            # Raise/advance the middle-knuckle station toward the guard. The
            # finger solver then adapts the source closure to this smaller rig.
            knuckle = xyz(bone(pose,'middle_01_r').translation)
            knuckle = add(hand_position,rotate(rotation,sub(knuckle,hand_position)))
            # Follow the marked arrow along the handle toward its guard:
            # 1.8 source cm at scale .65 = 1.17 visible world cm.
            target = xyz(ue.MathLibrary.transform_location(old_weapon,ue.Vector(1.5,-4.8,-1.4)))
            hand_position = add(hand_position,sub(target,knuckle))
            pose = put(pose,'hand_r',h['transform'](hand_position,new_rotation,xyz(old_hand.scale3d)))
            # Swing the complete forearm about this seated wrist. Its endpoint
            # lies on the bottom-centre ray below the visible viewport.
            upper, lower = bone(pose,'upperarm_r'), bone(pose,'lowerarm_r')
            old_wrist = xyz(bone(pose,'hand_r').translation)
            elbow = xyz(lower.translation)
            length = h['length'](sub(xyz(old_hand.translation),elbow))
            camera_wrist = xyz(ue.MathLibrary.transform_location(body,ue.Vector(*old_wrist)))
            ray = (1.,0.,-.8)
            a = h['dot'](ray,ray)
            b = -2*h['dot'](ray,camera_wrist)
            c = h['dot'](camera_wrist,camera_wrist)-length*length
            discriminant = b*b-4*a*c
            if discriminant < 0:
                raise RuntimeError('Knife forearm cannot reach the bottom-centre exit ray')
            depth = (-b+math.sqrt(discriminant))/(2*a)
            camera_elbow = mul(ray,depth)
            new_elbow = xyz(ue.MathLibrary.inverse_transform_location(body,ue.Vector(*camera_elbow)))
            # Rotate the hidden shoulder with the forearm; retain both lengths.
            pivot = h['from_to'](sub(elbow,xyz(old_hand.translation)),sub(new_elbow,old_wrist))
            # User requested exactly halfway between the original lateral arm
            # and revision2's bottom-centre swing: halve the rotation angle.
            pivot = h['unit']((*pivot[:3],1+pivot[3]))
            new_elbow = add(old_wrist,rotate(pivot,sub(elbow,xyz(old_hand.translation))))
            new_shoulder = add(new_elbow,rotate(pivot,sub(xyz(upper.translation),elbow)))
            pose = put(pose,'upperarm_r',h['transform'](new_shoulder,qmul(pivot,quat(upper.rotation)),xyz(upper.scale3d)))
            pose = put(pose,'lowerarm_r',h['transform'](new_elbow,qmul(pivot,quat(lower.rotation)),xyz(lower.scale3d)))
            pose = put(pose,'hand_r',h['transform'](old_wrist,new_rotation,xyz(old_hand.scale3d)))
            max_wrist_error = max(max_wrist_error,h['length'](sub(xyz(bone(pose,'hand_r').translation),old_wrist)))
            max_length_error = max(max_length_error,abs(h['length'](sub(old_wrist,new_elbow))-length))
            if finger_keys is None:
                mesh = settings.get('mesh') or ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/Knife/DA_Knife').mesh
                pose,finger_receipt = finger_fit(pose,old_weapon,skin,mesh,settings['weapon_idle_animation'],source)
                finger_keys = {name:ue.AnimPoseExtensions.get_bone_pose(pose,name,local) for name in names
                               if any(name.startswith(f+'_') for f in ('index','middle','ring','pinky','thumb')) and name.endswith('_r')}
            else:
                for name,key in finger_keys.items():
                    pose = ue.AnimPoseExtensions.set_bone_pose(pose,key,name,local)
            parent = socket(pose,'ER_Palm_R')
            relative = ue.MathLibrary.make_relative_transform(old_weapon,parent)
            relative.set_editor_property('scale3d',ue.Vector(knife_scale,knife_scale,knife_scale))
            if corrected_attachment is None:
                corrected_attachment = relative
            actual = ue.MathLibrary.compose_transforms(corrected_attachment,parent)
            actual.set_editor_property('scale3d',ue.Vector(knife_scale,knife_scale,knife_scale))
            max_error = max(max_error,(actual.translation-old_weapon.translation).length())
            if abs(h['dot'](quat(actual.rotation),quat(old_weapon.rotation))) < .99999:
                raise RuntimeError('Knife grip changed the approved blade orientation')
            camera_elbow = xyz(ue.MathLibrary.transform_location(body,ue.Vector(*new_elbow)))
            exit_uv.append([.5+camera_elbow[1]/camera_elbow[0]/2,.5-camera_elbow[2]/camera_elbow[0]*16/9/2])
            for name in names:
                tracks[name].append(ue.AnimPoseExtensions.get_bone_pose(pose,name,local))
        if max_error>.01 or max_wrist_error>.001 or max_length_error>.001:
            raise RuntimeError('Knife grip/pivot invariant failed')
        controller.open_bracket('Seat knife fist and pivot FPS forearm',False)
        try:
            for name,keys in tracks.items():
                if not controller.set_bone_track_keys(name,[k.translation for k in keys],[k.rotation for k in keys],[k.scale3d for k in keys],False):
                    raise RuntimeError('Cannot save knife grip track '+name)
        finally:
            controller.close_bracket(False)
        if not ue.EditorAssetLibrary.save_loaded_asset(clip,False):
            raise RuntimeError('Cannot save corrected knife pose')
    settings['first_person_palm_grip'] = True
    settings['first_person_grip_transform'] = corrected_attachment
    return dict(source_grip_frame=107,forearm_rotation_blend=.5,knife_scale=knife_scale,hand_upward_tilt_degrees=8,
                hand_advance_toward_guard_cm=1.17,
                knife_transform_error_cm=max_error,
                forearm_pivot_error_cm=max_wrist_error,segment_length_error_cm=max_length_error,
                forearm_endpoint_uv=exit_uv[0],fingers=finger_receipt)
