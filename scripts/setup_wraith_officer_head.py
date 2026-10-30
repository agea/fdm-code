"""Give Wraith Officer the retained Twinblast Officer sunglasses head.
Run in full Unreal editor with GeometryScripting. Source art and FPS stay intact.
"""
from pathlib import Path
import hashlib,json
import unreal as u
A=u.EditorAssetLibrary;G=u.GeometryScript_MeshQueries
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
c=u.get_default_object(A.load_blueprint_class(paths[0]));appearance=c.alpha_skins[0]
p=Path(u.Paths.project_dir()).resolve();out=p/'Saved/Testing/WraithOfficerHead';out.mkdir(parents=True,exist_ok=True)
folder='/Game/EclipseRecoil/Characters/Teams/Alpha/Skins/WraithOfficer/HeadStudy'
source=A.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/Skins/WraithOfficer/SK_ER_Wraith_Alpha_Officer')
donor=A.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/Skins/Officer/SK_ER_Twinblast_Alpha_Officer')
protected={str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for base in ['Assets/characters/Content/EclipseRecoil/Characters/Teams/Alpha/Skins/WraithAssault','Assets/characters/Content/EclipseRecoil/Characters/Teams/Alpha/Skins/WraithPatrol','Assets/characters/Content/EclipseRecoil/Characters/Teams/Alpha/Skins/Officer'] for f in (p/base).rglob('*.uasset')}
(out/'protected.json').write_text(json.dumps(protected,indent=2))
def delete(dm,ids):
 selection=u.GeometryScript_List.convert_array_to_index_list(ids,u.GeometryScriptIndexType.TRIANGLE)
 if isinstance(selection,tuple):selection=selection[0]
 assert u.GeometryScript_MeshEdits.delete_triangles_from_mesh(dm,selection)[1]==len(ids)
def read(mesh,lodtype):
 dm=u.DynamicMesh();r=u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(mesh,dm,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD(lod_type=lodtype,lod_index=0));assert r[-1]==u.GeometryScriptOutcomePins.SUCCESS
 return dm
A.make_directory(folder)
headpath=folder+'/SM_ER_WraithOfficer_TwinblastHead'
head=A.load_asset(headpath) if A.does_asset_exist(headpath) else None
if not head or A.get_metadata_tag(head,'EROfficerSunglassesHead')!='3':
 dm=read(donor,u.GeometryScriptLODType.RENDER_DATA)
 keep={'m00_upperbody','m04_head','m05_tearline','m06_eyes','m07_teethandtongue','m08_hair','m09_eyelash','m11_eyeshadow','m12_eyebrow'}
 for i,slot in enumerate(donor.materials):
  if str(slot.material_slot_name).lower() not in keep:
   selection=u.GeometryScript_MeshSelection.select_mesh_elements_by_material_id(dm,i)[1];u.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(dm,selection)
 mod=u.SkeletonModifier();assert mod.set_skeletal_mesh(donor)
 u.GeometryScript_MeshTransforms.inverse_transform_mesh(dm,mod.get_bone_transform('head',True))
 selection=u.GeometryScript_MeshSelection.select_mesh_elements_in_box(dm,u.Box(min=[-14.5,-13,-12],max=[21,14,12]),invert=True)[1]
 u.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(dm,selection)
 # Keep the sunglasses island, discarding the donor's loose shirt collar and cigarette.
 P=G.get_all_vertex_positions(dm,False)[1].convert_vector_list_to_array()
 T=G.get_all_triangle_indices(dm,False)[1].convert_triangle_list_to_array()
 shirt=next(i for i,s in enumerate(donor.materials) if str(s.material_slot_name).lower()=='m00_upperbody')
 parent=list(range(len(P)))
 def root(v):
  while parent[v]!=v:parent[v]=parent[parent[v]];v=parent[v]
  return v
 ids=[i for i,t in enumerate(T) if min(t.x,t.y,t.z)>=0 and u.GeometryScript_Materials.get_triangle_material_id(dm,i)[0]==shirt]
 for i in ids:
  t=T[i];r=root(t.x);parent[root(t.y)]=r;parent[root(t.z)]=r
 groups={}
 for i in ids:groups.setdefault(root(T[i].x),[]).append(i)
 remove=[i for group in groups.values() if max(P[v].x for j in group for v in [T[j].x,T[j].y,T[j].z])<0 for i in group]
 delete(dm,remove)
 compact=getattr(u.GeometryScript_Materials,next(n for n in dir(u.GeometryScript_Materials) if n.startswith('compact_material')))
 mats=compact(dm,[s.material_interface for s in donor.materials])[1]
 if not head:head=A.duplicate_asset('/Engine/BasicShapes/Cube',headpath)
 options=u.GeometryScriptCopyMeshToAssetOptions(replace_materials=True,new_materials=mats,new_material_slot_names=[str(m.get_name()) for m in mats],enable_recompute_normals=False,enable_recompute_tangents=False)
 assert u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(dm,head,options,u.GeometryScriptMeshWriteLOD(lod_index=0))[1]==u.GeometryScriptOutcomePins.SUCCESS
 reductions=[u.StaticMeshReductionSettings(percent_triangles=f,screen_size=s) for f,s in [(1.,1.),(.6,.12),(.3,.05),(.12,.02)]]
 assert u.get_editor_subsystem(u.StaticMeshEditorSubsystem).set_lods(head,u.StaticMeshReductionOptions(auto_compute_lod_screen_size=False,reduction_settings=reductions))==4
 A.set_metadata_tag(head,'EROfficerSunglassesHead','3');assert A.save_loaded_asset(head,False)
