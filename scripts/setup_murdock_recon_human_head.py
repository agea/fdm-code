"""Use Murdock's standard helmeted head on the Corrupt-derived Recon body.

Run with -EnablePlugins=GeometryScripting after setup_murdock_gadget_variety.py.
The head is rigidly attached to the existing head bone, including on corpses.
"""
from pathlib import Path
import unreal as u
A=u.EditorAssetLibrary;E=u.MaterialEditingLibrary
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
c=u.get_default_object(A.load_blueprint_class(paths[0]));source=c.alpha_skins[11];a=c.alpha_skins[12]
folder='/Game/EclipseRecoil/Characters/Teams/Alpha/Skins/MurdockRecon/Variety2';dest=folder+'/SM_ER_MurdockRecon_HumanHead'
if not A.does_asset_exist(dest) or A.get_metadata_tag(A.load_asset(dest),'ERFullHeadGeometry')!='1':
 dm=u.DynamicMesh();u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(source.mesh,dm,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD(lod_type=u.GeometryScriptLODType.RENDER_DATA,lod_index=0))
 # The source face slot contains eyes/mouth fragments; the skull/helmet also
 # uses the body slot. Keep the complete authored head region and omit glass.
 for slot in source.hidden_material_slots:
  selection=u.GeometryScript_MeshSelection.select_mesh_elements_by_material_id(dm,slot)[1]
  u.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(dm,selection)
 modifier=u.SkeletonModifier();assert modifier.set_skeletal_mesh(source.mesh)
 head=modifier.get_bone_transform('head',True);u.GeometryScript_MeshTransforms.inverse_transform_mesh(dm,head)
 outside=u.GeometryScript_MeshSelection.select_mesh_elements_in_box(dm,u.Box(min=[-7,-12,-12.5],max=[17,14,12.5]),invert=True)[1]
 u.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(dm,outside)
 mesh=A.load_asset(dest) if A.does_asset_exist(dest) else A.duplicate_asset('/Engine/BasicShapes/Cube',dest)
 exec(Path(__file__).with_name('setup_murdock_gadget.py').read_text().split('\ndef build():')[0])
 mats=list(source.materials)
 mats[0]=finish(u.load_asset('/Game/ParagonMurdock/Characters/Heroes/Murdock/Materials/M_Murdock_Body'),folder,'Alpha','HumanHeadBody',(1,1,1),True)
 opts=u.GeometryScriptCopyMeshToAssetOptions(replace_materials=True,new_materials=mats,new_material_slot_names=[str(s.material_slot_name) for s in source.mesh.materials])
 result=u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(dm,mesh,opts,u.GeometryScriptMeshWriteLOD(lod_index=0));assert result[1]==u.GeometryScriptOutcomePins.SUCCESS
 A.set_metadata_tag(mesh,'ERFullHeadGeometry','1');assert A.save_loaded_asset(mesh,False)
else:mesh=A.load_asset(dest)
neutral=A.load_asset('/Game/EclipseRecoil/Characters/Teams/Gear/M_ER_HiddenSection');hidden=set(a.hidden_material_slots);materials=list(a.materials)
for i,s in enumerate(a.mesh.materials):
 low=str(s.material_slot_name).lower()
 if any(w in low for w in ['head','eye']):hidden.add(i);materials[i]=neutral
 if 'arm_skin' in low:
  graph=materials[i]
  while isinstance(graph,u.MaterialInstanceConstant):graph=graph.parent
  color=next((n for n in E.get_material_expressions(graph) if str(n.get_editor_property('desc'))=='ER_ReconNaturalArmColor'),None) or E.create_material_expression(graph,u.MaterialExpressionConstant3Vector)
  color.set_editor_property('desc','ER_ReconNaturalArmColor');color.set_editor_property('constant',u.LinearColor(.30,.20,.14,1))
  assert E.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
  zero=next((n for n in E.get_material_expressions(graph) if str(n.get_editor_property('desc'))=='ER_ReconArmNoSubsurface'),None) or E.create_material_expression(graph,u.MaterialExpressionConstant)
  zero.set_editor_property('desc','ER_ReconArmNoSubsurface');zero.set_editor_property('r',0);E.connect_material_property(zero,'',u.MaterialProperty.MP_SUBSURFACE_COLOR)
  E.recompile_material(graph);assert A.save_loaded_asset(graph,False)
a.set_editor_property('hidden_material_slots',sorted(hidden));a.set_editor_property('materials',materials)
a.set_editor_property('attachments',[u.ERAppearanceAttachment(mesh=mesh,bone='head',transform=u.Transform())])
for body in [a.mesh,a.first_person_mesh]:
 slots=list(body.materials)
 for slot,mat in zip(slots,materials):slot.set_editor_property('material_interface',mat)
 body.set_editor_property('materials',slots);assert A.save_loaded_asset(body,False)
for path in paths:
 value=A.load_asset(path);d=u.get_default_object(value.generated_class());skins=list(d.alpha_skins);skins[12]=a;d.set_editor_property('alpha_skins',skins)
 u.BlueprintEditorLibrary.compile_blueprint(value);assert A.save_loaded_asset(value,False)
print('ER_MURDOCK_RECON_NATURAL_HEAD_READY')
