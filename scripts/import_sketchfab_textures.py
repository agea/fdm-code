"""Import the owner's Sketchfab PBR selection through Unreal, with checked wiring.

Only project-owned new materials/textures and the industrial recipe are changed.
Shared combat profiles, original Factory art and Depot remain untouched.
"""
from pathlib import Path
import hashlib
import json
import re
import runpy
import unreal as ue

project=Path(ue.Paths.project_dir()).resolve()
source=project/'Assets/maps/SourceAssets/SketchfabTextures'
manifest=json.loads((source/'materials-manifest.json').read_text())
selection=runpy.run_path(str(project/'scripts/sketchfab_texture_selection.py'))
if {entry['id'] for entry in manifest} != selection['MATERIAL_IDS']:
    raise RuntimeError('Reprepare the current approved texture selection before importing')
if not manifest or any(entry.get('preparation_revision')!=2 for entry in manifest):
    raise RuntimeError('Finish prepare_sketchfab_textures.py revision 2 before importing')
assets=ue.EditorAssetLibrary;edit=ue.MaterialEditingLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
root='/Game/EclipseRecoil/Art/Industrial/SketchfabTextures'
protected=[project/'Assets/maps/Content/EclipseRecoil/Maps/L_Depot.umap',project/'Assets/maps/Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset']
before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}

def save(obj):
    if not assets.save_loaded_asset(obj,False):raise RuntimeError('Cannot save '+obj.get_path_name())

def wire(a,pin,b,input_name):
    if not edit.connect_material_expressions(a,pin,b,input_name):raise RuntimeError('Cannot connect '+a.get_name()+' to '+input_name)

def output(node,pin,prop):
    if not edit.connect_material_property(node,pin,prop):raise RuntimeError('Cannot connect output '+str(prop))

def expression(material,cls,x,y):return edit.create_material_expression(material,cls,x,y)

def custom(material,code,inputs,kind,x,y):
    node=expression(material,ue.MaterialExpressionCustom,x,y)
    node.set_editor_property('code',code);node.set_editor_property('output_type',kind)
    fields=[]
    for name in inputs:
        field=ue.CustomInput();field.set_editor_property('input_name',name);fields.append(field)
    node.set_editor_property('inputs',fields)
    for name,value in inputs.items():wire(value,'',node,name)
    return node

