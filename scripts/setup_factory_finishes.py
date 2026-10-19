"""Adapt migrated Factory atlases to procedural steel/paint without editing vendor assets or physics."""
from pathlib import Path
import hashlib,json,runpy
import unreal as ue
assets=ue.EditorAssetLibrary;edit=ue.MaterialEditingLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
root=Path(ue.Paths.project_dir());folder='/Game/EclipseRecoil/Art/Industrial/Materials'
protected=['Content/EclipseRecoil/Maps/L_Depot.umap','Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset']
private=root/'Assets/maps';before={p:hashlib.sha256((private/p).read_bytes()).hexdigest() for p in protected}
assets.make_directory(folder)
def expr(m,cls,x,y):return edit.create_material_expression(m,cls,x,y)
def custom(m,code,inputs,kind,x,y):
 n=expr(m,ue.MaterialExpressionCustom,x,y);n.set_editor_property('code',code);n.set_editor_property('output_type',kind)
 fields=[]
 for key in inputs:
  f=ue.CustomInput();f.set_editor_property('input_name',key);fields.append(f)
 n.set_editor_property('inputs',fields)
 for key,source in inputs.items():
  pin=''
  if isinstance(source,tuple):source,pin=source
  if not edit.connect_material_expressions(source,pin,n,key):raise RuntimeError('Cannot wire '+key)
 return n
def vector(m,name,value,x,y):
 n=expr(m,ue.MaterialExpressionVectorParameter,x,y);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',ue.LinearColor(*value));return n
def make_master(name,prefix,rect,paint):
 path=folder+'/'+name
 m=assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset(name,folder,ue.Material,ue.MaterialFactoryNew())
 edit.delete_all_material_expressions(m)
 m.set_editor_property('used_with_instanced_static_meshes',True);m.set_editor_property('tangent_space_normal',False)
 p=expr(m,ue.MaterialExpressionWorldPosition,-1200,0);n=expr(m,ue.MaterialExpressionVertexNormalWS,-1200,120)
 bounds=vector(m,'AtlasRect',rect,-1200,250)
 tile=vector(m,'TileSizeCm',(600,300,0,0) if paint else (150,150,0,0),-1200,390)
 uv=custom(m,'''float3 a=abs(N); float2 uv;
 if(a.x>a.y && a.x>a.z) uv=float2(P.y*sign(N.x),-P.z)/Tile.xy;
 else if(a.y>a.z) uv=float2(-P.x*sign(N.y),-P.z)/Tile.xy;
 else uv=float2(P.x*sign(N.z),-P.y)/Tile.xy;
 float2 q=frac(uv*0.5); return float4(Rect.xy+abs(q*2-1)*Rect.zw,step(0.5,q)*2-1);''',{'P':p,'N':n,'Rect':(bounds,'RGBA'),'Tile':tile},ue.CustomMaterialOutputType.CMOT_FLOAT4,-950,0)
 mask=expr(m,ue.MaterialExpressionComponentMask,-700,0)
 for key,value in [('r',True),('g',True),('b',False),('a',False)]:mask.set_editor_property(key,value)
 # The editor API shortens Coordinates to UVs and the mask Input to an empty pin.
 if not edit.connect_material_expressions(uv,'',mask,''):raise RuntimeError('Cannot wire atlas coordinate mask')
 samples={}
 for i,(key,suffix,kind) in enumerate([('BC','_BC',ue.MaterialSamplerType.SAMPLERTYPE_COLOR),('ORM','_ORM',ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR),('T','_N',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)]):
  tex=assets.load_asset('/Game/EclipseRecoil/Art/FactoryEnvironment/Textures/'+prefix+suffix)
  if not tex:raise RuntimeError('Missing Factory texture '+prefix+suffix)
  sample=expr(m,ue.MaterialExpressionTextureSample,-480,i*220);sample.set_editor_property('texture',tex);sample.set_editor_property('sampler_type',kind);
  if not edit.connect_material_expressions(mask,'',sample,'UVs'):raise RuntimeError('Cannot wire atlas UVs')
  samples[key]=sample
 tint=vector(m,'PaintColor' if paint else 'SteelTint',(0.42,0.46,0.50,1),-450,680)
 color=custom(m,'return Tint.rgb*(0.72+0.55*dot(BC.rgb,float3(0.2126,0.7152,0.0722)));' if paint else 'return BC.rgb*Tint.rgb;',{'BC':samples['BC'],'Tint':tint},ue.CustomMaterialOutputType.CMOT_FLOAT3,0,0)
 rough=custom(m,'return clamp(ORM.g,0.45,0.8);' if paint else 'return clamp(ORM.g,0.38,0.72);',{'ORM':samples['ORM']},ue.CustomMaterialOutputType.CMOT_FLOAT1,0,200)
 metallic=custom(m,'return ORM.b*0.18;' if paint else 'return max(0.85,ORM.b);',{'ORM':samples['ORM']},ue.CustomMaterialOutputType.CMOT_FLOAT1,0,330)
 normal=custom(m,'''float3 t=normalize(float3(T.xy*''' + ('0.35' if paint else '0.25') + '''*UV.zw,T.z));float3 a=abs(N);
 if(a.x>a.y && a.x>a.z) return normalize(float3(t.z*sign(N.x),t.x*sign(N.x),-t.y));
 if(a.y>a.z) return normalize(float3(-t.x*sign(N.y),t.z*sign(N.y),-t.y));
 return normalize(float3(t.x*sign(N.z),-t.y,t.z*sign(N.z)));''',{'T':samples['T'],'UV':uv,'N':n},ue.CustomMaterialOutputType.CMOT_FLOAT3,0,470)
 for node,prop in [(color,ue.MaterialProperty.MP_BASE_COLOR),(rough,ue.MaterialProperty.MP_ROUGHNESS),(metallic,ue.MaterialProperty.MP_METALLIC),(normal,ue.MaterialProperty.MP_NORMAL)]:
  if not edit.connect_material_property(node,'',prop):raise RuntimeError('Cannot connect material output')
 edit.connect_material_property(samples['ORM'],'R',ue.MaterialProperty.MP_AMBIENT_OCCLUSION)
 edit.recompile_material(m);assets.set_metadata_tag(m,'ERFactoryFinishRevision','1')
 if not assets.save_loaded_asset(m):raise RuntimeError('Cannot save '+name)
 return m
