"""Own Gadget's identical shadow collision data; drop obsolete vendor previews.
Only the shadow-physics pointer changes on three skins. Main ragdolls, skeleton,
materials, sockets, scale and all animation/FPS bindings remain unchanged.
"""
from pathlib import Path
import json
import unreal as u
A=u.EditorAssetLibrary;p=Path(u.Paths.project_dir()).resolve();out=Path('/private/tmp/rr-character-cleanup');out.mkdir(exist_ok=True)
c=u.get_default_object(A.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
path='/Game/EclipseRecoil/Characters/Teams/Omega/Gadget/PA_ER_Gadget_Shadow'
ready=A.load_asset(path) if A.does_asset_exist(path) else None
if ready and all(c.omega_skins[i].mesh.get_editor_property('shadow_physics_asset')==ready for i in [5,6,7]):
 u.log('ER_GADGET_SHADOW_REFERENCE_ALREADY_CLEAN')
else:
 source=A.load_asset('/Game/ParagonGadget/Characters/Heroes/Gadget/Meshes/Gadget_CylShadows');assert source
 raw=A.load_asset('/Game/ParagonGadget/Characters/Heroes/Gadget/Meshes/Gadget');assert raw
 path='/Game/EclipseRecoil/Characters/Teams/Omega/Gadget/PA_ER_Gadget_Shadow'
 owned=A.load_asset(path) if A.does_asset_exist(path) else A.duplicate_asset(source.get_path_name().split('.')[0],path);assert owned
 assert u.ERCharacter.replace_owned_asset_references(owned,[raw],[c.omega_skins[5].mesh]);assert A.save_loaded_asset(owned,False)
 records=[]
 for i in [5,6,7]:
  mesh=c.omega_skins[i].mesh
  assert mesh.get_editor_property('shadow_physics_asset') in [source,owned]
  keep={prop:mesh.get_editor_property(prop) for prop in ['skeleton','physics_asset','materials']}
  bounds=mesh.get_imported_bounds();lods=u.SkeletalMeshEditorSubsystem.get_lod_count(mesh)
  probe=u.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(mesh)
  def frames_data():
   result=[]
   for n in range(probe.get_num_bones()):
    t=u.ERCharacter.get_weapon_reference_bone(mesh,probe.get_bone_name(n));result.append([t.translation.x,t.translation.y,t.translation.z,t.rotation.x,t.rotation.y,t.rotation.z,t.rotation.w,t.scale3d.x,t.scale3d.y,t.scale3d.z])
   return result
  frames=frames_data()
  mesh.set_editor_property('shadow_physics_asset',owned);assert A.save_loaded_asset(mesh,False)
  assert all(mesh.get_editor_property(k)==v for k,v in keep.items())
  after=mesh.get_imported_bounds()
  assert (after.origin-bounds.origin).length()<.001 and (after.box_extent-bounds.box_extent).length()<.001
  assert u.SkeletalMeshEditorSubsystem.get_lod_count(mesh)==lods
  assert all(abs(a-b)<1e-6 for x,y in zip(frames,frames_data()) for a,b in zip(x,y))
  records.append({'mesh':mesh.get_path_name(),'bone_frames':len(frames),'lods':lods,'main_physics_and_materials_unchanged':True})
 (out/'shadow-reference.json').write_text(json.dumps({'shadow_data_copied_from':source.get_path_name(),'only_preview_reference_replaced':True,'meshes':records},indent=2))
 u.log('ER_GADGET_SHADOW_REFERENCE_CLEANED')
