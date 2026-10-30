"""Restore Scavenger's own head on the active Jackal preset; preserve other skins."""
from pathlib import Path
import json,shutil,hashlib
import unreal as u
A=u.EditorAssetLibrary;p=Path(u.Paths.project_dir()).resolve();out=p/'Saved/Testing/DrongoScavenger';out.mkdir(parents=True,exist_ok=True)
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
protected={str(f.relative_to(p)):hashlib.sha256(f.read_bytes()).hexdigest() for base in ['Assets/characters/Content/EclipseRecoil/Characters/Teams/Omega','Assets/characters/Content/ParagonDrongo'] for f in (p/base).rglob('*.uasset')}
if (out/'protected.json').exists():protected=json.loads((out/'protected.json').read_text())
else:(out/'protected.json').write_text(json.dumps(protected))
# The historical import stored this shared function under _2, while the mask
# still references the unsuffixed path. Restore that alias without changing it.
fur='/Game/ParagonDrongo/Characters/Global/MaterialLayers/Fabric/ML_Fur'
if not A.does_asset_exist(fur):
 alias=A.duplicate_asset(fur+'_2',fur);assert alias;assert A.save_loaded_asset(alias,False)
rubber='/Game/ParagonDrongo/Characters/Global/MaterialLayers/Rubber/T_Rubber'
if not A.does_asset_exist(rubber):
 alias=A.duplicate_asset('/Game/ParagonTwinblast/Characters/Global/MaterialLayers/Rubber/T_Rubber_2',rubber);assert alias;assert A.save_loaded_asset(alias,False)
# Preserve the full authored attribute graph: generic matte conversion drops
# surface attributes used by this layered mask. Only repair the missing default.
folder='/Game/EclipseRecoil/Characters/Teams/Omega/Skins/Scavenger'
ns={};exec((p/'scripts/setup_team_characters.py').read_text().split('\nmanny =')[0],ns)
E=u.MaterialEditingLibrary
fixed=ns['copy'](A.load_asset('/Game/ParagonDrongo/Characters/Global/MaterialLayers/Rubber/ML_Rubber'),folder+'/MF_ER_Scavenger_Rubber')
for node in E.get_material_function_expressions(fixed):
 if isinstance(node,u.MaterialExpressionTextureObject) and not node.get_editor_property('texture'):node.set_editor_property('texture',A.load_asset(rubber))
E.update_material_function(fixed);assert A.save_loaded_asset(fixed,False)
layer=ns['copy'](A.load_asset('/Game/ParagonDrongo/Characters/Heroes/Drongo/Skins/Scavenger/Materials/Layer/MF_Drongo_Scavenger_Rubber'),folder+'/MF_ER_Scavenger_RubberLayer')
for node in E.get_material_function_expressions(layer):
 if isinstance(node,u.MaterialExpressionMaterialFunctionCall):assert node.set_material_function(fixed)
E.update_material_function(layer);assert A.save_loaded_asset(layer,False)
quality=ns['copy'](A.load_asset('/Game/ParagonDrongo/Characters/Global/MaterialFunctions/MF_Character_MaterialLayerQuality'),folder+'/MF_ER_Scavenger_FullDetail')
for node in E.get_material_function_expressions(quality):
 if isinstance(node,u.MaterialExpressionQualitySwitch):
  inputs=E.get_inputs_for_material_function_expression(quality,node)
  assert inputs[0],list(inputs)
  assert E.connect_material_expressions(inputs[0],'',node,'Low')
E.update_material_function(quality);assert A.save_loaded_asset(quality,False)
source=A.load_asset('/Game/ParagonDrongo/Characters/Heroes/Drongo/Skins/Scavenger/Materials/M_Drongo_Scavenger_FaceMask');assert source
face=ns['copy'](source,folder+'/M_ER_Scavenger_FieldMask')
for node in E.get_material_expressions(face):
 if isinstance(node,u.MaterialExpressionMaterialFunctionCall):
  function=node.get_editor_property('material_function')
  if function and function.get_name() in ['MF_Drongo_Scavenger_Rubber','MF_ER_Scavenger_RubberLayer']:assert node.set_material_function(layer)
  if function and function.get_name() in ['MF_Character_MaterialLayerQuality','MF_ER_Scavenger_FullDetail']:assert node.set_material_function(quality)