bodypath=folder+'/SK_ER_WraithOfficer_Unmasked'
body=A.load_asset(bodypath) if A.does_asset_exist(bodypath) else A.duplicate_asset(source.get_path_name().split('.')[0],bodypath)
if A.get_metadata_tag(body,'EROfficerHelmetRemoved')!='1':
 dm=read(source,u.GeometryScriptLODType.SOURCE_MODEL)
 P=G.get_all_vertex_positions(dm,False)[1].convert_vector_list_to_array();T=G.get_all_triangle_indices(dm,False)[1].convert_triangle_list_to_array()
 # Scalloped costume helmet is a separate rigid accessory above the collar.
 # Scope the cut to its material and head region; retain all torso/limb armor.
 ids=[i for i,t in enumerate(T) if min(t.x,t.y,t.z)>=0 and u.GeometryScript_Materials.get_triangle_material_id(dm,i)[0]==0 and all(P[v].z>163 and abs(P[v].x)<14 and abs(P[v].y)<22 for v in [t.x,t.y,t.z])]
 assert 3000<len(ids)<10000,len(ids)
 delete(dm,ids)
 opts=u.GeometryScriptCopyMeshToAssetOptions(enable_recompute_normals=False,enable_recompute_tangents=False)
 assert u.GeometryScript_AssetUtils.copy_mesh_to_skeletal_mesh(dm,body,opts,u.GeometryScriptMeshWriteLOD(lod_index=0))[1]==u.GeometryScriptOutcomePins.SUCCESS
 assert u.SkeletalMeshEditorSubsystem.regenerate_lod(body,5,True,False)
 A.set_metadata_tag(body,'EROfficerHelmetRemoved','1');A.set_metadata_tag(body,'EROfficerHelmetTrianglesRemoved',str(len(ids)));assert A.save_loaded_asset(body,False)
assert body.get_editor_property('skeleton')==source.get_editor_property('skeleton')
assert body.get_editor_property('physics_asset')==source.get_editor_property('physics_asset')
neutral=A.load_asset('/Game/EclipseRecoil/Characters/Teams/Gear/M_ER_HiddenSection')
mask=next(i for i,s in enumerate(source.materials) if str(s.material_slot_name)=='M02_backpackhead')
materials=list(appearance.materials);materials[mask]=neutral
appearance.set_editor_property('mesh',body);appearance.set_editor_property('materials',materials)
appearance.set_editor_property('hidden_material_slots',sorted(set(appearance.hidden_material_slots)|{mask}))
appearance.set_editor_property('attachments',[u.ERAppearanceAttachment(mesh=head,bone='head',transform=u.Transform())])
for path in paths:
 bp=A.load_asset(path);d=u.get_default_object(bp.generated_class());alpha=list(d.alpha_skins);alpha[0]=appearance;d.set_editor_property('alpha_skins',alpha);d.set_editor_property('alpha_appearance',appearance)
 u.BlueprintEditorLibrary.compile_blueprint(bp);A.set_metadata_tag(bp,'ERWraithOfficerHead','1');assert A.save_loaded_asset(bp,False)
for rel,expected in protected.items():assert hashlib.sha256((p/rel).read_bytes()).hexdigest()==expected,rel
(out/'setup.json').write_text(json.dumps({'head':head.get_path_name(),'body':body.get_path_name(),'removed_helmet_triangles':A.get_metadata_tag(body,'EROfficerHelmetTrianglesRemoved'),'head_materials':[s.material_interface.get_path_name() for s in head.static_materials],'protected_packages':len(protected)},indent=2))
u.log('ER_WRAITH_OFFICER_HEAD_READY')
