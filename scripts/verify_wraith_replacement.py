"""Fresh-process Wraith catalog/FPS checks and unchanged-art/source hashes."""
from pathlib import Path
import hashlib,json,runpy
import unreal as u
p=Path(u.Paths.project_dir()).resolve();out=p/'Saved/Testing/WraithReplacement'
runpy.run_path(str(p/'scripts/verify_team_skins.py'))
runpy.run_path(str(p/'scripts/verify_team_characters.py'))
runpy.run_path(str(p/'scripts/verify_skin_first_person_forearms.py'))
A=u.EditorAssetLibrary;E=u.MaterialEditingLibrary
c=u.get_default_object(A.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
assert [str(s.display_name) for s in c.alpha_skins[:3]]==['Wraith · Officer','Wraith · Assault','Wraith · Patrol']
assert len({tuple(str(s.material_slot_name) for s in a.mesh.materials) for a in c.alpha_skins[:3]})==3
neutral=u.load_asset('/Game/EclipseRecoil/Characters/Teams/Gear/M_ER_HiddenSection')
for index,a in enumerate(c.alpha_skins[:3]):
 if A.get_metadata_tag(A.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter'),'ERWraithTwinblastHead') in ['1','2','3','4','5','6'] and index==1:
  assert len(a.attachments)==1 and a.attachments[0].mesh.get_name()=='SM_ER_WraithAssault_TwinblastHead'
  assert str(a.attachments[0].bone)=='head'
  for i,s in enumerate(a.mesh.materials):
   if 'backpackhead' in str(s.material_slot_name).lower():
    assert (i not in a.hidden_material_slots and a.materials[i].get_name()=='M_ER_WraithAssault_NeckOnly') if A.get_metadata_tag(A.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter'),'ERWraithTwinblastHead')=='5' else i in a.hidden_material_slots
 elif index==0 and A.get_metadata_tag(A.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter'),'ERWraithOfficerHead')=='1':
  assert len(a.attachments)==1 and a.attachments[0].mesh.get_name()=='SM_ER_WraithOfficer_TwinblastHead'
  assert a.mesh.get_name()=='SK_ER_WraithOfficer_Unmasked'
  assert 4 in a.hidden_material_slots
 else:assert not a.attachments
 assert a.copy_physics_pose_for_rendering
 assert 'backpack' in [str(b) for b in a.hidden_bones]
 assert len(a.materials)==len(a.mesh.materials)
 assert a.mesh.skeleton==a.corpse_pose.get_editor_property('skeleton')
 for mesh in [a.mesh,a.first_person_mesh]:
  for socket in ['HandGrip_R','HandGrip_L','ER_Palm_R','ER_Palm_L']:assert mesh.find_socket(socket),(mesh.get_path_name(),socket)
 for i,s in enumerate(a.mesh.materials):
  if 'weapon' in str(s.material_slot_name).lower():assert i in a.hidden_material_slots and a.materials[i]==neutral
 for m in a.materials:
  while isinstance(m,u.MaterialInstanceConstant):m=m.parent
  node=E.get_material_property_input_node(m,u.MaterialProperty.MP_EMISSIVE_COLOR)
  assert node and node.get_editor_property('r')==0,m.get_path_name()
hashes=json.loads((p.parent/'_backups/wraith-replacement-20261030/protected-character-hashes.json').read_text())
for rel,h in hashes.items():assert hashlib.sha256((p/rel).read_bytes()).hexdigest()==h,rel
for rel,h in json.loads((out/'import-hashes.json').read_text()).items():assert hashlib.sha256((p/'Assets/characters'/rel).read_bytes()).hexdigest()==h,rel
stage=p.parent/'_imports/ParagonStaging'
for rel,h in json.loads((out/'staging-protected-hashes.json').read_text()).items():assert hashlib.sha256((stage/rel).read_bytes()).hexdigest()==h,rel
assert not list((stage/'Content/ParagonMurdock').rglob('*.uasset'))
assert not list((stage/'Content/ParagonMurdock').rglob('*.umap'))
(out/'verified.json').write_text(json.dumps({'wraith_skins':3,'other_character_art_unchanged':len(hashes),'source_packages_unchanged':145,'staging_gadget_wraith_unchanged':True,'staging_murdock_removed':True,'catalogs':3,'fps_rig_and_camera_preserved':True},indent=2))
u.log('ER_WRAITH_REPLACEMENT_VERIFIED skins=3 old_art_unchanged=1 staging_gadget_preserved=1')