created={}
for entry in manifest:
    name='M_ER_SF_'+entry['id'];path=root+'/'+name
    material=assets.load_asset(path) if assets.does_asset_exist(path) else None
    ground=entry['id'] in {'asphalt_11','grass_02','snow_01'}
    if material and assets.get_metadata_tag(material,'ERSketchfabTextureRevision')=='2' and (not ground or assets.get_metadata_tag(material,'ERGroundBlendRevision')=='1'):
        created[entry['id']]=material;continue
    textures={}
    for role,relative in entry['maps'].items():
        original=source/relative
        if source not in original.resolve().parents:raise RuntimeError('Unsafe input')
        texture_name='T_ER_SF_'+entry['id']+'_'+role;texture_path=root+'/'+entry['id']+'/'+texture_name
        texture=assets.load_asset(texture_path) if assets.does_asset_exist(texture_path) else None
        if not texture or assets.get_metadata_tag(texture,'ERSketchfabPreparationRevision')!='2':
            task=ue.AssetImportTask();task.filename=str(original);task.destination_path=root+'/'+entry['id'];task.destination_name=texture_name
            task.automated=True;task.save=False;task.replace_existing=True;task.factory=ue.TextureFactory()
            tools.import_asset_tasks([task]);texture=assets.load_asset(texture_path)
            if not texture:raise RuntimeError('Texture import failed '+texture_name)
            texture.set_editor_property('srgb',role=='Color')
            texture.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP if role=='Normal' else ue.TextureCompressionSettings.TC_DEFAULT if role=='Color' else ue.TextureCompressionSettings.TC_MASKS)
            if role=='Normal':texture.set_editor_property('flip_green_channel',entry['flip_normal_green'])
            for key,value in [('ERSourceURL',entry['source']),('ERAuthor',entry['author']),('ERLicenseURL',entry['license']['url'])]:assets.set_metadata_tag(texture,key,value)
            assets.set_metadata_tag(texture,'ERSketchfabPreparationRevision','2')
            save(texture)
        textures[role]=texture
    if material:edit.delete_all_material_expressions(material)
    else:material=tools.create_asset(name,root,ue.Material,ue.MaterialFactoryNew())
    material.set_editor_property('used_with_instanced_static_meshes',True);material.set_editor_property('tangent_space_normal',False)
    position=expression(material,ue.MaterialExpressionWorldPosition,-1100,0)
    normal=expression(material,ue.MaterialExpressionVertexNormalWS,-1100,150)
    size=expression(material,ue.MaterialExpressionScalarParameter,-1100,300);size.set_editor_property('parameter_name','TileSizeCm');size.set_editor_property('default_value',entry['tile_cm'])
    # Every vertical face keeps V on world Z, so brick courses cannot turn upright.
    uv=custom(material,'float3 a=abs(N); if(a.x>a.y && a.x>a.z) return float2(P.y*sign(N.x),-P.z)/Size; if(a.y>a.z) return float2(-P.x*sign(N.y),-P.z)/Size; return float2(P.x*sign(N.z),-P.y)/Size;',{'P':position,'N':normal,'Size':size},ue.CustomMaterialOutputType.CMOT_FLOAT2,-850,0)
    if ground:
        second_uv=custom(material,'return float2(-UV.y,UV.x)*0.83+float2(0.371,0.619);',{'UV':uv},ue.CustomMaterialOutputType.CMOT_FLOAT2,-850,80)
        weight=custom(material,'return 0.5+0.5*sin(P.x/713.0+sin(P.y/997.0))*sin(P.y/881.0+cos(P.x/1297.0));',{'P':position},ue.CustomMaterialOutputType.CMOT_FLOAT1,-850,160)
    strength=expression(material,ue.MaterialExpressionScalarParameter,-850,300);strength.set_editor_property('parameter_name','NormalStrength');strength.set_editor_property('default_value',entry['normal_strength'])
    for index,(role,texture) in enumerate(textures.items()):
        sample=expression(material,ue.MaterialExpressionTextureSample,-600,index*200)
        sample.set_editor_property('texture',texture);sample.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if role=='Normal' else ue.MaterialSamplerType.SAMPLERTYPE_COLOR if role=='Color' else ue.MaterialSamplerType.SAMPLERTYPE_MASKS)
        wire(uv,'',sample,'UVs')
        pin='RGB' if role=='Color' else 'R'
        if ground:
            second=expression(material,ue.MaterialExpressionTextureSample,-600,index*200+80)
            second.set_editor_property('texture',texture);second.set_editor_property('sampler_type',sample.get_editor_property('sampler_type'))
            wire(second_uv,'',second,'UVs')
            code='return normalize(lerp(A.xyz,float3(B.y,-B.x,B.z),W));' if role=='Normal' else 'return lerp(A.rgb,B.rgb,W);' if role=='Color' else 'return lerp(A.r,B.r,W);'
            sample=custom(material,code,{'A':sample,'B':second,'W':weight},ue.CustomMaterialOutputType.CMOT_FLOAT3 if role in {'Color','Normal'} else ue.CustomMaterialOutputType.CMOT_FLOAT1,-250,index*200)
            pin=''
            if role=='Color' and entry['id']=='grass_02':
                sample=custom(material,'return C*float3(0.5,0.62,0.5);',{'C':sample},ue.CustomMaterialOutputType.CMOT_FLOAT3,-50,index*200)
        if role=='Normal':
            transformed=custom(material,'float3 t=normalize(float3(T.xy*Strength,T.z));float3 a=abs(N); if(a.x>a.y && a.x>a.z) return normalize(float3(t.z*sign(N.x),t.x*sign(N.x),-t.y)); if(a.y>a.z) return normalize(float3(-t.x*sign(N.y),t.z*sign(N.y),-t.y)); return normalize(float3(t.x*sign(N.z),-t.y,t.z*sign(N.z)));',{'N':normal,'T':sample,'Strength':strength},ue.CustomMaterialOutputType.CMOT_FLOAT3,0,index*200)
            output(transformed,'',ue.MaterialProperty.MP_NORMAL)
        else:
            prop={'Color':ue.MaterialProperty.MP_BASE_COLOR,'Roughness':ue.MaterialProperty.MP_ROUGHNESS,'AO':ue.MaterialProperty.MP_AMBIENT_OCCLUSION,'Metallic':ue.MaterialProperty.MP_METALLIC}[role]
            output(sample,pin,prop)
    edit.recompile_material(material)
    for key,value in [('ERSourceURL',entry['source']),('ERAuthor',entry['author']),('ERLicenseURL',entry['license']['url']),('ERProjection','World dominant face; checked UVs; cm'),('ERSketchfabTextureRevision','2')]:assets.set_metadata_tag(material,key,value)
    if ground:assets.set_metadata_tag(material,'ERGroundBlendRevision','1')
    save(material);created[entry['id']]=material

