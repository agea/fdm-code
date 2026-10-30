"""Wraith Assault head trial: hide the original head section and attach Twinblast's.

Run in the full editor with -EnablePlugins=GeometryScripting (LOD generation
needs StaticMeshEditorSubsystem). Only the new static head and three
catalogs are saved. Original skeletal meshes/FPS/animations/materials stay intact.
Restore the mask with restore_wraith_assault_mask.py (no backup required).
"""
from pathlib import Path
import json
import unreal as u
A=u.EditorAssetLibrary
p=Path(u.Paths.project_dir()).resolve()
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
c=u.get_default_object(A.load_blueprint_class(paths[0]))
donor=c.alpha_skins[4];appearance=c.alpha_skins[1]
assert donor.display_name=='Twinblast · Support' and appearance.display_name=='Wraith · Assault'
folder='/Game/EclipseRecoil/Characters/Teams/Alpha/Skins/WraithAssault/HeadStudy'
dest=folder+'/SM_ER_WraithAssault_TwinblastHead'
VERSION='6'
head=A.load_asset(dest) if A.does_asset_exist(dest) else None
if not head or A.get_metadata_tag(head,'ERTwinblastHeadExtraction') not in ['5','6']:
 dm=u.DynamicMesh()
 result=u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(donor.mesh,dm,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD(lod_type=u.GeometryScriptLODType.RENDER_DATA,lod_index=0))
 assert result[-1]==u.GeometryScriptOutcomePins.SUCCESS
 keep={'m00_head','m05_tearline','m06_eyes','m07_teethandtongue','m08_hair','m09_eyelash','m10_tear_corner','m11_eyeshadow','m12_eyebrow'}
 for i,slot in enumerate(donor.mesh.materials):
  if str(slot.material_slot_name).lower() not in keep:
   selection=u.GeometryScript_MeshSelection.select_mesh_elements_by_material_id(dm,i)[1]
   u.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(dm,selection)
 modifier=u.SkeletonModifier();assert modifier.set_skeletal_mesh(donor.mesh)
 u.GeometryScript_MeshTransforms.inverse_transform_mesh(dm,modifier.get_bone_transform('head',True))
 outside=u.GeometryScript_MeshSelection.select_mesh_elements_in_box(dm,u.Box(min=[-19.5,-13,-12],max=[21,14,12]),invert=True)[1]
 u.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(dm,outside)
 # The head/shirt material also includes separate red leather collar islands.
 # Remove those disconnected pieces below the jaw while retaining skin/neck.
 P=u.GeometryScript_MeshQueries.get_all_vertex_positions(dm,False)[1].convert_vector_list_to_array()
 T=u.GeometryScript_MeshQueries.get_all_triangle_indices(dm,False)[1].convert_triangle_list_to_array()
 parent=list(range(len(P)))
 def component_root(x):
  while parent[x]!=x:parent[x]=parent[parent[x]];x=parent[x]
  return x
 for t in T:
  if min(t.x,t.y,t.z)<0:continue
  r=component_root(t.x);parent[component_root(t.y)]=r;parent[component_root(t.z)]=r
 groups={}
 for i,t in enumerate(T):
  if min(t.x,t.y,t.z)>=0:groups.setdefault(component_root(t.x),[]).append(i)
 remove=[]
 for ids in groups.values():
  if all(u.GeometryScript_Materials.get_triangle_material_id(dm,i)[0]==0 for i in ids) and max(P[v].x for i in ids for v in [T[i].x,T[i].y,T[i].z])<-8:
   remove.extend(ids)
 assert 100<=len(remove)<=300,('Donor collar islands require review',len(remove))
 selection=u.GeometryScript_List.convert_array_to_index_list(remove,u.GeometryScriptIndexType.TRIANGLE)
 if isinstance(selection,tuple):selection=selection[0]
 assert u.GeometryScript_MeshEdits.delete_triangles_from_mesh(dm,selection)[1]==len(remove)
 # Keep only the human head/upper neck and a short donor collar transition.
 current=u.GeometryScript_MeshQueries.get_all_triangle_indices(dm,False)[1].convert_triangle_list_to_array()
 trim=[i for i,t in enumerate(current) if min(t.x,t.y,t.z)>=0 and min(P[v].x for v in [t.x,t.y,t.z])<-14.5]
 selection=u.GeometryScript_List.convert_array_to_index_list(trim,u.GeometryScriptIndexType.TRIANGLE)
 if isinstance(selection,tuple):selection=selection[0]
 u.GeometryScript_MeshEdits.delete_triangles_from_mesh(dm,selection)
 compact=getattr(u.GeometryScript_Materials,next(n for n in dir(u.GeometryScript_Materials) if n.startswith('compact_material')))
 materials=compact(dm,list(donor.materials))[1]
 assert 5<=len(materials)<=9,(len(materials),materials)
 A.make_directory(folder)
 if not head:head=A.duplicate_asset('/Engine/BasicShapes/Cube',dest)
 options=u.GeometryScriptCopyMeshToAssetOptions(replace_materials=True,new_materials=materials,new_material_slot_names=[str(m.get_name()) for m in materials],enable_recompute_normals=False,enable_recompute_tangents=False)
 result=u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(dm,head,options,u.GeometryScriptMeshWriteLOD(lod_index=0))
 assert result[1]==u.GeometryScriptOutcomePins.SUCCESS
 reductions=[u.StaticMeshReductionSettings(percent_triangles=f,screen_size=s) for f,s in [(1.,1.),(.6,.12),(.3,.05),(.12,.02)]]
 assert u.get_editor_subsystem(u.StaticMeshEditorSubsystem).set_lods(head,u.StaticMeshReductionOptions(auto_compute_lod_screen_size=False,reduction_settings=reductions))==4
 A.set_metadata_tag(head,'ERHeadCollarTrianglesRemoved',str(len(remove)))
 A.set_metadata_tag(head,'ERTwinblastHeadExtraction',VERSION);assert A.save_loaded_asset(head,False)
