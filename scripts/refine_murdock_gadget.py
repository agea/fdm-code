"""Refine only the ten new appearances after real gallery review."""
from pathlib import Path
import json
import unreal as u
A=u.EditorAssetLibrary;E=u.MaterialEditingLibrary
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
def refine():
 bp=u.load_asset(paths[0])
 if A.get_metadata_tag(bp,'ERMurdockGadgetFinish')=='1':
  print('ER_NEW_CHARACTER_FINISH_ALREADY_APPLIED');return
 c=u.get_default_object(bp.generated_class());choices={};notes=[]
 upper=[(.08,.24,.58),(.12,.27,.52),(.22,.34,.50),(.40,.42,.42),(.10,.22,.42)]
 lower=[(.08,.14,.22),(.27,.29,.26),(.40,.37,.28),(.10,.24,.47),(.22,.25,.19)]
 for team in ['alpha','omega']:
  skins=list(c.get_editor_property(team+'_skins'));assert len(skins)==15
  for i in range(10,15):
   a=skins[i];probe=u.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(a.mesh)
   keep=[b for b in a.hidden_bones if probe.get_bone_index(b)>=0]
   notes.append(dict(team=team,index=i,removed_missing_bones=[str(b) for b in a.hidden_bones if b not in keep]))
   a.set_editor_property('hidden_bones',keep)
   if team=='alpha':
    for slot,mat in zip(a.mesh.materials,a.materials):
     name=str(slot.material_slot_name).lower()
     if not any(w in name for w in ['body','legs','m02_merc_skin','m02_arms']):continue
     tint=lower[i-10] if 'legs' in name else upper[i-10]
     graph=mat
     while isinstance(graph,u.MaterialInstanceConstant):graph=graph.parent
     assert '/Murdock' in graph.get_path_name()
     for node in E.get_material_expressions(graph):
      if str(node.get_editor_property('desc'))=='ER_GroundedTint':node.set_editor_property('default_value',u.LinearColor(*tint,1))
     if isinstance(mat,u.MaterialInstanceConstant):
      E.set_material_instance_vector_parameter_value(mat,'ER_GroundedTint',u.LinearColor(*tint,1));E.update_material_instance(mat);assert A.save_loaded_asset(mat,False)
     E.recompile_material(graph);assert A.save_loaded_asset(graph,False)
   skins[i]=a
  choices[team]=skins
 for path in paths:
  value=u.load_asset(path);d=u.get_default_object(value.generated_class())
  for team,skins in choices.items():d.set_editor_property(team+'_skins',skins)
  u.BlueprintEditorLibrary.compile_blueprint(value);A.set_metadata_tag(value,'ERMurdockGadgetFinish','1');assert A.save_loaded_asset(value,False)
 p=Path(u.Paths.project_dir())/'Saved/Testing/murdock-gadget-refine.json';p.write_text(json.dumps(notes,indent=2))
 # Record controller APIs and any auxiliary tracks without changing animation data.
 clip=c.alpha_skins[10].corpse_pose;ctrl=clip.get_editor_property('controller')
 (p.parent/'murdock-gadget-controller.json').write_text(json.dumps([x for x in dir(ctrl) if 'bone' in x.lower()],indent=2))
 print('ER_NEW_CHARACTER_REFINEMENT_COMPLETE')

refine()
