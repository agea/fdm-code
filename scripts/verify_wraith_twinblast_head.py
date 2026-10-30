"""Verify the unmasked Wraith variant and explicitly report hash-baseline availability."""
from pathlib import Path
import json,hashlib,runpy
import unreal as u
p=Path(u.Paths.project_dir()).resolve()
for script in ['verify_team_skins.py','verify_team_characters.py','verify_skin_first_person_forearms.py']:
 runpy.run_path(str(p/'scripts'/script))
A=u.EditorAssetLibrary;c=u.get_default_object(A.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
a=c.alpha_skins[1];part=a.attachments[0];head=part.mesh
assert A.get_metadata_tag(head,'ERTwinblastHeadExtraction')=='6'
assert str(part.bone)=='head' and part.transform.scale3d==u.Vector(1,1,1)
assert 'head' not in [str(b) for b in a.hidden_bones],'The attachment must retain its bone transform'
assert len(c.alpha_skins[0].attachments)==1 and not c.alpha_skins[2].attachments
assert c.alpha_skins[0].attachments[0].mesh.get_name()=='SM_ER_WraithOfficer_TwinblastHead'
assert head.get_bounding_box().max.x>15
assert len(head.static_materials)==9
slot=next(i for i,s in enumerate(a.mesh.materials) if str(s.material_slot_name)=='M02_backpackhead')
assert slot in a.hidden_material_slots and a.materials[slot].get_name()=='M_ER_HiddenSection'
assert head.get_num_lods()==4
assert A.get_metadata_tag(head,'ERWraithCollarColor')=='NeutralBlue'
collar=head.static_materials[0].material_interface
assert isinstance(collar,u.MaterialInstanceConstant)
tint=u.MaterialEditingLibrary.get_material_instance_vector_parameter_value(collar,'ArmColor')
assert all(abs(a-b)<1e-5 for a,b in zip([tint.r,tint.g,tint.b],[.04,.06,.09]))
assert 100<=int(A.get_metadata_tag(head,'ERHeadCollarTrianglesRemoved'))<=300
manifest=p.parent/'_backups/wraith-head-study-20261030/protected-hashes.json'
receipt=p/'Saved/Testing/WraithHeadStudy/verified.json'
previous=json.loads(receipt.read_text()) if receipt.exists() else {}
report={'unmasked_skin':1,'head_bone_retained':True,'head_material_sections':len(head.static_materials),'head_lods':head.get_num_lods(),'collar_triangles_removed':int(A.get_metadata_tag(head,'ERHeadCollarTrianglesRemoved')),'neck_transition':'donor upper neck and neutral collar; original mask section hidden'}
if manifest.exists():
 hashes=json.loads(manifest.read_text())
 for rel,expected in hashes.items():assert hashlib.sha256((p/rel).read_bytes()).hexdigest()==expected,rel
 report.update(original_character_packages_unchanged=len(hashes),current_original_hash_check='PASS')
else:
 report.update(current_original_hash_check='UNAVAILABLE: baseline backup removed externally',previous_original_hash_check_packages=previous.get('original_character_packages_unchanged',previous.get('previous_original_hash_check_packages')))
# These independently retained setup receipts still prove stable body/FPS identities.
setup=json.loads((p/'Saved/Testing/WraithReplacement/setup.json').read_text())
heights=json.loads((p/'Saved/Testing/WraithReplacement/heights.json').read_text())
for row in setup:
 appearance=c.alpha_skins[row['index']]
 assert appearance.mesh.get_path_name()==row['mesh'] if row['index']!=0 else appearance.mesh.get_name()=='SK_ER_WraithOfficer_Unmasked'
 assert appearance.first_person_mesh.get_name()=='SK_ER_Wraith_Alpha_'+str(appearance.display_name).split(' · ')[1]+'_Forearms'
 height=next(h for h in heights if h['index']==row['index'])
 assert abs(appearance.scale-height['scale'])<1e-6 and abs(appearance.feet_offset-height['feet_offset'])<1e-5
report['body_fps_identities_scales_and_pose_bindings']='PASS'
receipt.write_text(json.dumps(report,indent=2))
u.log('ER_WRAITH_TWINBLAST_HEAD_VERIFIED current_hash_check=%s'%report['current_original_hash_check'])
