"""Measure world/FPS palm sockets on the ten new skins; preserve all older art."""
from pathlib import Path
import json
import runpy
import unreal as u
p=Path(u.Paths.project_dir());geometry=runpy.run_path(str(Path(__file__).with_name('palm_geometry.py')))
xyz=lambda v:(v.x,v.y,v.z)
A=u.EditorAssetLibrary;c=u.get_default_object(A.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
receipt=[]
for team in ['alpha','omega']:
 for appearance in c.get_editor_property(team+'_skins')[10:15]:
  for mesh in [appearance.mesh,appearance.first_person_mesh]:
   if A.get_metadata_tag(mesh,'ERMurdockGadgetPalmVersion')=='1':continue
   for side in ['l','r']:
    bone='hand_'+side;hand=u.ERCharacter.get_weapon_reference_bone(mesh,bone)
    points=[xyz(v) for v in u.ERCharacter.get_presentation_vertices(mesh,bone)]
    fingers={f:xyz(u.MathLibrary.inverse_transform_location(hand,u.ERCharacter.get_weapon_reference_bone(mesh,f+'_01_'+side).translation)) for f in ['index','middle','ring','pinky','thumb']}
    try:fit=geometry['palm_surface'](points,fingers)
    except RuntimeError:
     # Murdock's mechanical palm is weighted to bot_hand, under hand_r.
     # Measure those vertices in the driven hand's frame; keep its hierarchy.
     probe=u.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(mesh)
     extra=['lowerarm_'+side]+[f+'_01_'+side for f in ['index','middle','ring','pinky','thumb']]
     if side=='r':extra+=['bot_hand','bot_loarm']
     for weighted_bone in extra:
      if probe.get_bone_index(weighted_bone)<0:continue
      frame=u.ERCharacter.get_weapon_reference_bone(mesh,weighted_bone)
      points.extend(xyz(u.MathLibrary.inverse_transform_location(hand,u.MathLibrary.transform_location(frame,v))) for v in u.ERCharacter.get_presentation_vertices(mesh,weighted_bone))
     try:fit=geometry['palm_surface'](points,fingers)
     except RuntimeError:
      (p/'Saved/Testing/murdock-gadget-palm-debug.json').write_text(json.dumps(dict(mesh=mesh.get_path_name(),side=side,points=points,fingers=fingers),indent=2))
      raise RuntimeError('Palm fitting requires review: '+mesh.get_name()+' '+side)
    rotation=u.MathLibrary.make_rot_from_xz(u.Vector(*fit['x']),u.Vector(*fit['z']))
    frame=u.Transform(location=u.Vector(*fit['position']),rotation=rotation)
    assert u.ERCharacter.set_palm_contact_socket(mesh,'ER_Palm_'+side.upper(),bone,frame),mesh.get_name()
    receipt.append(dict(mesh=mesh.get_path_name(),side=side,**fit))
   A.set_metadata_tag(mesh,'ERMurdockGadgetPalmVersion','1')
   assert A.save_loaded_asset(mesh,False)
(p/'Saved/Testing/murdock-gadget-palms.json').write_text(json.dumps(receipt,indent=2))
print('ER_NEW_PALM_SOCKETS_READY contacts='+str(len(receipt)))
