"""Replace HE floor fragments and fluid smoke with compact authored effects.

Run after migrate_epic_grenade.py and compiling the native adapters. Saves only
owned defaults/materials, keeping Epic originals and source archives intact.
"""
import json
from pathlib import Path
import unreal as ue

assets=ue.EditorAssetLibrary; tools=ue.AssetToolsHelpers.get_asset_tools(); edit=ue.MaterialEditingLibrary
ROOT='/Game/EclipseRecoil/Effects'
def load(path):
    obj=ue.load_asset(path)
    if not obj:raise RuntimeError('Missing '+path)
    return obj
def save(obj):
    if not assets.save_loaded_asset(obj,False):raise RuntimeError('Cannot save '+obj.get_path_name())
def node(mat,cls,**props):
    result=edit.create_material_expression(mat,cls)
    for key,value in props.items():result.set_editor_property(key,value)
    return result
def wire(src,out,dst,pin):
    if not edit.connect_material_expressions(src,out,dst,pin):raise RuntimeError('Cannot connect '+pin)
def scalar(mat,name,value):return node(mat,ue.MaterialExpressionScalarParameter,parameter_name=name,default_value=value)
def vector(mat,name,value):return node(mat,ue.MaterialExpressionVectorParameter,parameter_name=name,default_value=ue.LinearColor(*value))

