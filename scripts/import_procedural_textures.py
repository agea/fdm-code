"""Import the verified CC0 2K source bundle via Unreal; preserve existing art."""
from pathlib import Path
import hashlib
import json
import unreal as ue
source = (Path(ue.Paths.project_dir()).resolve() / 'SourceAssets/ProceduralTextures').resolve()
manifest = json.loads((source / 'manifest.json').read_text())
if {entry['id'] for entry in manifest['assets']} != {'Plaster001','red_brick_03','concrete_wall_006','wood_planks','stone_wall_02','rusty_metal_04','asphalt_02'}:
    raise RuntimeError('Use the seven-set source manifest after the 20 October unused-texture cleanup')
assets = ue.EditorAssetLibrary
tools = ue.AssetToolsHelpers.get_asset_tools()
edit = ue.MaterialEditingLibrary
root = '/Game/EclipseRecoil/Art/ProceduralTextures'
def save(asset):
    if not assets.save_loaded_asset(asset, only_if_is_dirty=False): raise RuntimeError('Cannot save ' + asset.get_path_name())
def expr(m, cls, x=0, y=0): return edit.create_material_expression(m, cls, x, y)
def custom(m, code, inputs, output, x=0, y=0):
    node = expr(m, ue.MaterialExpressionCustom, x, y)
    node.set_editor_property('code', code); node.set_editor_property('output_type', output)
    args = []
    for name in inputs:
        arg = ue.CustomInput(); arg.set_editor_property('input_name', name); args.append(arg)
    node.set_editor_property('inputs', args)
    for name, value in inputs.items(): edit.connect_material_expressions(value, '', node, name)
    return node
for entry in manifest['assets']:
    for f in entry['files']:
        p = source / f['file']
        if source not in p.resolve().parents or not p.is_file() or p.stat().st_size != f['bytes'] or hashlib.sha256(p.read_bytes()).hexdigest() != f['sha256']:
            raise RuntimeError('Missing/unsafe/mismatched source: ' + f['file'])
materials = {}; imported = 0
for entry in manifest['assets']:
    maps = {}
    for f in entry['files']:
        name = Path(f['file']).name.lower()
        role = next((role for role, tokens in {'Color': ['_color.png','_diff_'], 'Normal': ['_normaldx.png','_nor_dx_'], 'Roughness': ['_roughness.png','_rough_'], 'AO': ['_ambientocclusion.png','_ao_'], 'Metallic': ['_metalness.png','_metal_']}.items() if any(t in name for t in tokens)), None)
        if not role or not name.endswith('.png') or '_rough_ao_' in name: continue
        dest = root + '/' + entry['id']; asset_name = 'T_ER_' + entry['id'] + '_' + role
        path = dest + '/' + asset_name
        texture = assets.load_asset(path) if assets.does_asset_exist(path) else None
        if not texture:
            task = ue.AssetImportTask()
            for key, value in {'filename': str(source/f['file']), 'destination_path': dest, 'destination_name': asset_name, 'automated': True, 'save': False, 'replace_existing': False, 'factory': ue.TextureFactory()}.items(): task.set_editor_property(key, value)
            tools.import_asset_tasks([task]); texture = assets.load_asset(path)
            if not isinstance(texture, ue.Texture2D): raise RuntimeError('Import failed ' + path)
            texture.set_editor_property('srgb', role == 'Color')
            texture.set_editor_property('compression_settings', ue.TextureCompressionSettings.TC_NORMALMAP if role == 'Normal' else ue.TextureCompressionSettings.TC_DEFAULT if role == 'Color' else ue.TextureCompressionSettings.TC_MASKS)
            assets.set_metadata_tag(texture, 'ERSourceURL', entry['source']); assets.set_metadata_tag(texture, 'ERSourceSHA256', f['sha256']); assets.set_metadata_tag(texture, 'ERLicense', 'CC0-1.0')
            save(texture); imported += 1
        maps[role] = texture
    if not all(k in maps for k in ['Color','Normal','Roughness']): raise RuntimeError('Incomplete set ' + entry['id'])
    name = 'M_ER_PBR_' + entry['id']; path = root + '/' + name
    material = assets.load_asset(path) if assets.does_asset_exist(path) else None
    if not material:
        material = tools.create_asset(name, root, ue.Material, ue.MaterialFactoryNew())
        material.set_editor_property('used_with_instanced_static_meshes', True); material.set_editor_property('tangent_space_normal', False)
        position = expr(material, ue.MaterialExpressionWorldPosition, -1000, 0)
        normal = expr(material, ue.MaterialExpressionVertexNormalWS, -1000, 150)
        tiling = expr(material, ue.MaterialExpressionScalarParameter, -1000, 300)
        tiling.set_editor_property('parameter_name', 'TileSizeCm'); tiling.set_editor_property('default_value', {'asphalt_02':400,'concrete_wall_006':300}.get(entry['id'],200))
        uv = custom(material, 'float3 a=abs(N); if(a.x>a.y && a.x>a.z) return float2(P.y*sign(N.x),-P.z)/Size; if(a.y>a.z) return float2(-P.x*sign(N.y),-P.z)/Size; return float2(P.x*sign(N.z),-P.y)/Size;', {'P':position,'N':normal,'Size':tiling}, ue.CustomMaterialOutputType.CMOT_FLOAT2, -700, 0)
        for index, (role, texture) in enumerate(maps.items()):
            sample = expr(material, ue.MaterialExpressionTextureSample, -400, index*170)
            sample.set_editor_property('texture', texture)
            sample.set_editor_property('sampler_type', ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'Normal' else ue.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'Color' else ue.MaterialSamplerType.SAMPLERTYPE_MASKS)
            edit.connect_material_expressions(uv, '', sample, 'Coordinates')
            if role == 'Normal':
                output = custom(material, 'float3 a=abs(N); if(a.x>a.y && a.x>a.z) return normalize(float3(T.z*sign(N.x),T.x*sign(N.x),-T.y)); if(a.y>a.z) return normalize(float3(-T.x*sign(N.y),T.z*sign(N.y),-T.y)); return normalize(float3(T.x*sign(N.z),-T.y,T.z*sign(N.z)));', {'N':normal,'T':sample}, ue.CustomMaterialOutputType.CMOT_FLOAT3, 0, index*170)
                edit.connect_material_property(output, '', ue.MaterialProperty.MP_NORMAL)
            else:
                prop = {'Color':ue.MaterialProperty.MP_BASE_COLOR,'Roughness':ue.MaterialProperty.MP_ROUGHNESS,'AO':ue.MaterialProperty.MP_AMBIENT_OCCLUSION,'Metallic':ue.MaterialProperty.MP_METALLIC}[role]
                edit.connect_material_property(sample, 'RGB' if role == 'Color' else 'R', prop)
        edit.recompile_material(material)
        assets.set_metadata_tag(material, 'ERProjection', 'World-aligned dominant face; cm; world-space DX normals')
        assets.set_metadata_tag(material, 'ERSourceURL', entry['source']); assets.set_metadata_tag(material, 'ERLicense', 'CC0-1.0'); save(material)
    materials[entry['id']] = material
