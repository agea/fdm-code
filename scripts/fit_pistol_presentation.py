"""Fit the owned pistol hand and rear view to the 17 October grip references."""
from pathlib import Path
import json
import math
import runpy
import unreal as ue

maths=runpy.run_path(str(Path(__file__).with_name('retarget_weapon_hands.py')))
palm_tools=runpy.run_path(str(Path(__file__).with_name('setup_palm_contacts.py')))
xyz=maths['xyz'];add=maths['add'];sub=maths['sub'];mul=maths['mul'];length=maths['length'];unit=maths['unit']
quat=maths['quat'];qmul=maths['qmul'];inverse=maths['inverse'];rotate=maths['rotate'];from_to=maths['from_to']

HAND_SCALE=.62
# Seat the knuckles against the grip flank instead of fitting only one palm socket.
HAND_SEAT=(-.8,-3.4,-.7)
MUZZLE_CAMERA=(56.,56.*.164,56.*-.1603125)
CENTER_AIM_DEPTH=350.

def camera_rotation():
    # A finite convergence point reproduces the reference's visible barrel
    # perspective while the entire barrel axis still projects through the reticle.
    x=CENTER_AIM_DEPTH-MUZZLE_CAMERA[0];y=-MUZZLE_CAMERA[1];z=-MUZZLE_CAMERA[2]
    return ue.Rotator(pitch=math.degrees(math.atan2(z,math.hypot(x,y))),
                      yaw=math.degrees(math.atan2(y,x)),roll=0)

def attachment(pose,weapon_mesh,weapon_idle):
    palm=ue.AnimPoseExtensions.get_socket_pose(pose,'ER_Palm_R',ue.AnimPoseSpaces.WORLD)
    grip=ue.AnimPoseExtensions.get_socket_pose(pose,'HandGrip_R',ue.AnimPoseSpaces.WORLD)
    previous=ue.MathLibrary.compose_transforms(ue.Transform(rotation=ue.Rotator(pitch=0,yaw=90,roll=0)),grip)
    relative=ue.MathLibrary.make_relative_transform(previous,palm)
    frame=palm_tools['pistol_transform'](weapon_mesh,weapon_idle,relative.rotation,palm.scale3d.x)
    offset=ue.MathLibrary.transform_direction(frame,ue.Vector(*HAND_SEAT))*(-1/palm.scale3d.x)
    frame.set_editor_property('translation',frame.translation+offset)
    weapon=ue.MathLibrary.compose_transforms(frame,palm)
    weapon.set_editor_property('scale3d',frame.scale3d)
    return frame,weapon