if A.get_metadata_tag(face,'ERScavengerPrintedMask')!='1':
 attr=E.get_material_property_input_node(face,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES);output=E.get_material_property_input_node_output_name(face,u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
 split=E.create_material_expression(face,u.MaterialExpressionBreakMaterialAttributes);assert E.connect_material_expressions(attr,output,split,'Attr')
 face.set_editor_property('use_material_attributes',False)
 for pin,prop in [('Normal',u.MaterialProperty.MP_NORMAL),('Specular',u.MaterialProperty.MP_SPECULAR),('AmbientOcclusion',u.MaterialProperty.MP_AMBIENT_OCCLUSION),('Opacity',u.MaterialProperty.MP_OPACITY),('OpacityMask',u.MaterialProperty.MP_OPACITY_MASK)]:assert E.connect_material_property(split,pin,prop)
 def vector(color):
  n=E.create_material_expression(face,u.MaterialExpressionConstant3Vector);n.set_editor_property('constant',u.LinearColor(*color,1));return n
 def sample(name):
  n=E.create_material_expression(face,u.MaterialExpressionTextureSample);n.set_editor_property('texture',A.load_asset('/Game/ParagonDrongo/Characters/Heroes/Drongo/Skins/Scavenger/Textures/'+name));return n
 def connect(a,pin,b,inp):assert E.connect_material_expressions(a,pin,b,inp)
 pattern=sample('T_Drongo_Scavenger_Upperbody_Pattern')
 paint=E.create_material_expression(face,u.MaterialExpressionLinearInterpolate);connect(split,'BaseColor',paint,'A');connect(vector((.8,.8,.75)),'',paint,'B');connect(pattern,'R',paint,'Alpha')
 m2=sample('T_Drongo_Scavenger_Face_Mask02');m3=sample('T_Drongo_Scavenger_Face_Mask03')
 union=E.create_material_expression(face,u.MaterialExpressionMax);connect(m2,'A',union,'A');connect(m3,'A',union,'B')
 lenses=E.create_material_expression(face,u.MaterialExpressionMax);connect(union,'',lenses,'A');connect(m3,'B',lenses,'B')
 dark=E.create_material_expression(face,u.MaterialExpressionLinearInterpolate);connect(paint,'',dark,'A');connect(vector((.012,.015,.014)),'',dark,'B');connect(lenses,'',dark,'Alpha');assert E.connect_material_property(dark,'',u.MaterialProperty.MP_BASE_COLOR)
 for prop,value in [(u.MaterialProperty.MP_EMISSIVE_COLOR,0),(u.MaterialProperty.MP_ROUGHNESS,.68),(u.MaterialProperty.MP_METALLIC,.05)]:
  n=E.create_material_expression(face,u.MaterialExpressionConstant);n.set_editor_property('r',value);assert E.connect_material_property(n,'',prop)
 face.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
 A.set_metadata_tag(face,'ERScavengerPrintedMask','1')
face.set_editor_property('used_with_skeletal_mesh',True);E.recompile_material(face);assert A.save_loaded_asset(face,False)
for path in paths:
 bp=A.load_asset(path);d=u.get_default_object(bp.generated_class());skins=list(d.omega_skins);a=skins[1]
 assert a.display_name=='Drongo · Jackal' and a.mesh.get_name()=='SK_ER_Drongo_Omega_Scavenger'
 file=p/'Content'/(path.removeprefix('/Game/')+'.uasset');backup=out/(bp.get_name()+'-before.uasset')
 if not backup.exists():shutil.copyfile(file,backup)
 m=list(a.materials);m[0]=face;a.set_editor_property('materials',m)
 a.set_editor_property('hidden_material_slots',[i for i in a.hidden_material_slots if i!=0])
 a.set_editor_property('attachments',[part for part in a.attachments if part.mesh.get_name()!='SM_ER_DrongoMask'])
 skins[1]=a;d.set_editor_property('omega_skins',skins)
 u.BlueprintEditorLibrary.compile_blueprint(bp);A.set_metadata_tag(bp,'ERScavengerOriginalHead','1');assert A.save_loaded_asset(bp,False)
for rel,h in protected.items():assert hashlib.sha256((p/rel).read_bytes()).hexdigest()==h,rel
(out/'setup.json').write_text(json.dumps({'skin':1,'name':'Drongo · Jackal','original_scavenger_head':True,'protected_packages':len(protected)},indent=2))

for name in ['M_ER_Omega_ScavengerSkull_Grounded','M_ER_Omega_ScavengerSkullMask_Grounded','M_ER_Omega_ScavengerOriginalMask_Grounded','M_ER_Scavenger_OriginalSurface']:
 retired=folder+'/'+name
 if A.does_asset_exist(retired) and not A.find_package_referencers_for_asset(retired,load_assets_to_confirm=True):assert A.delete_asset(retired)