template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
if not template:raise RuntimeError('Missing industrial recipe')
revision='2'
if assets.get_metadata_tag(template,'ERSketchfabTextureRevision')!=revision:
    recipe=template.get_editor_property('central_building')
    # Missing downloads are never fabricated. Each installed group is recorded.
    mapping={'concrete_surfaces':['concrete_seamless','concrete_2k'],
             'brick_surfaces':['brick_tiled','brick_textures'],'wood_surfaces':['wood_05'],
             'ground_surfaces':['asphalt_11','grass_02','snow_01']}
    assigned={}
    for field,prefixes in mapping.items():
        palette=[value for prefix in prefixes for key,value in created.items() if key==prefix or key.startswith(prefix+'_')]
        if palette:recipe.set_editor_property(field,palette);assigned[field]=[m.get_path_name() for m in palette]
    if 'steel_irregular' in created:recipe.set_editor_property('steel_surface',created['steel_irregular'])
    if 'iron_plates' in created:recipe.set_editor_property('deck_surface',created['iron_plates'])
    template.set_editor_property('central_building',recipe)
    for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
        if not template.validate_layout(seed):raise RuntimeError('Invalid palette seed '+str(seed))
    template.set_editor_property('revision',max(7,template.get_editor_property('revision')))
    # A partial acquisition stays resumable; only a complete selected palette locks the migration.
    required=[prefix for prefixes in mapping.values() for prefix in prefixes]+['steel_irregular','iron_plates']
    complete=all(any(key==prefix or key.startswith(prefix+'_') for key in created) for prefix in required)
    if complete:assets.set_metadata_tag(template,'ERSketchfabTextureRevision',revision)
    save(template)
    if complete:runpy.run_path(str(project/'scripts/setup_industrial_pcg_preview.py'))
else:
    recipe=template.get_editor_property('central_building')
    assigned={field:[material.get_path_name() for material in recipe.get_editor_property(field)] for field in ['concrete_surfaces','brick_surfaces','wood_surfaces','ground_surfaces']}
for path,digest in before.items():
    if hashlib.sha256(Path(path).read_bytes()).hexdigest()!=digest:raise RuntimeError('Protected asset changed '+path)
receipt=dict(materials={key:value.get_path_name() for key,value in created.items()},assigned=assigned,preserved_sha256=before,validated_seeds=516,ground_blend_revision=1)
(source/'import-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
credits_path=project/'Branding/Credits.txt'
credits=credits_path.read_text()
heading='TEXTURES — SKETCHFAB COLLECTION'
credits=re.sub(re.escape(heading)+r'\n.*?(?=\n\n|\Z)','',credits,flags=re.S).replace('\n\n\n','\n\n')
lines=[heading]
seen=set()
for entry in manifest:
    if entry['id'] not in created or entry['source_uid'] in seen:continue
    seen.add(entry['source_uid'])
    lines.extend([entry['name']+' — '+entry['author']+'.',entry['source'],entry['author_url'],entry['license']['url']])
lines.append('Creative Commons Attribution 4.0. Changes: texture-only Unreal PBR adaptation, world projection, ground blending/grass tint and runtime resolution up to 2K. Original maps and credits retained privately.')
credits=credits.replace('THANK YOU\n','\n'.join(lines)+'\n\nTHANK YOU\n')
credits_path.write_text(credits)
ue.log('ER_SKETCHFAB_TEXTURES_IMPORTED '+json.dumps(receipt))