path = root + '/M_ER_Glass'
if not assets.does_asset_exist(path):
    material = tools.create_asset('M_ER_Glass', root, ue.Material, ue.MaterialFactoryNew())
    material.set_editor_property('blend_mode', ue.BlendMode.BLEND_TRANSLUCENT); material.set_editor_property('two_sided', True)
    tint = expr(material, ue.MaterialExpressionVectorParameter, -250, 0)
    tint.set_editor_property('parameter_name', 'Tint'); tint.set_editor_property('default_value', ue.LinearColor(.75,.9,1,1))
    edit.connect_material_property(tint, '', ue.MaterialProperty.MP_BASE_COLOR)
    for name, value, prop, y in [('Opacity',.15,ue.MaterialProperty.MP_OPACITY,150),('Roughness',.08,ue.MaterialProperty.MP_ROUGHNESS,300)]:
        node = expr(material, ue.MaterialExpressionScalarParameter, -250, y)
        node.set_editor_property('parameter_name', name); node.set_editor_property('default_value', value)
        edit.connect_material_property(node, '', prop)
    edit.recompile_material(material); save(material)
catalog = assets.load_asset('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials')
if not catalog: raise RuntimeError('Run setup_destruction_lab.py first')
if assets.get_metadata_tag(catalog, 'ERPBRVersion') != '1':
    rows = list(catalog.get_editor_property('materials'))
    mapping = {ue.ERBlockMaterial.DRYWALL:'Plaster001',ue.ERBlockMaterial.BRICK:'red_brick_03',ue.ERBlockMaterial.CONCRETE:'concrete_wall_006'}
    for row in rows:
        kind=row.get_editor_property('type')
        if kind in mapping: row.set_editor_property('surface',materials[mapping[kind]])
    catalog.set_editor_property('materials', rows); assets.set_metadata_tag(catalog, 'ERPBRVersion', '1'); save(catalog)
# The old destruction/procedural laboratories and their unused floor/ceiling
# sets were retired. Never reopen/recreate them during a library import.
ue.log('ER_TEXTURE_LIBRARY_SAVED: sets=' + str(len(materials)) + ' imported=' + str(imported) + ' glass=1 checksums=' + str(sum(len(entry['files']) for entry in manifest['assets'])))
