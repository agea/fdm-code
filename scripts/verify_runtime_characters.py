"""Verify current runtime character assets without Saved receipts or staging inputs."""
from pathlib import Path
import json,hashlib,runpy
import unreal as u
p=Path(u.Paths.project_dir()).resolve();state=json.loads((p/'docs/CHARACTER_RUNTIME_STATE.json').read_text())
for rel,h in dict(state['asset_hashes'],**state['catalog_hashes']).items():
 assert (p/rel).is_file(),rel
 assert hashlib.sha256((p/rel).read_bytes()).hexdigest()==h,rel
for rel in state['removed_packages'].values():assert not (p/rel).exists(),rel
for script in ['verify_team_skins.py','verify_team_characters.py','verify_skin_first_person_forearms.py']:runpy.run_path(str(p/'scripts'/script))
A=u.EditorAssetLibrary;c=u.get_default_object(A.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
for i,name in [(0,'SM_ER_WraithOfficer_TwinblastHead'),(1,'SM_ER_WraithAssault_TwinblastHead')]:
 a=c.alpha_skins[i];assert len(a.attachments)==1 and a.attachments[0].mesh.get_name()==name
 assert str(a.attachments[0].bone)=='head' and 'head' not in map(str,a.hidden_bones)
 assert a.attachments[0].mesh.get_num_lods()==4
assert not c.alpha_skins[2].attachments
scavenger=c.omega_skins[1];assert 0 not in scavenger.hidden_material_slots and {5,6}<=set(scavenger.hidden_material_slots)
assert A.get_metadata_tag(scavenger.materials[0],'ERScavengerPrintedMask')=='1'
assert not any(x.mesh.get_name()=='SM_ER_DrongoMask' for x in scavenger.attachments)
assert c.omega_skins[7].display_name=='Gadget · Patrol'
R=u.AssetRegistryHelpers.get_asset_registry();R.search_all_assets(True);R.wait_for_completion()
options=u.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True,include_searchable_names=False,include_soft_management_references=True,include_hard_management_references=True)
removed=set(state['removed_packages'])
for rel in state['asset_hashes']:
 package='/Game/'+rel.split('/Content/',1)[1].removesuffix('.uasset').removesuffix('.umap')
 assert not removed.intersection(map(str,R.get_dependencies(package,options))),package
u.log('ER_RUNTIME_CHARACTERS_VERIFIED packages=%d skins=20'%state['character_packages'])