def fit_thumb(pose,skin,weapon_mesh,weapon_idle,receipt):
    _,weapon=attachment(pose,weapon_mesh,weapon_idle)
    names=['thumb_01_r','thumb_02_r','thumb_03_r']
    bones=[ue.AnimPoseExtensions.get_bone_pose(pose,n,ue.AnimPoseSpaces.WORLD) for n in names]
    vertices=[xyz(v) for v in ue.ERCharacter.get_presentation_vertices(skin,names[-1])]
    if not vertices:raise RuntimeError('Missing owned thumb-tip geometry')
    far=sorted(vertices,key=length)[-max(3,len(vertices)//10):]
    tip_local=tuple(sum(p[i] for p in far)/len(far) for i in range(3))
    old=[xyz(b.translation) for b in bones]+[xyz(ue.MathLibrary.transform_location(bones[-1],ue.Vector(*tip_local)))]
    lengths=[length(sub(old[i+1],old[i])) for i in range(3)]
    contact=ue.ERCharacter.get_weapon_surface_contact(weapon_mesh,ue.Vector(-1.5,-100,3.8),ue.Vector(-1.5,100,3.8),weapon_idle)
    if max(abs(v) for v in xyz(contact))>1000:raise RuntimeError('No opposite pistol grip surface for thumb')
    target_local=add(xyz(contact),(0,-.35,0))
    world=lambda p:xyz(ue.MathLibrary.transform_location(weapon,ue.Vector(*p)))
    target=world(target_local);base=old[0]
    if length(sub(target,base))>=sum(lengths):raise RuntimeError('Pistol thumb target exceeds its anatomical reach')
    # Extend the thumb along the opposite flank; preserve all phalanx lengths.
    points=[base,world((-7.8,-2.2,5.)),world((-4.5,-2.,4.3)),target]
    for _ in range(80):
        points[3]=target
        for i in [2,1,0]:points[i]=add(points[i+1],mul(unit(sub(points[i],points[i+1])),lengths[i]))
        points[0]=base
        for i in range(3):points[i+1]=add(points[i],mul(unit(sub(points[i+1],points[i])),lengths[i]))
        if length(sub(points[3],target))<.001:break
    for i,(name,bone) in enumerate(zip(names,bones)):
        rotation=qmul(from_to(sub(old[i+1],old[i]),sub(points[i+1],points[i])),quat(bone.rotation))
        pose=ue.AnimPoseExtensions.set_bone_pose(pose,maths['transform'](points[i],rotation,xyz(bone.scale3d)),name,ue.AnimPoseSpaces.WORLD)
    actual=ue.MathLibrary.transform_location(ue.AnimPoseExtensions.get_bone_pose(pose,names[-1],ue.AnimPoseSpaces.WORLD),ue.Vector(*tip_local))
    error=length(sub(xyz(actual),target))
    if error>.02:raise RuntimeError('Thumb fit failed: '+str(error))
    receipt.update(thumb_tip_local=tip_local,thumb_target_weapon=target_local,thumb_error_cm=error,
                   thumb_segment_lengths_cm=lengths)
    ue.log('ER_PISTOL_THUMB_FIT error_cm='+str(error))
    return pose

def motion_basis():
    body=quat(ue.Rotator(pitch=0,yaw=-90,roll=0).quaternion())
    # The supplied viewer reference has an upright grip, without sideways roll.
    camera=quat(camera_rotation().quaternion())
    return qmul(qmul(inverse(body),camera),body)

def pose_options(weapon_mesh,weapon_idle,receipt):
    delta=motion_basis()
    return dict(ER_POSE_RIGHT_ELBOW_TARGET=(-12,-5,110),
                ER_POSE_RIGHT_GRIP=((-10,25,125),delta,(-35,10,115)),
                ER_POSE_FINALIZE=lambda pose,skin:fit_thumb(pose,skin,weapon_mesh,weapon_idle,receipt))

def camera_tip(data,skin,base):
    options=ue.AnimPoseEvaluationOptions();options.should_retarget=not data.first_person_palm_grip;options.optional_skeletal_mesh=skin
    pose=ue.AnimPoseExtensions.get_anim_pose_at_time(data.idle_animation,0,options)
    socket=ue.AnimPoseExtensions.get_socket_pose(pose,'ER_Palm_R' if data.first_person_palm_grip else 'HandGrip_R',ue.AnimPoseSpaces.WORLD)
    relative=data.first_person_grip_transform if data.first_person_palm_grip else data.grip_transform
    body=ue.Transform(location=base+data.view_offset,rotation=ue.Rotator(pitch=0,yaw=-90,roll=0))
    component=ue.MathLibrary.compose_transforms(relative,socket)
    component.set_editor_property('scale3d',relative.scale3d)
    weapon=ue.MathLibrary.compose_transforms(component,body)
    visual_tip=ue.ERCharacter.get_weapon_muzzle_from_geometry(data.mesh,data.weapon_idle_animation)
    return ue.MathLibrary.transform_location(weapon,visual_tip)

def frame_settings(settings,skin,receipt):
    options=ue.AnimPoseEvaluationOptions();options.should_retarget=False;options.optional_skeletal_mesh=skin
    pose=ue.AnimPoseExtensions.get_anim_pose_at_time(settings['idle_animation'],0,options)
    frame,weapon=attachment(pose,settings['mesh'],settings['weapon_idle_animation'])
    settings['first_person_grip_transform']=frame
    defaults=ue.get_default_object(ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter').generated_class())
    base=defaults.first_person_body.get_editor_property('relative_location')
    rifle=ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/Assault/DA_Assault')
    if not rifle:raise RuntimeError('Pistol framing requires the imported assault reference')
    reference=camera_tip(rifle,skin,base)
    body=ue.Transform(location=base+settings['view_offset'],rotation=ue.Rotator(pitch=0,yaw=-90,roll=0))
    visual_tip=ue.ERCharacter.get_weapon_muzzle_from_geometry(settings['mesh'],settings['weapon_idle_animation'])
    current=ue.MathLibrary.transform_location(ue.MathLibrary.compose_transforms(weapon,body),visual_tip)
    # Match the supplied image's framing after excluding its white border:
    # barrel centre about (58.2%,64.25%), with the front sight above it near60%.
    # At90deg horizontal FOV the viewport is (1+Y/X)/2, .5-Z/X*width/(2*height).
    target=ue.Vector(*MUZZLE_CAMERA)
    settings['view_offset']=settings['view_offset']+target-current
    receipt.update(hand_seat_cm=HAND_SEAT,hand_scale=HAND_SCALE,muzzle_camera_cm=xyz(target),assault_muzzle_camera_cm=xyz(reference),visual_muzzle_local_cm=xyz(visual_tip),
                   view_offset_cm=xyz(settings['view_offset']),
                   camera_angles_degrees=dict(pitch=camera_rotation().pitch,yaw=camera_rotation().yaw,roll=0),
                   center_aim_camera_cm=(CENTER_AIM_DEPTH,0,0),
                   reference_muzzle_viewport_uv=(.582,.6425),palm_contact_height_cm=2,palm_embed_cm=.35)
    root=Path(ue.Paths.project_dir())/'Assets/weapons/SourceAssets/SketchfabWeapons'
    (root/'pistol-presentation.json').write_text(json.dumps(receipt,indent=2)+'\n')
    parent=ue.AnimPoseExtensions.get_socket_pose(pose,'ER_Palm_R',ue.AnimPoseSpaces.WORLD)
    return xyz(ue.MathLibrary.transform_location(parent,frame.translation))
