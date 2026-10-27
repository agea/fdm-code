"""Adapt all five source finger chains to the fitted hand and real handle surface."""
from pathlib import Path
import runpy
import unreal as ue


def fit(pose,weapon,skin,mesh,animation,source):
    h = runpy.run_path(str(Path(__file__).with_name('retarget_weapon_hands.py')))
    xyz,quat = h['xyz'],h['quat']
    add,sub,mul,unit,length = (h[n] for n in ('add','sub','mul','unit','length'))
    world = ue.AnimPoseSpaces.WORLD
    bone = lambda n:ue.AnimPoseExtensions.get_bone_pose(pose,n,world)
    to_world = lambda p:xyz(ue.MathLibrary.transform_location(weapon,ue.Vector(*p)))
    to_local = lambda p:xyz(ue.MathLibrary.inverse_transform_location(weapon,ue.Vector(*p)))
    sections = source['handle_sections']
    weapon_scale = weapon.scale3d.x
    def section(y):
        # The circular guard wings start beyond -4; use the handle's own
        # centreline there, retaining the actual triangle query for contact.
        value = min(sections,key=lambda s:abs(s['y']-max(-4,min(3,y))))
        return ((sum(value['x'])/2,y,sum(value['z'])/2),
                ((value['x'][1]-value['x'][0])/2,(value['z'][1]-value['z'][0])/2))
    receipt = {}
    for finger in ('index','middle','ring','pinky','thumb'):
        names = [finger+'_'+str(i).zfill(2)+'_r' for i in (1,2,3)]
        bones = [bone(n) for n in names]
        vertices = sorted([xyz(v) for v in ue.ERCharacter.get_presentation_vertices(skin,names[-1])],key=length)
        far = vertices[-max(3,len(vertices)//10):]
        if not far:raise RuntimeError('Missing fingertip geometry '+finger)
        tip_local = tuple(sum(p[i] for p in far)/len(far) for i in range(3))
        old = [xyz(b.translation) for b in bones]+[xyz(ue.MathLibrary.transform_location(bones[-1],ue.Vector(*tip_local)))]
        lengths = [length(sub(old[i+1],old[i])) for i in range(3)]
        base = to_local(old[0])
        src = source['fingers'][finger]
        source_length = sum(length(sub(src[i+1],src[i])) for i in range(3))
        scale = sum(lengths)/(source_length*weapon_scale)
        station = base[1]+(src[-1][1]-src[0][1])*scale
        if finger=='thumb':
            index = to_local(xyz(bone('index_01_r').translation))
            station = index[1]+(src[-1][1]-source['fingers']['index'][0][1])*scale
        centre,_ = section(station)
        src_centre,_ = section(src[-1][1])
        desired = unit((src[-1][0]-src_centre[0],0,src[-1][2]-src_centre[2]))
        near = unit((base[0]-centre[0],0,base[2]-centre[2]))
        # Leave room for the smaller handle in the thumb/index web instead
        # of forcing the thumb tip tightly onto the same surface as the fingers.
        clearance = .8 if finger=='thumb' else .2
        if finger=='thumb':desired = unit(add(mul(desired,.6),mul(near,.4)))
        target = None
        for step in range(21):
            w = step/20
            direction = unit(add(mul(desired,1-w),mul(near,w)))
            start = add(centre,mul(direction,100))
            end = sub(centre,mul(direction,100))
            contact = xyz(ue.ERCharacter.get_weapon_surface_contact(mesh,ue.Vector(*start),ue.Vector(*end),animation))
            if max(abs(v) for v in contact)>1000:continue
            candidate = add(contact,mul(direction,clearance/weapon_scale))
            if length(sub(candidate,base))*weapon_scale < sum(lengths)*.97:
                target = to_world(candidate)
                break
        if target is None:raise RuntimeError('No reachable handle contact for '+finger)
        # Seed from the original closed fist, using this rig's measured segment
        # lengths. FABRIK closes onto the actual imported handle surface.
        points = [old[0]]
        for i in range(3):
            direction = unit(sub(src[i+1],src[i]))
            direction = h['rotate'](quat(weapon.rotation),direction)
            points.append(add(points[-1],mul(direction,lengths[i])))
        for _ in range(160):
            points[-1] = target
            for i in (2,1,0):
                points[i] = add(points[i+1],mul(unit(sub(points[i],points[i+1])),lengths[i]))
            points[0] = old[0]
            for i in range(3):
                points[i+1] = add(points[i],mul(unit(sub(points[i+1],points[i])),lengths[i]))
            if length(sub(points[-1],target))<.001:break
        error = length(sub(points[-1],target))
        if error>.02:raise RuntimeError('Finger closure failed '+finger+' '+str(error))
        for i,(name,b) in enumerate(zip(names,bones)):
            rotation = h['qmul'](h['from_to'](sub(old[i+1],old[i]),sub(points[i+1],points[i])),quat(b.rotation))
            pose = ue.AnimPoseExtensions.set_bone_pose(pose,h['transform'](points[i],rotation,xyz(b.scale3d)),name,world)
        tip = ue.MathLibrary.transform_location(bone(names[-1]),ue.Vector(*tip_local))
        error = length(sub(xyz(tip),target))
        if error>.03:raise RuntimeError('Rendered fingertip misses handle '+finger)
        receipt[finger] = dict(contact_error_cm=error,segment_lengths_cm=lengths,
                              contact_weapon_cm=to_local(target),tip_local_cm=tip_local,
                              handle_clearance_cm=clearance)
    return pose,receipt
