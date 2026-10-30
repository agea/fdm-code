"""Replace Alpha0/1/2 with three owned Wraith bodies, preserving all other skins.

Import the audited source closure first. Old Twinblast art stays available for
the later face study. This migration keeps Wraith's original masked heads.
"""
from pathlib import Path
import json,runpy
import unreal as ue
exec(Path(__file__).with_name('setup_murdock_gadget.py').read_text().split('\ndef build():')[0])
project=Path(ue.Paths.project_dir()).resolve()
MARKER='ERWraithReplacement';VERSION='1'
base='/Game/ParagonWraith/Characters/Heroes/Wraith/'
specs=[
 (0,'Officer',base+'Skins/ODGreen/Meshes/Wraith_ODGreen',(.14,.32,.62),(.39,.42,.44)),
 (1,'Assault',base+'Meshes/Wraith',(.065,.19,.43),(.11,.14,.17)),
 (2,'Patrol',base+'Skins/LunarOps/Meshes/Wraith_LunarOps',(.22,.39,.64),(.26,.30,.33)),
]

def make_animations(source):
 folder=root+'/Alpha/Wraith';assets.make_directory(folder)
 skeleton=copy(source.skeleton,folder+'/SKEL_ER_Wraith')
 skeleton.add_compatible_skeleton(source.skeleton);save(skeleton)
 preview=copy(source,folder+'/SK_ER_Wraith_Retarget');preview.skeleton=skeleton;save(preview)
 output=folder+'/Animations';anim_path=output+'/ER_Wraith_ABP_TP_Rifle';idle_path=output+'/ER_Wraith_MF_Rifle_Idle_ADS'
 if not assets.does_asset_exist(anim_path):
  target=make_rig(preview,'IK_ER_Wraith')
  rt=assets.load_asset(root+'/RTG_ER_Manny_Wraith') if assets.does_asset_exist(root+'/RTG_ER_Manny_Wraith') else tools.create_asset('RTG_ER_Manny_Wraith',root,ue.IKRetargeter,ue.IKRetargetFactory())
  ctrl=ue.IKRetargeterController.get_controller(rt)
  ctrl.set_ik_rig(ue.RetargetSourceOrTarget.SOURCE,load(root+'/IK_ER_Manny'))
  ctrl.set_ik_rig(ue.RetargetSourceOrTarget.TARGET,target)
  if not ctrl.get_num_retarget_ops():ctrl.add_default_ops()
  ctrl.auto_map_chains(ue.AutoMapChainType.FUZZY,True);ctrl.auto_align_all_bones(ue.RetargetSourceOrTarget.TARGET);save(rt)
  opts=ue.IKRetargetBatchOperationInputs()
  for k,v in dict(assets_to_retarget=[assets.find_asset_data('/Game/Variant_Shooter/Anims/ABP_TP_Rifle'),assets.find_asset_data('/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS')],source_mesh=load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple'),target_mesh=preview,ik_retarget_asset=rt,prefix='ER_Wraith_',target_path=output,include_referenced_assets=True,overwrite_existing_files=False).items():opts.set_editor_property(k,v)
  ue.IKRetargetBatchOperation.run_batch_retarget(opts)
  for path in assets.list_assets(output,recursive=True):
   value=load(path)
   if isinstance(value,ue.AnimBlueprint):ue.BlueprintEditorLibrary.compile_blueprint(value)
   save(value)
 return skeleton,assets.load_blueprint_class(anim_path),load(idle_path),preview

def measure_palms(mesh):
 geometry=runpy.run_path(str(Path(__file__).with_name('palm_geometry.py')))
 xyz=lambda v:(v.x,v.y,v.z)
 for side in ['l','r']:
  bone='hand_'+side;frame=ue.ERCharacter.get_weapon_reference_bone(mesh,bone)
  points=[xyz(v) for v in ue.ERCharacter.get_presentation_vertices(mesh,bone)]
  fingers={f:xyz(ue.MathLibrary.inverse_transform_location(frame,ue.ERCharacter.get_weapon_reference_bone(mesh,f+'_01_'+side).translation)) for f in ['index','middle','ring','pinky','thumb']}
  fit=geometry['palm_surface'](points,fingers)
  rotation=ue.MathLibrary.make_rot_from_xz(ue.Vector(*fit['x']),ue.Vector(*fit['z']))
  assert ue.ERCharacter.set_palm_contact_socket(mesh,'ER_Palm_'+side.upper(),bone,ue.Transform(location=ue.Vector(*fit['position']),rotation=rotation))
 save(mesh)

def preserve_scales(mesh,output):
 ref=mesh.skeleton.get_reference_pose();probe=ue.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(mesh)
 scales={str(b):ue.AnimPoseExtensions.get_bone_pose(ref,b,ue.AnimPoseSpaces.LOCAL).scale3d for b in ue.AnimPoseExtensions.get_bone_names(ref) if probe.get_bone_index(b)>=0}
 scales={b:s for b,s in scales.items() if any(abs(v-1)>.0001 for v in (s.x,s.y,s.z))}
 if not scales:return
 for path in assets.list_assets(output,recursive=True):
  clip=load(path)
  if not isinstance(clip,ue.AnimSequence):continue
  ctrl=clip.get_editor_property('controller');model=ctrl.get_model_interface();rate=model.get_frame_rate()
  opts=ue.AnimPoseEvaluationOptions();opts.optional_skeletal_mesh=mesh;opts.should_retarget=False
  poses=[ue.AnimPoseExtensions.get_anim_pose_at_time(clip,f*rate.denominator/rate.numerator,opts) for f in range(model.get_number_of_frames()+1)]
  ctrl.open_bracket('Preserve authored Wraith auxiliary bone scales',False)
  try:
   for bone,scale in scales.items():
    keys=[ue.AnimPoseExtensions.get_bone_pose(p,bone,ue.AnimPoseSpaces.LOCAL) for p in poses]
    if bone.lower() not in [str(b).lower() for b in model.get_bone_track_names()]:assert ctrl.add_bone_curve(bone,False)
    assert ctrl.set_bone_track_keys(bone,[k.translation for k in keys],[k.rotation for k in keys],[scale]*len(keys),False)
  finally:ctrl.close_bracket(False)
  save(clip)

def build():
 bp=load(BP_PATHS[0])
 if assets.get_metadata_tag(bp,MARKER)==VERSION:return
 catalog=ue.get_default_object(bp.generated_class());alpha=list(catalog.alpha_skins);assert len(alpha)==10
 assert [a.display_name for a in alpha[:3]]==['Twinblast · Officer','Twinblast · Assault','Twinblast · Patrol']
 rig=alpha[0].first_person_mesh;manny=load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
 skeleton,anim,idle,preview=make_animations(load(specs[1][2]));assert anim
 neutral=load(root+'/Gear/M_ER_HiddenSection');receipt=[]
 for index,role,source_path,upper,lower in specs:
  folder=root+'/Alpha/Skins/Wraith'+role;assets.make_directory(folder);source=load(source_path)
  mesh=copy(source,folder+'/SK_ER_Wraith_Alpha_'+role);mesh.skeleton=skeleton
  physics=copy(load(base+'Meshes/Wraith_Extents'),folder+'/PA_ER_Wraith'+role+'_Ragdoll')
  assert ue.ERCorpse.build_team_ragdoll_physics(physics,mesh,limb_swing=55,leg_swing=45)
  assert ue.ERCharacter.copy_training_grip_sockets(mesh,manny);save(physics)
  probe=ue.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(mesh)
  hidden_bones=[b for b in ['backpack','canon_gun','weapon_l','weapon_r'] if probe.get_bone_index(b)>=0]
  slots=list(mesh.materials);materials=[];hidden=[]
  for i,slot in enumerate(slots):
   name=str(slot.material_slot_name);low=name.lower()
   if 'weapon' in low:material=neutral;hidden.append(i)
   else:
    tint=upper if 'upperbody' in low else lower if 'lowerbody' in low else (.19,.23,.29)
    material=finish(slot.material_interface,folder,'Alpha',name,tint)
    graph=material
    while isinstance(graph,ue.MaterialInstanceConstant):graph=graph.parent
    if 'upperbody' in low:
     nodes={str(n.get_editor_property('desc')):n for n in edit.get_material_expressions(graph)}
     lift=nodes.get('ER_WraithClothLift') or edit.create_material_expression(graph,ue.MaterialExpressionLinearInterpolate)
     lift.set_editor_property('desc','ER_WraithClothLift');lift.set_editor_property('const_b',.65);lift.set_editor_property('const_alpha',.20)
     assert edit.connect_material_expressions(nodes['ER_GroundedDesaturation'],'',lift,'A')
     assert edit.connect_material_expressions(lift,'',nodes['ER_GroundedColor'],'A')
     edit.recompile_material(graph);save(graph)
   materials.append(material);slot.set_editor_property('material_interface',material)
  mesh.set_editor_property('materials',slots);save(mesh)
  fps=copy(source,root+'/Alpha/FirstPerson/SK_ER_Wraith_Alpha_'+role+'_Forearms')
  assert ue.ERCharacter.build_first_person_forearms(fps,mesh,rig,hidden,hidden_bones);save(fps)
  measure_palms(mesh);measure_palms(fps)
  ref=skeleton.get_reference_pose();pos=lambda b:ue.AnimPoseExtensions.get_bone_pose(ref,b,ue.AnimPoseSpaces.WORLD).translation
  feet=min(pos('foot_l').z,pos('foot_r').z)-6;scale=185/(pos('head').z+12-feet)
  a=ue.ERTeamAppearance()
  for key,value in dict(display_name='Wraith · '+role,mesh=mesh,first_person_mesh=fps,animation_class=anim,corpse_pose=idle,materials=materials,hidden_bones=hidden_bones,hidden_material_slots=hidden,attachments=[],copy_physics_pose_for_rendering=True,scale=scale,feet_offset=-92.5-feet*scale).items():a.set_editor_property(key,value)
  alpha[index]=a;receipt.append(dict(index=index,name=a.display_name,source=source_path,mesh=mesh.get_path_name(),hidden_bones=hidden_bones,hidden_slots=hidden))
 preserve_scales(alpha[1].mesh,root+'/Alpha/Wraith/Animations')
 assert ue.ERCharacter.prepare_team_skeleton(skeleton,alpha[0].mesh);save(skeleton)
 for path in assets.list_assets(root+'/Alpha/Wraith/Animations',recursive=True):
  value=load(path);assert ue.ERCharacter.replace_owned_asset_references(value,[preview],[alpha[0].mesh])
  if isinstance(value,ue.AnimBlueprint):ue.BlueprintEditorLibrary.compile_blueprint(value)
  save(value)
 for path in BP_PATHS:
  value=load(path);c=ue.get_default_object(value.generated_class())
  c.set_editor_property('alpha_skins',alpha);c.set_editor_property('alpha_appearance',alpha[0])
  if hasattr(c,'alpha_body_materials'):c.set_editor_property('alpha_body_materials',alpha[0].materials)
  if hasattr(c,'alpha_first_person_materials'):c.set_editor_property('alpha_first_person_materials',alpha[0].materials)
  if hasattr(c,'first_person_body'):c.first_person_body.set_skeletal_mesh_asset(alpha[0].first_person_mesh)
  ue.BlueprintEditorLibrary.compile_blueprint(value);assets.set_metadata_tag(value,MARKER,VERSION);save(value)
 out=project/'Saved/Testing/WraithReplacement';out.mkdir(parents=True,exist_ok=True)
 (out/'setup.json').write_text(json.dumps(receipt,indent=2))
 ue.log('ER_WRAITH_REPLACEMENT_READY skins=3 indices=0,1,2 masked_heads=1')

build()
