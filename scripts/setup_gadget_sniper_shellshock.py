"""Historical optional Veteran9 Shellshock study; active Sniper now selects Patrol7.
Copy the approved Shellshock body/FPS/physics from Field Engineer8. Original
packages and all other appearances stay unchanged. Run through Unreal Python.
"""
from pathlib import Path
import hashlib,json,shutil
import unreal as u
A=u.EditorAssetLibrary;E=u.MaterialEditingLibrary
p=Path(u.Paths.project_dir()).resolve();out=p/'Saved/Testing/GadgetMarksmanSniper';out.mkdir(parents=True,exist_ok=True)
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
folder='/Game/EclipseRecoil/Characters/Teams/Omega/Skins/GadgetVeteran/Shellshock'
fpspath='/Game/EclipseRecoil/Characters/Teams/Omega/FirstPerson/SK_ER_Gadget_Omega_Veteran_Shellshock_Forearms'
def path(x):return x.get_path_name() if x else None
def signature(a):
 return {'name':a.display_name,'mesh':path(a.mesh),'fps':path(a.first_person_mesh),'animation':path(a.animation_class),'pose':path(a.corpse_pose),'materials':[path(m) for m in a.materials],'hidden_bones':list(map(str,a.hidden_bones)),'hidden_slots':list(a.hidden_material_slots),'attachments':[{'mesh':path(x.mesh),'material':path(x.material),'bone':str(x.bone),'location':[x.transform.translation.x,x.transform.translation.y,x.transform.translation.z],'rotation':[x.transform.rotation.x,x.transform.rotation.y,x.transform.rotation.z,x.transform.rotation.w],'scale':[x.transform.scale3d.x,x.transform.scale3d.y,x.transform.scale3d.z]} for x in a.attachments],'palm_offset':[a.world_support_palm_offset_cm.x,a.world_support_palm_offset_cm.y,a.world_support_palm_offset_cm.z],'scale':a.scale,'feet':a.feet_offset,'physics_pose':a.copy_physics_pose_for_rendering}
def copy(asset,dest):
 return A.load_asset(dest) if A.does_asset_exist(dest) else A.duplicate_asset(path(asset).split('.')[0],dest)
def protected_catalogs():
 return {bp:{team:[signature(a) for i,a in enumerate(u.get_default_object(A.load_blueprint_class(bp)).get_editor_property(team+'_skins')) if team!='omega' or i!=9] for team in ['alpha','omega']} for bp in paths}
manifest=out/'protected-hashes.json'
if manifest.exists():protected=json.loads(manifest.read_text())
else:
 protected={str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for f in (p/'Assets/characters/Content').rglob('*.uasset')}
 manifest.write_text(json.dumps(protected))
snapshot=out/'other-appearances.json'
if not snapshot.exists():snapshot.write_text(json.dumps(protected_catalogs(),indent=2))
c=u.get_default_object(A.load_blueprint_class(paths[0]));source=c.omega_skins[8];old=c.omega_skins[9]
assert source.display_name=='Gadget · Field Engineer' and old.display_name=='Gadget · Veteran'
assert c.omega_skins[6].display_name=='Gadget · Mechanic'
A.make_directory(folder)
body=copy(source.mesh,folder+'/SK_ER_Gadget_Omega_Veteran_Shellshock');assert body
physics=copy(source.mesh.get_editor_property('physics_asset'),folder+'/PA_ER_GadgetVeteran_Shellshock');assert physics
body.set_editor_property('physics_asset',physics)
fps=copy(source.first_person_mesh,fpspath);assert fps
assert body.get_editor_property('skeleton')==source.mesh.get_editor_property('skeleton')
assert fps.get_editor_property('skeleton')==source.first_person_mesh.get_editor_property('skeleton')
materials=[];palettes=[]
for i,(slot,mat) in enumerate(zip(source.mesh.materials,source.materials)):
 if i in source.hidden_material_slots:
  materials.append(mat);continue
 name=str(slot.material_slot_name).lower()
 tint=(.55,.235,.075) if 'lower' in name else (.25,.285,.32)
 # Clone the approved graph/instance chain; never recolor Engineer or source art.
 def material_copy(value,suffix):
  owned=copy(value,folder+'/'+suffix)
  if isinstance(owned,u.MaterialInstanceConstant):
   parent=material_copy(value.get_editor_property('parent'),suffix+'_Parent')
   E.set_material_instance_parent(owned,parent)
   E.set_material_instance_vector_parameter_value(owned,'ER_GroundedTint',u.LinearColor(*tint,1));E.update_material_instance(owned)
  else:
   nodes=[n for n in E.get_material_expressions(owned) if str(n.get_editor_property('desc'))=='ER_GroundedTint'];assert len(nodes)==1,(name,owned)
   nodes[0].set_editor_property('default_value',u.LinearColor(*tint,1))
   owned.set_editor_property('used_with_skeletal_mesh',True);E.recompile_material(owned)
  A.set_metadata_tag(owned,'ERGadgetSniperShellshockPalette','1');assert A.save_loaded_asset(owned,False);return owned
 material=material_copy(mat,'M_ER_GadgetSniper_Section'+str(i));materials.append(material)
 palettes.append({'slot':i,'name':str(slot.material_slot_name),'tint':tint,'material':path(material)})
assert len(materials)==len(body.materials)==len(fps.materials)
for mesh in [body,fps]:
 slots=list(mesh.materials)
 for s,m in zip(slots,materials):s.set_editor_property('material_interface',m)
 mesh.set_editor_property('materials',slots);A.set_metadata_tag(mesh,'ERGadgetSniperShellshock','1');assert A.save_loaded_asset(mesh,False)
assert A.save_loaded_asset(physics,False)
a=u.ERTeamAppearance()
for field in ['animation_class','corpse_pose','hidden_bones','hidden_material_slots','attachments','world_support_palm_offset_cm','copy_physics_pose_for_rendering','scale','feet_offset']:
 a.set_editor_property(field,source.get_editor_property(field))
a.set_editor_property('display_name',old.display_name);a.set_editor_property('mesh',body);a.set_editor_property('first_person_mesh',fps);a.set_editor_property('materials',materials)
for bp_path in paths:
 bp=A.load_asset(bp_path);file=p/'Content'/(bp_path.removeprefix('/Game/')+'.uasset');backup=out/(bp.get_name()+'-before.uasset')
 if not backup.exists():shutil.copyfile(file,backup)
 d=u.get_default_object(bp.generated_class());skins=list(d.omega_skins);skins[9]=a;d.set_editor_property('omega_skins',skins)
 u.BlueprintEditorLibrary.compile_blueprint(bp);A.set_metadata_tag(bp,'ERGadgetSniperShellshock','1');assert A.save_loaded_asset(bp,False)
assert protected_catalogs()==json.loads(snapshot.read_text())
for rel,h in protected.items():assert hashlib.sha256((p/rel).read_bytes()).hexdigest()==h,rel
(out/'setup.json').write_text(json.dumps({'index':9,'name':a.display_name,'body':path(body),'fps':path(fps),'source':path(source.mesh),'palettes':palettes,'protected_packages':len(protected),'other_appearances_preserved':True},indent=2))
u.log('ER_GADGET_SNIPER_SHELLSHOCK_READY')