# Rectangles select plain gray / painted metal, excluding logos, bolts and rusty atlas islands.
steel=make_master('M_ER_FactoryGraySteel','T_Metalbeam01',(0.03,0.185,0.39,0.05),False)
paint=make_master('M_ER_FactoryContainerPaint','T_Container01',(0.03,0.31,0.65,0.27),True)
colors={'Blue':(.055,.16,.27,1),'Red':(.32,.055,.035,1),'Green':(.08,.19,.12,1),'Ochre':(.46,.25,.045,1)}
palette=[]
for name,color in colors.items():
 path=folder+'/MI_ER_FactoryContainer_'+name
 mi=assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset('MI_ER_FactoryContainer_'+name,folder,ue.MaterialInstanceConstant,ue.MaterialInstanceConstantFactoryNew())
 edit.set_material_instance_parent(mi,paint);edit.set_material_instance_vector_parameter_value(mi,'PaintColor',ue.LinearColor(*color));edit.update_material_instance(mi)
 if not assets.save_loaded_asset(mi):raise RuntimeError('Cannot save '+name)
 palette.append(mi)
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
recipe=template.get_editor_property('central_building');recipe.set_editor_property('steel_surface',steel);recipe.set_editor_property('deck_surface',steel);recipe.set_editor_property('container_surfaces',palette);template.set_editor_property('central_building',recipe)
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
 if not template.validate_layout(seed):raise RuntimeError('Invalid visual recipe '+str(seed))
template.set_editor_property('revision',5);assets.set_metadata_tag(template,'ERArchetypeRevision','5')
if not assets.save_loaded_asset(template):raise RuntimeError('Cannot save recipe')
runpy.run_path(str(root/'scripts/setup_industrial_pcg_preview.py'))
for p,sha in before.items():
 if hashlib.sha256((private/p).read_bytes()).hexdigest()!=sha:raise RuntimeError('Protected asset changed '+p)
report={'result':'PASS','revision':5,'steel':steel.get_path_name(),'containers':[m.get_path_name() for m in palette],'colors_linear':colors,'factory_textures':['T_Metalbeam01_BC','T_Metalbeam01_N','T_Metalbeam01_ORM','T_Container01_BC','T_Container01_N','T_Container01_ORM'],'container_color_symmetry':'independent per copy, deterministic by seed','steel_uv_rect':[0.03,0.185,0.39,0.05],'container_uv_rect':[0.03,0.31,0.65,0.27],'container_tile_cm':[600,300],'physics_catalog_unchanged':True,'preserved_sha256':before,'validated_seeds':516}
(root/'Saved/Testing/factory-finishes-assets-report.json').write_text(json.dumps(report,indent=2)+'\n')
(private/'SourceAssets/MapTemplates/factory_finishes.json').write_text(json.dumps(report,indent=2)+'\n')
print('ER_FACTORY_FINISHES_SETUP_PASS '+json.dumps(report))
