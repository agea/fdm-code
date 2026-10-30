"""Revision2: four Murdock bodies and Gadget's authored exposed abdomen."""
from pathlib import Path
import runpy
import unreal as ue
exec(Path(__file__).with_name('setup_murdock_gadget.py').read_text().split('\ndef build():')[0])
VERSION='2';MARKER='ERMurdockGadgetVariety'

def build_variety():
 bp=load(BP_PATHS[0])
 if assets.get_metadata_tag(bp,MARKER)==VERSION:return
 c=ue.get_default_object(bp.generated_class())
 alpha=list(c.alpha_skins);omega=list(c.omega_skins)
 manny=load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
 fps_rig=alpha[0].first_person_mesh;neutral=load(root+'/Gear/M_ER_HiddenSection')
 specs=[(12,'MurdockRecon',murdock+'Skins/Tier2/Corrupt/Meshes/Murdock_Corrupt',(.17,.31,.50),(.27,.29,.26)),
        (13,'MurdockPatrol',murdock+'Skins/Tier_1_5/Shogun/Meshes/Murdock_Shogun_GDC',(.20,.33,.52),(.29,.31,.32))]
 for index,key,source_path,dominant,muted in specs:
  old=alpha[index];source=load(source_path)
  folder=root+'/Alpha/Skins/'+key+'/Variety2';assets.make_directory(folder)
  mesh=copy(source,folder+'/SK_ER_Murdock_Alpha_'+key+'_V2');mesh.skeleton=alpha[10].mesh.skeleton
  physics=copy(load(murdock+'Meshes/Murdock_Extents'),folder+'/PA_ER_'+key+'_V2')
  assert ue.ERCorpse.build_team_ragdoll_physics(physics,mesh,limb_swing=55,leg_swing=45)
  assert ue.ERCharacter.copy_training_grip_sockets(mesh,manny)
  save(physics)
  probe=ue.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(mesh)
  hide=[b for b in ['bot_uparm','gun','shotgun_base','helmet_sight','hip_pack_a','hip_pack_b','hip_pack_c','hip_pack_d','pack_hose_in','pack_hose_out','shogun_hiparmor_01_l','shogun_hiparmor_01_m','shogun_hiparmor_01_r'] if probe.get_bone_index(b)>=0]
  hidden=[];materials=[];slots=list(mesh.materials)
  for i,slot in enumerate(slots):
   name=str(slot.material_slot_name);low=name.lower()
   if any(w in low for w in ['weapon','glass','packs']):hidden.append(i);material=neutral
   else:
    natural=any(w in low for w in ['face','eye','mouth','tear','skin_head','arm_skin'])
    tint=(1,1,1) if natural else muted if 'legs' in low else dominant
    material=finish(slot.material_interface,folder,'Alpha',name,tint,natural)
   materials.append(material);slot.set_editor_property('material_interface',material)
  mesh.set_editor_property('materials',slots);save(mesh)
  fps=copy(source,root+'/Alpha/FirstPerson/SK_ER_Murdock_Alpha_'+key+'_V2_Forearms')
  assert ue.ERCharacter.build_first_person_forearms(fps,mesh,fps_rig,hidden,hide);save(fps)
  a=ue.ERTeamAppearance()
  for field in ['display_name','animation_class','corpse_pose','scale','feet_offset','copy_physics_pose_for_rendering']:
   a.set_editor_property(field,old.get_editor_property(field))
  for field,value in dict(mesh=mesh,first_person_mesh=fps,materials=materials,hidden_bones=hide,hidden_material_slots=hidden,attachments=[]).items():a.set_editor_property(field,value)
  alpha[index]=a
 # Make the second Merc visibly unmasked, while retaining the first Skull variant.
 a=alpha[14];hidden=set(a.hidden_material_slots);materials=list(a.materials)
 for i,slot in enumerate(a.mesh.materials):
  if 'merc_mask' in str(slot.material_slot_name).lower():hidden.add(i);materials[i]=neutral
 a.set_editor_property('hidden_material_slots',sorted(hidden));a.set_editor_property('materials',materials);alpha[14]=a
 # The original lower-body atlas also contains the exposed waist. Preserve it
 # exactly as the source graph authored it, independently of cloth tint.
 for index in [10,11,12,14]:
  a=omega[index]
  for slot,material in zip(a.mesh.materials,a.materials):
   if 'lowerbody' not in str(slot.material_slot_name).lower():continue
   graph=material
   while isinstance(graph,ue.MaterialInstanceConstant):graph=graph.parent
   if assets.get_metadata_tag(graph,'ERPreserveAbdomen')=='1':continue
   nodes=list(edit.get_material_expressions(graph))
   mask=next(n for n in nodes if isinstance(n,ue.MaterialExpressionTextureSample) and 'Alpha=Skin' in str(n.get_editor_property('desc')))
   tinted=next(n for n in nodes if str(n.get_editor_property('desc'))=='ER_GroundedColor')
   desat=next(n for n in nodes if str(n.get_editor_property('desc'))=='ER_GroundedDesaturation')
   original=edit.get_inputs_for_material_expression(graph,desat)[0]
   blend=edit.create_material_expression(graph,ue.MaterialExpressionLinearInterpolate);blend.set_editor_property('desc','ER_PreserveAuthoredAbdomen')
   assert edit.connect_material_expressions(tinted,'',blend,'A')
   assert edit.connect_material_expressions(original,'BaseColor' if isinstance(original,ue.MaterialExpressionBreakMaterialAttributes) else '',blend,'B')
   assert edit.connect_material_expressions(mask,'A',blend,'Alpha')
   assert edit.connect_material_property(blend,'',ue.MaterialProperty.MP_BASE_COLOR)
   assets.set_metadata_tag(graph,'ERPreserveAbdomen','1');edit.recompile_material(graph);save(graph)
 for path in BP_PATHS:
  value=load(path);d=ue.get_default_object(value.generated_class());d.set_editor_property('alpha_skins',alpha);d.set_editor_property('omega_skins',omega)
  ue.BlueprintEditorLibrary.compile_blueprint(value);assets.set_metadata_tag(value,MARKER,VERSION);save(value)
 print('ER_MURDOCK_VARIETY_GADGET_ABDOMEN_READY')

build_variety()
runpy.run_path(str(Path(__file__).with_name('setup_murdock_gadget_palms.py')))
