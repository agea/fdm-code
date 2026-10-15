"""Measure contact frames on owned skin geometry; never alter existing sockets."""
from pathlib import Path
import json
import runpy
import unreal as ue

geometry=runpy.run_path(str(Path(__file__).with_name('palm_geometry.py')))
xyz=lambda v:(v.x,v.y,v.z)

def setup():
    defaults=ue.get_default_object(ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter').generated_class())
    meshes={}
    for team in ['alpha','omega']:
        for appearance in defaults.get_editor_property(team+'_skins'):
            for mesh in [appearance.mesh,appearance.first_person_mesh]:meshes[mesh.get_path_name()]=mesh
    receipt=[]
    for path,mesh in meshes.items():
        changed=False
        for side in ['l','r']:
            bone='hand_'+side;hand=ue.ERCharacter.get_weapon_reference_bone(mesh,bone)
            points=[xyz(p) for p in ue.ERCharacter.get_presentation_vertices(mesh,bone)]
            fingers={f:xyz(ue.MathLibrary.inverse_transform_location(hand,ue.ERCharacter.get_weapon_reference_bone(mesh,f+'_01_'+side).translation)) for f in ['index','middle','ring','pinky','thumb']}
            fit=geometry['palm_surface'](points,fingers)
            rotation=ue.MathLibrary.make_rot_from_xz(ue.Vector(*fit['x']),ue.Vector(*fit['z']))
            frame=ue.Transform(location=ue.Vector(*fit['position']),rotation=rotation)
            name='ER_Palm_'+side.upper();old=mesh.find_socket(name)
            angles=['pitch','yaw','roll']
            if not old or geometry['length'](geometry['sub'](xyz(old.relative_location),fit['position']))>.001 or max(abs((getattr(old.relative_rotation,a)-getattr(rotation,a)+180)%360-180) for a in angles)>.001:
                if not ue.ERCharacter.set_palm_contact_socket(mesh,name,bone,frame):raise RuntimeError('Cannot save palm contact '+path)
                changed=True
            receipt.append(dict(mesh=path,side=side,**fit))
        if changed and not ue.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False):raise RuntimeError('Cannot save '+path)
    root=Path(ue.Paths.project_dir())/'Assets/weapons/SourceAssets/SketchfabWeapons'
    (root/'palm-contact-report.json').write_text(json.dumps(receipt,indent=2)+'\n')
    ue.log('ER_PALM_SURFACES_READY meshes='+str(len(meshes)))

def surface_contacts(mesh,key,guess,animation=None):
    if key=='Launcher':return []
    points=[]
    for index in range(21):
        x=guess.x-index*.5
        point=ue.ERCharacter.get_weapon_surface_contact(mesh,ue.Vector(x,0,-100),ue.Vector(x,0,100),animation)
        if max(abs(v) for v in xyz(point))<1000:points.append(point)
    if not points:raise RuntimeError('No support surface on '+key)
    return points

def pistol_transform(mesh,animation,rotation,parent_scale=.5):
    # Seat the palm higher on the backstrap, so the hand wraps the upper grip
    # instead of disappearing below the reference view's lower edge.
    contact=ue.ERCharacter.get_weapon_surface_contact(mesh,ue.Vector(-5,100,2),ue.Vector(-5,-100,2),animation)
    if max(abs(v) for v in xyz(contact))>1000:raise RuntimeError('No pistol handle surface')
    # Preserve the fitted wrist/weapon angle while seating the handle at the palm.
    frame=ue.Transform()
    frame.set_editor_property('rotation',rotation)
    # The FPS rifle component has absolute scale1; only attachment translation
    # inherits the scaled hand socket. Compensate that difference explicitly.
    offset=ue.MathLibrary.transform_direction(frame,contact)*(-1/parent_scale)
    frame.set_editor_property('translation',offset+ue.Vector(0,0,-.35/parent_scale))
    return frame
