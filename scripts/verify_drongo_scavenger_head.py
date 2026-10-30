"""Verify active Jackal retains Scavenger's head, without the GDC gas-mask overlay."""
from pathlib import Path
import hashlib,json,runpy
import unreal as u
p=Path(u.Paths.project_dir()).resolve();A=u.EditorAssetLibrary
for script in ['verify_team_skins.py','verify_team_characters.py','verify_skin_first_person_forearms.py']:runpy.run_path(str(p/'scripts'/script))
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']:
 bp=A.load_asset(path);d=u.get_default_object(bp.generated_class());a=d.omega_skins[1]
 assert A.get_metadata_tag(bp,'ERScavengerOriginalHead')=='1'
 assert a.display_name=='Drongo · Jackal' and a.mesh.get_name()=='SK_ER_Drongo_Omega_Scavenger'
 assert 0 not in a.hidden_material_slots and 5 in a.hidden_material_slots and 6 in a.hidden_material_slots
 assert a.materials[0].get_name() in ['M_ER_Scavenger_FieldMask']
 assert a.materials[0].get_editor_property('used_with_skeletal_mesh')
 assert A.get_metadata_tag(a.materials[0],'ERScavengerPrintedMask')=='1'
 assert not any(x.mesh.get_name()=='SM_ER_DrongoMask' for x in a.attachments)
 assert any(x.mesh.get_name()=='SM_ER_DrongoMask' for x in d.omega_skins[3].attachments)
 assert d.omega_skins[0].mesh.get_name()=='SK_ER_Drongo_Omega_Raider'
 assert any(m and m.get_name()=='M_Drongo_Scavenger_Head_Skin' for m in a.materials)
protected=json.loads((p/'Saved/Testing/DrongoScavenger/protected.json').read_text())
runpy.run_path(str(p/'scripts/verify_owned_package_hashes.py'))['verify_owned_package_hashes'](p,protected)
(p/'Saved/Testing/DrongoScavenger/verified.json').write_text(json.dumps({'catalogs':3,'preset':'breacher / Drongo · Jackal','protected_packages':len(protected),'other_drongo_skin_and_fps_preserved':True},indent=2))