source=ROOT+'/Grenade/Epic/Blueprint_Communication/Effects/P_Explosion_Bomb'
flash=ue.load_asset(ROOT+'/Grenade/M_ER_BriefFlash') or tools.create_asset('M_ER_BriefFlash',ROOT+'/Grenade',ue.Material,ue.MaterialFactoryNew())
edit.delete_all_material_expressions(flash)
flash.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT)
color=vector(flash,'FlashColor',(4,3.8,3.4,1))
edit.connect_material_property(color,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
edit.recompile_material(flash);save(flash)
burst=ue.load_asset(ROOT+'/Grenade/P_ER_CompactHE') or assets.duplicate_asset(source,ROOT+'/Grenade/P_ER_CompactHE')
if not ue.ERGrenadeExplosion.configure_compact_burst(burst):raise RuntimeError('Cannot configure HE burst')
save(burst)
he=load(ROOT+'/Grenade/BP_ERGrenadeExplosion');d=ue.get_default_object(he.generated_class())
for key,value in dict(burst_template=burst,burst_scale=.55,debris_mesh=None,debris_animation=None,flash_mesh=None,flash_material=ue.load_asset(ROOT+'/Grenade/M_ER_BriefFlash'),ground_fragment_mesh=load('/Engine/BasicShapes/Cube')).items():d.set_editor_property(key,value)
d.get_editor_property('debris').set_skeletal_mesh_asset(None)
d.get_editor_property('flash').set_static_mesh(None)
d.get_editor_property('flash').set_material(0,flash)
d.get_editor_property('burst').set_template(burst)
ue.BlueprintEditorLibrary.compile_blueprint(he);save(he)

mat=ue.load_asset(ROOT+'/Smoke/M_ER_ContainedSmoke') or tools.create_asset('M_ER_ContainedSmoke',ROOT+'/Smoke',ue.Material,ue.MaterialFactoryNew())
edit.delete_all_material_expressions(mat)
for key,value in dict(blend_mode=ue.BlendMode.BLEND_TRANSLUCENT,shading_model=ue.MaterialShadingModel.MSM_UNLIT,two_sided=True,disable_depth_test=True).items():mat.set_editor_property(key,value)
texture=load(ROOT+'/Grenade/Epic/Effects/Textures/T_Smoke_Tiled_D')
noise=node(mat,ue.MaterialExpressionTextureObjectParameter,parameter_name='SmokeNoise',texture=texture)
atlas=node(mat,ue.MaterialExpressionTextureObjectParameter,parameter_name='DensityAtlas',texture=load('/Engine/EngineResources/WhiteSquareTexture'))
inputs={
 'Atlas':atlas,'Noise':noise,
 'WPos':node(mat,ue.MaterialExpressionWorldPosition),
 'Cam':node(mat,ue.MaterialExpressionCameraPositionWS),
 'Depth':node(mat,ue.MaterialExpressionSceneDepth),
 'PixelDepth':node(mat,ue.MaterialExpressionPixelDepth),
 'Face':node(mat,ue.MaterialExpressionTwoSidedSign),
 'BMin':vector(mat,'VolumeMin',(-440,-440,0,0)),
 'BSize':vector(mat,'VolumeSize',(880,880,400,0)),
 'Phase':vector(mat,'ShapePhase',(0,0,0,0)),
 'Tint':vector(mat,'SmokeColor',(.30,.31,.32,1)),
 'Growth':scalar(mat,'Growth',0),'Fade':scalar(mat,'Fade',0),'Age':scalar(mat,'SmokeAge',0),
}
for i in range(4):
    inputs['CS'+str(i)]=vector(mat,'CutStart'+str(i),(0,0,0,0))
    inputs['CE'+str(i)]=vector(mat,'CutEnd'+str(i),(0,0,0,0))
    inputs['CR'+str(i)]=scalar(mat,'CutRadius'+str(i),0)
code=r'''
float3 mn=BMin.rgb, sz=BSize.rgb, mx=mn+sz;
float3 ro=Cam, rd=normalize(WPos-Cam);
bool inside=all(ro>=mn)&&all(ro<=mx);
if((!inside && Face<0)||(inside && Face>0)) return float4(0,0,0,0);
float3 inv=1.0/(rd+float3(0.000001,0.000001,0.000001));
float3 a=(mn-ro)*inv,b=(mx-ro)*inv;
float3 lo=min(a,b),hi=max(a,b);
float begin=max(0,max(lo.x,max(lo.y,lo.z)));
float end=min(hi.x,min(hi.y,hi.z));
end=min(end,Depth*length(WPos-Cam)/max(PixelDepth,.01));
if(end<=begin || Growth<=0 || Fade<=0) return float4(0,0,0,0);
float step=(end-begin)/48;
float trans=1;float3 result=0;
float3 origin=mn+float3(sz.xy*.5,0);
float3 starts[4]={CS0.rgb,CS1.rgb,CS2.rgb,CS3.rgb};
float3 ends[4]={CE0.rgb,CE1.rgb,CE2.rgb,CE3.rgb};
float radii[4]={CR0,CR1,CR2,CR3};
[loop] for(int i=0;i<48;i++)
{
 float3 p=ro+rd*(begin+(i+.5)*step);
 float3 uvw=(p-mn)/sz;
 float3 g=uvw*float3(16,16,8)-.5;
 float z=clamp(g.z,0,7),z0=floor(z),z1=min(7,z0+1);
 float2 xy=clamp(g.xy,float2(0,0),float2(15,15))+.5;
 float2 uv0=float2((xy.x+z0*16)/128,xy.y/16);
 float2 uv1=float2((xy.x+z1*16)/128,xy.y/16);
 float occupancy=lerp(Texture2DSampleLevel(Atlas,AtlasSampler,uv0,0).r,Texture2DSampleLevel(Atlas,AtlasSampler,uv1,0).r,frac(z));
 float3 q=(p-origin)/float3(sz.xy*.5,sz.z);
 float n=Texture2DSampleLevel(Noise,NoiseSampler,(p.xy+Phase.xy*37)*.004+float2(Age*.012,Age*.006),0).r;
 float n2=Texture2DSampleLevel(Noise,NoiseSampler,(p.yz+Phase.yz*37)*.008-float2(Age*.008,Age*.015),0).r;
 // Smooth asymmetric lobes, unique replicated phase per cloud. Stay within the 4.4m horizontal / 4m vertical bounds.
 float3 dir=q/max(length(q),.001);
 float lobes=sin(dir.x*4.1+Phase.x+Age*.28)*sin(dir.y*3.7+Phase.y-Age*.22)*cos(dir.z*4.6+Phase.z+Age*.18);
 float sway=sin(dir.x*3.1+Phase.x+Age*.85)*cos(dir.y*2.7+Phase.y-Age*.65);
 float boundary=min(1.0,.88+.07*lobes+.04*sway+.045*(n+n2-1));
 // Directional delays make separate billows lead/lag, catching up at full growth.
 // The phase is fixed per cloud so growth stays smooth rather than flickering.
 float patch=.5+.5*sin(dir.x*3.1+Phase.x)*cos(dir.y*4.3+Phase.y)*sin(dir.z*2.7+Phase.z);
 float delay=.22*patch;
 float localGrowth=saturate((Growth-delay)/(1-delay));
 float edge=saturate((localGrowth*boundary-length(q))/.20);
 float density=occupancy*edge*(.45+.55*n)*(.65+.35*n2)*Fade;
 [unroll] for(int c=0;c<4;c++)
 {
  float3 v=ends[c]-starts[c];float h=saturate(dot(p-starts[c],v)/max(dot(v,v),.01));
  float dist=length(p-starts[c]-h*v);
  float core=radii[c]>=100 ? .8 : .3;
  density*=lerp(1,smoothstep(radii[c]*core,max(radii[c],.001),dist),step(.001,radii[c]));
 }
 float alpha=1-exp(-density*step*.035);
 float shade=.62+.38*uvw.z+.10*n;
 result+=trans*alpha*Tint.rgb*shade;
 trans*=1-alpha;
 if(trans<.008) break;
}
float alpha=1-trans;
return float4(result/max(alpha,.001),alpha);
'''
# Avoid a name collision with HLSL's step() function.
code=code.replace('float step=','float marchStep=').replace('*step)', '*marchStep)').replace('*step*','*marchStep*')
custom=node(mat,ue.MaterialExpressionCustom,code=code,output_type=ue.CustomMaterialOutputType.CMOT_FLOAT4)
args=[]
for name in inputs:
    arg=ue.CustomInput();arg.set_editor_property('input_name',name);args.append(arg)
custom.set_editor_property('inputs',args)
for name,src in inputs.items():wire(src,'',custom,name)
rgb=node(mat,ue.MaterialExpressionComponentMask,r=True,g=True,b=True,a=False);wire(custom,'',rgb,'')
alpha=node(mat,ue.MaterialExpressionComponentMask,r=False,g=False,b=False,a=True);wire(custom,'',alpha,'')
if not edit.connect_material_property(rgb,'',ue.MaterialProperty.MP_EMISSIVE_COLOR):raise RuntimeError('Color output')
if not edit.connect_material_property(alpha,'',ue.MaterialProperty.MP_OPACITY):raise RuntimeError('Opacity output')
edit.recompile_material(mat);save(mat)
smoke=load(ROOT+'/Smoke/BP_ERSmokeCloud');d=ue.get_default_object(smoke.generated_class())
for key,value in dict(volume_material=mat,volume_mesh=load('/Engine/BasicShapes/Cube'),system=None,sim_size=ue.Vector(880,880,400),duration=12,smoke_color=ue.LinearColor(.30,.31,.32,1)).items():d.set_editor_property(key,value)
d.get_editor_property('fumes').set_asset(None)
d.get_editor_property('volume').set_static_mesh(load('/Engine/BasicShapes/Cube'))
d.get_editor_property('volume').set_material(0,mat)
ue.BlueprintEditorLibrary.compile_blueprint(smoke);save(smoke)
projectile=load('/Game/EclipseRecoil/Weapons/BP_ERProjectile');d=ue.get_default_object(projectile.generated_class())
d.set_editor_property('grenade_explosion_class',he.generated_class());d.set_editor_property('smoke_cloud_class',smoke.generated_class())
ue.BlueprintEditorLibrary.compile_blueprint(projectile);save(projectile)
report={'he':burst.get_path_name(),'he_scale':.55,'he_duration':3,'smoke_shape':'hemisphere','smoke_size_cm':[880,880,400],'containment_grid':[16,16,8],'raymarch_steps':48,'smoke_duration':12,'max_clouds':4,'max_bursts':8,'wall_test':'whole-cell overlap and neighbour sweep','dynamic_blocks':'revision refresh every .75s','global_fog_changed':False,'launcher_impact_detonation':False,'smoke_manual_opaque_depth_clip':True,'smoke_camera_inside':True,'he_fire_enabled':False,'he_flash_seconds':.065,'he_smoke_particles':3,'he_ground_chips':[3,6],'he_ground_trace_cm':60}
out=Path(ue.Paths.project_dir())/'Assets/weapons/SourceAssets/EpicContentExamples';out.mkdir(parents=True,exist_ok=True)
(out/'integration.json').write_text(json.dumps(report,indent=2)+'\n')
ue.log('ER_GRENADE_EFFECTS_READY '+json.dumps(report))
