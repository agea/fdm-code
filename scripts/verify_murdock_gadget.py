"""Check the saved new appearance bindings and preserve the earlier20 variants."""
import hashlib
import json
import runpy
from pathlib import Path
import unreal as u
p=Path(u.Paths.project_dir()).resolve()
runpy.run_path(str(p/'scripts/verify_team_skins.py'))
runpy.run_path(str(p/'scripts/verify_skin_first_person_forearms.py'))
c=u.get_default_object(u.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
neutral=u.load_asset('/Game/EclipseRecoil/Characters/Teams/Gear/M_ER_HiddenSection')
for team,hero in [('alpha','Murdock'),('omega','Gadget')]:
 skins=list(c.get_editor_property(team+'_skins'))
 assert len(skins)==15
 for index,a in enumerate(skins[10:],10):
  assert a.display_name.startswith(hero+' · ') and a.mesh.skeleton==a.corpse_pose.get_editor_property('skeleton')
  assert a.first_person_mesh and a.mesh.physics_asset and a.copy_physics_pose_for_rendering
  assert .5<a.scale<1.5
  if a.attachments:assert hero=='Murdock' and index==12 and len(a.attachments)==1 and a.attachments[0].mesh.get_name()=='SM_ER_MurdockRecon_HumanHead'
  assert all(a.mesh.find_socket(name) and a.first_person_mesh.find_socket(name) for name in ['HandGrip_R','HandGrip_L','ER_Palm_R','ER_Palm_L'])
  assert all(a.materials[i]==neutral for i in a.hidden_material_slots)
  names=[str(s.material_slot_name).lower() for s in a.mesh.materials]
  for i,name in enumerate(names):
   if any(w in name for w in (['weapon','glass'] if hero=='Murdock' else ['machine','turkeybot'])):
    assert i in a.hidden_material_slots,(hero,index,name)
  probe=u.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(a.mesh)
  accessory='bot_uparm' if hero=='Murdock' else 'backpack_base'
  if probe.get_bone_index(accessory)>=0:assert accessory in [str(x) for x in a.hidden_bones]
  for material in a.materials:
   while isinstance(material,u.MaterialInstanceConstant):material=material.parent
   node=u.MaterialEditingLibrary.get_material_property_input_node(material,u.MaterialProperty.MP_EMISSIVE_COLOR)
   assert node and node.get_editor_property('r')==0,(hero,index,material.get_name(),'glow')
  print('ER_NEW_CHARACTER_BINDINGS_PASS',team,index,a.display_name)
# The skull mask has its source atlas and stays visible on the first new skin.
a=c.alpha_skins[10]
mask=next(i for i,s in enumerate(a.mesh.materials) if 'merc_mask' in str(s.material_slot_name).lower())
assert mask not in a.hidden_material_slots and a.materials[mask]!=neutral
# No earlier owned art package may be resaved by this migration.
manifest=json.loads((p.parent/'_backups/murdock-gadget-20261029/protected-character-hashes.json').read_text())
for relative,expected in manifest.items():
 f=p/'Assets/characters/Content/EclipseRecoil/Characters'/relative
 assert hashlib.sha256(f.read_bytes()).hexdigest()==expected,relative
print('ER_MURDOCK_GADGET_VERIFIED skins=10 old_art_unchanged=1')

if u.EditorAssetLibrary.get_metadata_tag(u.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter'),'ERMurdockGadgetVariety')=='2':
 signatures={tuple(str(s.material_slot_name) for s in a.mesh.materials) for a in c.alpha_skins[10:15]}
 assert len(signatures)>=4,'Murdock needs four distinct source bodies'
 recon=c.alpha_skins[12]
 assert len(recon.attachments)==1 and str(recon.attachments[0].bone)=='head'
 head=recon.attachments[0].mesh
 assert u.EditorAssetLibrary.get_metadata_tag(head,'ERFullHeadGeometry')=='1'
 bounds=head.get_bounding_box()
 assert bounds.max.x-bounds.min.x>18,'Recon head must include the skull/helmet, not only eyes and mouth'
 for index in [10,11,12,14]:
  a=c.omega_skins[index]
  graph=next(m for s,m in zip(a.mesh.materials,a.materials) if 'lowerbody' in str(s.material_slot_name).lower())
  while isinstance(graph,u.MaterialInstanceConstant):graph=graph.parent
  assert u.EditorAssetLibrary.get_metadata_tag(graph,'ERPreserveAbdomen')=='1'
  assert any(str(n.get_editor_property('desc'))=='ER_PreserveAuthoredAbdomen' for n in u.MaterialEditingLibrary.get_material_expressions(graph))
 print('ER_CHARACTER_VARIETY_VERIFIED murdock_bodies=4 gadget_abdomen_preserved=4')