# The donor's shared head/shirt shader calls its red collar pigment ArmColor.
# Override only that cloth parameter in a new instance; keep all skin inputs.
mi_path=folder+'/MI_ER_TwinblastHead_WraithCollar'
mi=A.load_asset(mi_path) if A.does_asset_exist(mi_path) else u.AssetToolsHelpers.get_asset_tools().create_asset('MI_ER_TwinblastHead_WraithCollar',folder,u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
E=u.MaterialEditingLibrary;E.set_material_instance_parent(mi,donor.materials[0])
E.set_material_instance_vector_parameter_value(mi,'ArmColor',u.LinearColor(.04,.06,.09,1))
# UE5.8's setter returns false unconditionally; verify the stored value instead.
color=E.get_material_instance_vector_parameter_value(mi,'ArmColor')
assert all(abs(a-b)<1e-5 for a,b in zip([color.r,color.g,color.b],[.04,.06,.09]))
E.update_material_instance(mi);assert A.save_loaded_asset(mi,False)
slots=list(head.static_materials);slots[0].set_editor_property('material_interface',mi);head.set_editor_property('static_materials',slots)
A.set_metadata_tag(head,'ERWraithCollarColor','NeutralBlue');assert A.save_loaded_asset(head,False)
# Keep this first trial reversible: hide the entire original mask/head section
# and retain the donor's upper neck and neutral-colored collar.
neutral=u.load_asset('/Game/EclipseRecoil/Characters/Teams/Gear/M_ER_HiddenSection')
mask=next(i for i,s in enumerate(appearance.mesh.materials) if str(s.material_slot_name)=='M02_backpackhead')
hidden=set(appearance.hidden_material_slots);hidden.add(mask)
materials=list(appearance.materials);materials[mask]=neutral
A.set_metadata_tag(head,'ERTwinblastHeadExtraction',VERSION)
A.set_metadata_tag(head,'ERWraithNeckGuardTriangles','0');A.set_metadata_tag(head,'ERNeckGuardBackpackTrianglesExcluded','0')
assert A.save_loaded_asset(head,False)
appearance.set_editor_property('hidden_material_slots',sorted(hidden));appearance.set_editor_property('materials',materials)
appearance.set_editor_property('attachments',[u.ERAppearanceAttachment(mesh=head,bone='head',transform=u.Transform())])
for path in paths:
 bp=A.load_asset(path);d=u.get_default_object(bp.generated_class());alpha=list(d.alpha_skins);alpha[1]=appearance;d.set_editor_property('alpha_skins',alpha)
 u.BlueprintEditorLibrary.compile_blueprint(bp);A.set_metadata_tag(bp,'ERWraithTwinblastHead',VERSION);assert A.save_loaded_asset(bp,False)
retired=folder+'/M_ER_WraithAssault_NeckOnly'
if A.does_asset_exist(retired) and not A.find_package_referencers_for_asset(retired,load_assets_to_confirm=True):
 assert A.delete_asset(retired)
out=p/'Saved/Testing/WraithHeadStudy';out.mkdir(parents=True,exist_ok=True)
(out/'setup.json').write_text(json.dumps({'skin_index':1,'skin_name':appearance.display_name,'head':head.get_path_name(),'donor':donor.mesh.get_path_name(),'hidden_slots':sorted(hidden),'head_bounds':str(head.get_bounding_box()),'attachment_bone':'head','skeletal_meshes_modified':False,'neck_transition':'donor upper neck and neutral collar'},indent=2))
u.log('ER_WRAITH_TWINBLAST_HEAD_READY skin=1 skeletal_meshes_preserved=1')
