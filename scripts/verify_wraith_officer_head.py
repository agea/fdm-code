"""Fresh saved-asset checks for Officer's sunglasses head and unchanged Assault."""
from pathlib import Path
import json,hashlib,runpy
import unreal as u
p=Path(u.Paths.project_dir()).resolve();A=u.EditorAssetLibrary
runpy.run_path(str(p/'scripts/verify_wraith_twinblast_head.py'))
c=u.get_default_object(A.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'));a=c.alpha_skins[0]
assert a.display_name=='Wraith · Officer' and len(a.attachments)==1
head=a.attachments[0].mesh
assert head.get_name()=='SM_ER_WraithOfficer_TwinblastHead' and A.get_metadata_tag(head,'EROfficerSunglassesHead')=='3'
assert str(a.attachments[0].bone)=='head' and 'head' not in map(str,a.hidden_bones)
assert head.get_num_lods()==4
assert len(head.static_materials)==9
assert any(s.material_interface.get_name()=='M_TwinBlast_ActionMovie_Head_Shirt' for s in head.static_materials), 'Sunglasses material must be retained'
assert a.mesh.get_name()=='SK_ER_WraithOfficer_Unmasked'
assert A.get_metadata_tag(a.mesh,'EROfficerHelmetRemoved')=='1'
assert u.SkeletalMeshEditorSubsystem.get_lod_count(a.mesh)==5
source=A.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/Skins/WraithOfficer/SK_ER_Wraith_Alpha_Officer')
for prop in ['skeleton','physics_asset']:assert a.mesh.get_editor_property(prop)==source.get_editor_property(prop)
assert 4 in a.hidden_material_slots and 5 in a.hidden_material_slots
assert a.materials[4].get_name()=='M_ER_HiddenSection'
manifest=json.loads((p/'Saved/Testing/WraithOfficerHead/protected.json').read_text())
for rel,expected in manifest.items():assert hashlib.sha256((p/rel).read_bytes()).hexdigest()==expected,rel
(p/'Saved/Testing/WraithOfficerHead/verified.json').write_text(json.dumps({'catalogs':3,'head_lods':4,'body_lods':5,'protected_packages_unchanged':len(manifest),'assault_head_unchanged':True,'fps_and_pose_checks':'PASS'},indent=2))
u.log('ER_WRAITH_OFFICER_HEAD_VERIFIED')
