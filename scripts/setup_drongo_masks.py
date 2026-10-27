"""Fit Scavenger masks and hide upper hoses on embedded Drongo masks."""
from pathlib import Path
from datetime import datetime
import zipfile
import unreal as u
assets=u.EditorAssetLibrary;project=Path(u.Paths.project_dir()).resolve()
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
c=u.get_default_object(assets.load_asset(paths[0]).generated_class())
mask_path='/Game/EclipseRecoil/Characters/Teams/Gear/SM_ER_DrongoMask'
mask=assets.load_asset(mask_path) if assets.does_asset_exist(mask_path) else None
backup=project/'Saved/AssetBackups'/('DrongoMasks-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip');backup.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
 for path in paths+([mask_path] if mask else []):
  file=project/'Content'/(path.removeprefix('/Game/')+'.uasset');z.write(file,str(file.relative_to(project)))
if not mask:mask=assets.duplicate_asset('/Engine/BasicShapes/Cube',mask_path)
assert mask
if assets.get_metadata_tag(mask,'ERDrongoMaskVersion')!='3':
 assert u.ERCorpse.build_drongo_mask(mask,c.omega_skins[0].mesh)
 materials=list(mask.get_editor_property('static_materials'));source=c.omega_skins[0]
 for entry in materials:
  index=next((i for i,s in enumerate(source.mesh.materials) if s.material_interface==entry.material_interface),None)
  assert index is not None,(str(entry.material_slot_name),str(entry.material_interface),[(str(s.material_slot_name),str(s.material_interface)) for s in source.mesh.materials])
  entry.set_editor_property('material_interface',source.materials[index])
 mask.set_editor_property('static_materials',materials);assets.set_metadata_tag(mask,'ERDrongoMaskVersion','3');assert assets.save_loaded_asset(mask,False)
for path in paths:
 bp=assets.load_asset(path);d=u.get_default_object(bp.generated_class());choices=list(d.omega_skins)
 for index in [1,3]:
  a=choices[index];parts=[p for p in a.attachments if p.mesh!=mask]
  part=u.ERAppearanceAttachment();part.set_editor_property('mesh',mask);part.set_editor_property('bone','head');part.set_editor_property('transform',u.Transform())
  parts.append(part);a.set_editor_property('attachments',parts);choices[index]=a
 # GDC-based skins carry their own mask, including articulated upper hoses.
 # The Breacher/Marksman loadout labels do not identify appearance indices.
 for index in [0,2,4]:
  a=choices[index]
  hidden=list(a.hidden_bones)
  for bone in ['headHose_top_l_01','headHose_top_r_01']:
   if bone not in [str(b) for b in hidden]:hidden.append(bone)
  a.set_editor_property('hidden_bones',hidden);choices[index]=a
 d.set_editor_property('omega_skins',choices)
 a=d.get_editor_property('omega_appearance');hidden=list(a.hidden_bones)
 for bone in ['headHose_top_l_01','headHose_top_r_01']:
  if bone not in [str(b) for b in hidden]:hidden.append(bone)
 a.set_editor_property('hidden_bones',hidden);d.set_editor_property('omega_appearance',a)
 u.BlueprintEditorLibrary.compile_blueprint(bp);assert assets.save_loaded_asset(bp,False)
print('ER_DRONGO_MASKS_COMPLETE skins=5 catalogs=3 backup='+str(backup))
