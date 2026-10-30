"""Add bounded field-clothing collision and restore Drongo head gear on owned assets.

Run via Unreal -run=pythonscript. Original art, gameplay physics assets and
animation clips stay untouched; ZIP backups precede every owned package write.
"""
from pathlib import Path
from datetime import datetime
import zipfile,json,hashlib
import unreal as u
assets=u.EditorAssetLibrary
project=Path(u.Paths.project_dir())
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
marker='ERFieldClothingVersion'
version='3'
cloth_only='ERClothOnly' in u.SystemLibrary.get_command_line()
bp=assets.load_asset(paths[0]);c=u.get_default_object(bp.generated_class())
assert len(c.alpha_skins)>=10 and len(c.omega_skins)>=10
meshes={a.mesh.get_path_name().split('.')[0]:a.mesh for a in list(c.alpha_skins[:5])+list(c.omega_skins[5:10])}
tb='/Game/ParagonTwinblast/Characters/Heroes/TwinBlast/'
officer=assets.load_asset(tb+'Skins/Tier2/ActionMovie/Meshes/TwinBlast_ActionHero')
classic=assets.load_asset(tb+'Meshes/TwinBlast')
collision=assets.load_asset(tb+'Skins/Tier2/ActionMovie/Meshes/TwinBlast_ActionHero_ClothCollision')
assert officer and classic and collision
protected=paths+[a.first_person_mesh.get_path_name().split('.')[0] for a in list(c.alpha_skins[:5])+list(c.omega_skins[5:10])]+[m.physics_asset.get_path_name().split('.')[0] for m in meshes.values()]+[m.get_path_name().split('.')[0] for m in [officer,classic,collision]]
def hashes():
 return {p:hashlib.sha256((project/'Content'/(p.removeprefix('/Game/')+'.uasset')).read_bytes()).hexdigest() for p in protected}
before=hashes()
backup=project/'Saved/AssetBackups'/('FieldClothing-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip');backup.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
 for path in paths+list(meshes):
  file=project/'Content'/(path.removeprefix('/Game/')+'.uasset');assert file.exists(),file
  z.write(file,str(file.relative_to(project)))
with zipfile.ZipFile(backup) as z:assert z.testzip() is None
report=[]
for path,mesh in meshes.items():
 skirt='SK_ER_Phase_' in mesh.get_name()
 if assets.get_metadata_tag(mesh,marker)==version:
  assert u.ERCharacter.validate_field_clothing(mesh,skirt),mesh.get_name()
  continue
 if skirt or list(mesh.get_editor_property('mesh_clothing_assets')):
  source=None if skirt else officer if '_Officer' in mesh.get_name() else classic
  assert u.ERCharacter.configure_field_clothing(mesh,skirt,source,None if skirt else collision),mesh.get_name()
  assets.set_metadata_tag(mesh,marker,version);assert assets.save_loaded_asset(mesh,False)
  assert u.ERCharacter.validate_field_clothing(mesh,skirt),mesh.get_name()
  report.append(mesh.get_path_name())
assert before==hashes(),'Protected character, FPS, ragdoll or source package changed'
for path in ([] if cloth_only else paths):
 bp=assets.load_asset(path);defaults=u.get_default_object(bp.generated_class());choices=list(defaults.omega_skins)
 for i,a in enumerate(choices[:5]):
  hidden=[str(b) for b in a.hidden_bones]
  a.set_editor_property('hidden_bones',[b for b in hidden if not any(w in b.lower() for w in ['headhose','hosechest','hoseshoulder','hoseside','chesthose'])])
  a.set_editor_property('hidden_material_slots',[j for j in a.hidden_material_slots if 'bones' not in str(a.mesh.materials[j].material_slot_name).lower()])
  choices[i]=a
 defaults.set_editor_property('omega_skins',choices)
 # The legacy fallback must use the same mask policy as selectable Drongo skins.
 a=defaults.get_editor_property('omega_appearance')
 a.set_editor_property('hidden_bones',[b for b in a.hidden_bones if not any(w in str(b).lower() for w in ['headhose','hosechest','hoseshoulder','hoseside','chesthose'])])
 a.set_editor_property('hidden_material_slots',[j for j in a.hidden_material_slots if 'bones' not in str(a.mesh.materials[j].material_slot_name).lower()])
 defaults.set_editor_property('omega_appearance',a)
 u.BlueprintEditorLibrary.compile_blueprint(bp);assets.set_metadata_tag(bp,marker,version);assert assets.save_loaded_asset(bp,False)
(project/'Saved/Testing/FieldClothing-setup.json').write_text(json.dumps({'version':version,'backup':str(backup),'meshes':report,'protected_hashes':before},indent=2))
u.log('ER_FIELD_CLOTHING_SETUP_COMPLETE meshes=%d backup=%s'%(len(report),backup))

import runpy
if not cloth_only:runpy.run_path(str(Path(__file__).with_name('setup_drongo_masks.py')))
