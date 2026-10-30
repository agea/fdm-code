"""Fresh-process validation of owned cloth limits and the mandatory Drongo mask."""
from pathlib import Path
import runpy
import unreal as u
runpy.run_path(str(Path(__file__).with_name('verify_team_skins.py')))
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
c=u.get_default_object(u.load_asset(paths[0]).generated_class())
count=0
for a in list(c.alpha_skins[:5])+list(c.omega_skins[5:10]):
 skirt='SK_ER_Phase_' in a.mesh.get_name()
 cloth=list(a.mesh.get_editor_property('mesh_clothing_assets'))
 if not skirt and not cloth:continue
 assert cloth,a.mesh.get_name()
 assert u.ERCharacter.validate_field_clothing(a.mesh,skirt),a.mesh.get_name()
 count+=1
for path in paths:
 d=u.get_default_object(u.load_asset(path).generated_class())
 if u.EditorAssetLibrary.get_metadata_tag(u.load_asset(path),'ERPhaseGrenadierSkirtVersion')=='1':
  reference=d.omega_skins[8]
  source=next(reference.materials[i] for i,s in enumerate(reference.mesh.materials) if 'leather_skirt' in str(s.material_slot_name).lower())
  for appearance in list(d.omega_skins)[5:10]:
   assert all(appearance.materials[i]==source for i,s in enumerate(appearance.mesh.materials) if 'leather_skirt' in str(s.material_slot_name).lower())
 for index in [1,3]:
  if index==1 and u.EditorAssetLibrary.get_metadata_tag(u.load_asset(path),'ERScavengerOriginalHead')=='1':
   assert not any(p.mesh and p.mesh.get_name()=='SM_ER_DrongoMask' for p in d.omega_skins[index].attachments)
   continue
  assert any(p.mesh and p.mesh.get_name()=='SM_ER_DrongoMask' and str(p.bone)=='head' for p in d.omega_skins[index].attachments)
 for index,a in enumerate(list(d.omega_skins[:5])+[d.omega_appearance]):
  if index in [0,2,4,5]:
   assert {'headHose_top_l_01','headHose_top_r_01'} <= {str(b) for b in a.hidden_bones}
  assert not any('bones' in str(a.mesh.materials[j].material_slot_name).lower() for j in a.hidden_material_slots)
assert count==9,count
u.log('ER_FIELD_CLOTHING_VERIFY_COMPLETE skins=9 drongo_catalogs=3')
