"""Add bounded field-clothing collision and restore Drongo head gear on owned assets.

Run via Unreal -run=pythonscript. Original art, gameplay physics assets and
animation clips stay untouched; ZIP backups precede every owned package write.
"""
from pathlib import Path
from datetime import datetime
import zipfile,json
import unreal as u
assets=u.EditorAssetLibrary
project=Path(u.Paths.project_dir())
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
marker='ERFieldClothingVersion'
bp=assets.load_asset(paths[0]);c=u.get_default_object(bp.generated_class())
assert len(c.alpha_skins)==10 and len(c.omega_skins)==10
meshes={a.mesh.get_path_name().split('.')[0]:a.mesh for a in list(c.alpha_skins[:5])+list(c.omega_skins[5:])}
backup=project/'Saved/AssetBackups'/('FieldClothing-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip');backup.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
 for path in paths+list(meshes):
  file=project/'Content'/(path.removeprefix('/Game/')+'.uasset');assert file.exists(),file
  z.write(file,str(file.relative_to(project)))
report=[]
for path,mesh in meshes.items():
 if assets.get_metadata_tag(mesh,marker)=='1':continue
 skirt='SK_ER_Phase_' in mesh.get_name()
 if skirt or list(mesh.get_editor_property('mesh_clothing_assets')):
  assert u.ERCharacter.configure_field_clothing(mesh,skirt),mesh.get_name()
  assets.set_metadata_tag(mesh,marker,'1');assert assets.save_loaded_asset(mesh,False)
  report.append(mesh.get_path_name())
for path in paths:
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
 u.BlueprintEditorLibrary.compile_blueprint(bp);assets.set_metadata_tag(bp,marker,'1');assert assets.save_loaded_asset(bp,False)
(project/'Saved/Testing/FieldClothing-setup.json').write_text(json.dumps({'backup':str(backup),'meshes':report},indent=2))
u.log('ER_FIELD_CLOTHING_SETUP_COMPLETE meshes=%d backup=%s'%(len(report),backup))

import runpy
runpy.run_path(str(Path(__file__).with_name('setup_drongo_masks.py')))
