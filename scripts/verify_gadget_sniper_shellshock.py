"""Fresh-process checks for distinct active Gadget Marksman and Sniper bodies."""
from pathlib import Path
import hashlib,json,runpy
import unreal as u
p=Path(u.Paths.project_dir()).resolve();out=p/'Saved/Testing/GadgetMarksmanSniper';A=u.EditorAssetLibrary;E=u.MaterialEditingLibrary
for name in ['verify_team_skins.py','verify_team_characters.py','verify_skin_first_person_forearms.py']:runpy.run_path(str(p/'scripts'/name))
helpers={};exec((p/'scripts/setup_gadget_sniper_shellshock.py').read_text().split('\nmanifest=')[0],helpers)
assert helpers['protected_catalogs']()==json.loads((out/'other-appearances.json').read_text())
protected=json.loads((out/'protected-hashes.json').read_text())
runpy.run_path(str(p/'scripts/verify_owned_package_hashes.py'))['verify_owned_package_hashes'](p,protected)
c=u.get_default_object(A.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'));a=c.omega_skins[9];source=c.omega_skins[8];marksman=c.omega_skins[6]
assert a.display_name=='Gadget · Veteran' and marksman.display_name=='Gadget · Mechanic'
assert a.mesh.get_name()=='SK_ER_Gadget_Omega_Veteran_Shellshock' and a.mesh!=source.mesh and a.mesh!=marksman.mesh
assert a.first_person_mesh.get_name()=='SK_ER_Gadget_Omega_Veteran_Shellshock_Forearms'
assert a.mesh.get_editor_property('skeleton')==source.mesh.get_editor_property('skeleton')==marksman.mesh.get_editor_property('skeleton')
assert a.animation_class==source.animation_class and a.corpse_pose==source.corpse_pose
assert list(a.hidden_bones)==list(source.hidden_bones) and list(a.hidden_material_slots)==list(source.hidden_material_slots)
assert a.world_support_palm_offset_cm==source.world_support_palm_offset_cm
assert a.mesh.get_editor_property('physics_asset')!=source.mesh.get_editor_property('physics_asset')
assert u.SkeletalMeshEditorSubsystem.get_lod_count(a.mesh)==u.SkeletalMeshEditorSubsystem.get_lod_count(source.mesh)
for left,right in [(a.mesh,source.mesh),(a.first_person_mesh,source.first_person_mesh)]:
 for name in ['HandGrip_L','HandGrip_R','ER_Palm_L','ER_Palm_R']:
  x,y=left.find_socket(name),right.find_socket(name);assert x and y
  for prop in ['bone_name','relative_location','relative_rotation','relative_scale']:assert x.get_editor_property(prop)==y.get_editor_property(prop),(name,prop)
for row in json.loads((out/'setup.json').read_text())['palettes']:
 m=a.materials[row['slot']]
 while isinstance(m,u.MaterialInstanceConstant):m=m.get_editor_property('parent')
 assert m.get_editor_property('used_with_skeletal_mesh')
 assert E.get_material_property_input_node(m,u.MaterialProperty.MP_EMISSIVE_COLOR).get_editor_property('r')==0
 tint=next(n.get_editor_property('default_value') for n in E.get_material_expressions(m) if str(n.get_editor_property('desc'))=='ER_GroundedTint')
 assert all(abs(x-y)<1e-5 for x,y in zip([tint.r,tint.g,tint.b],row['tint']))
(out/'verified.json').write_text(json.dumps({'catalogs':3,'protected_packages':len(protected),'marksman_unchanged':True,'sniper_shellshock':True,'fps_pose_and_socket_checks':'PASS','other_appearances_unchanged':True,'body_lods':u.SkeletalMeshEditorSubsystem.get_lod_count(a.mesh)},indent=2))
u.log('ER_GADGET_SNIPER_SHELLSHOCK_VERIFIED')
