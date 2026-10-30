"""Murdock review trims (29 October): black trousers, no right-arm hoses, no Merc left pauldron.

Run with -EnablePlugins=GeometryScripting after setup_murdock_recon_human_head.py.
- Skull/Infantry/Veteran trousers use Patrol's dark trouser material.
- Right forearm hoses, wrist cover and mechanical arm bones are hidden on all
  five Murdock skins (body and rebuilt first-person forearms).
- The large left-shoulder pauldron of the Merc body
  (Skull and Veteran) is deleted from the owned mesh copy.
Backup: ../_backups/murdock-trim-20261029.
"""
import unreal as u
A=u.EditorAssetLibrary;G=u.GeometryScript_MeshQueries
MARKER='ERMurdockTrim';VERSION='1'
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
# bot_hand carries the mechanical right palm and stays visible.
ARM_BONES=['arm_hose_top','arm_hose_bott','wrist_cover','bot_loarm']
CUT=.5 # fraction of shoulder->elbow kept as pauldron

def legs_black(a,legs):
 # The Merc/base trouser textures stay beige under any tint; Patrol's Shogun
 # trouser material shares the M0x_Legs UV layout and is natively dark.
 materials=list(a.materials)
 for i,slot in enumerate(a.mesh.materials):
  if 'legs' in str(slot.material_slot_name).lower():materials[i]=legs
 a.set_editor_property('materials',materials)

def remove_pauldron(mesh):
 if A.get_metadata_tag(mesh,'ERNoPauldron')=='1':return
 dm=u.DynamicMesh()
 u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(mesh,dm,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD(lod_type=u.GeometryScriptLODType.SOURCE_MODEL,lod_index=0))
 P=G.get_all_vertex_positions(dm,False)[1].convert_vector_list_to_array()
 T=G.get_all_triangle_indices(dm,False)[1].convert_triangle_list_to_array()
 parent=list(range(len(P)))
 def root(x):
  while parent[x]!=x:parent[x]=parent[parent[x]];x=parent[x]
  return x
 for t in T:
  r=root(t.x);parent[root(t.y)]=r;parent[root(t.z)]=r
 comps={}
 for i,t in enumerate(T):comps.setdefault(root(t.x),[]).append(i)
 # The pauldron is the shoulder half of the left-arm armor island (it also
 # carries the forearm guard, which stays). Split it along shoulder->elbow.
 mod=u.SkeletonModifier();assert mod.set_skeletal_mesh(mesh)
 S=mod.get_bone_transform('upperarm_l',True).translation;L=mod.get_bone_transform('lowerarm_l',True).translation
 axis=L-S;length2=axis.x**2+axis.y**2+axis.z**2
 def along(v):return ((v.x-S.x)*axis.x+(v.y-S.y)*axis.y+(v.z-S.z)*axis.z)/length2
 arm=[tris for tris in comps.values() if len(tris)>4000 and min(P[T[i].x].x for i in tris)>0 and max(P[T[i].x].x for i in tris)>55]
 assert len(arm)==1,('left arm armor',mesh.get_name(),len(arm))
 found=[[i for i in arm[0] if max(along(P[v]) for v in (T[i].x,T[i].y,T[i].z))<CUT]]
 ids=u.GeometryScript_List.convert_array_to_index_list(found[0],u.GeometryScriptIndexType.TRIANGLE)
 if isinstance(ids,tuple):ids=ids[0]
 deleted=u.GeometryScript_MeshEdits.delete_triangles_from_mesh(dm,ids)[1]
 assert deleted==len(found[0]),(deleted,len(found[0]))
 opts=u.GeometryScriptCopyMeshToAssetOptions(enable_recompute_normals=False,enable_recompute_tangents=False)
 result=u.GeometryScript_AssetUtils.copy_mesh_to_skeletal_mesh(dm,mesh,opts,u.GeometryScriptMeshWriteLOD(lod_index=0))
 assert result[1]==u.GeometryScriptOutcomePins.SUCCESS
 A.set_metadata_tag(mesh,'ERNoPauldron','1');assert A.save_loaded_asset(mesh,False)
 print('ER_MURDOCK_PAULDRON_REMOVED',mesh.get_name(),len(found[0]))

def trim():
 bp=A.load_asset(paths[0])
 if A.get_metadata_tag(bp,MARKER)==VERSION:print('ER_MURDOCK_TRIM_ALREADY_APPLIED');return
 c=u.get_default_object(bp.generated_class());skins=list(c.alpha_skins)
 fps_rig=skins[0].first_person_mesh
 patrol=skins[13];legs=next(m for s,m in zip(patrol.mesh.materials,patrol.materials) if 'legs' in str(s.material_slot_name).lower())
 for i in range(10,15):
  a=skins[i];probe=u.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(a.mesh)
  hide=[str(b) for b in a.hidden_bones];hide+=[b for b in ARM_BONES if probe.get_bone_index(b)>=0 and b not in hide]
  a.set_editor_property('hidden_bones',hide)
  if i in (10,11,14):legs_black(a,legs)
  if i in (10,14):remove_pauldron(a.mesh)
  fps=a.first_person_mesh
  # Rebuilding drops the measured palm contacts; keep the reviewed frames.
  palms=[(s.socket_name,s.bone_name,u.Transform(s.relative_location,s.relative_rotation,s.relative_scale)) for s in [fps.find_socket(n) for n in ['ER_Palm_L','ER_Palm_R']] if s]
  assert len(palms)==2,fps.get_name()
  assert u.ERCharacter.build_first_person_forearms(fps,a.mesh,fps_rig,list(a.hidden_material_slots),hide)
  for name,bone,frame in palms:assert u.ERCharacter.set_palm_contact_socket(fps,name,bone,frame)
  assert A.save_loaded_asset(fps,False)
  skins[i]=a
 for path in paths:
  value=A.load_asset(path);d=u.get_default_object(value.generated_class());d.set_editor_property('alpha_skins',skins)
  u.BlueprintEditorLibrary.compile_blueprint(value);A.set_metadata_tag(value,MARKER,VERSION);assert A.save_loaded_asset(value,False)
 print('ER_MURDOCK_TRIM_READY')

trim()
