"""Repair placeholder parents on private MOUT instances from their baked maps.

Some imported Substance instances have no parent (or use WorldGridMaterial). Build
owned world-aligned PBR parents from the supplied color/normal/roughness/AO/
metallic textures. Never change vendor originals or the destruction catalog.
"""
import json
from pathlib import Path
import unreal as ue

assets = ue.EditorAssetLibrary
edit = ue.MaterialEditingLibrary
tools = ue.AssetToolsHelpers.get_asset_tools()
root = '/Game/EclipseRecoil/Art/ArabianEnvironment/Materials'
rows = [('M_StoneWall1_SBSAR', 'M_Stonewall', 'stonewall_INST'),
        ('M_StoneFloor_SBSAR', 'M_StoneFloor', 'StoneFloor_INST'),
        ('M_Wood_SBSAR', 'M_Wood', 'agedwood_INST'),
        ('M_MetalWithFlaking_SBSAR', 'M_Metal_01', 'metal_INST')]
receipt = []

def expr(material, cls, x, y):
    return edit.create_material_expression(material, cls, x, y)

def custom(material, code, inputs, output, x, y):
    node = expr(material, ue.MaterialExpressionCustom, x, y)
    node.set_editor_property('code', code)
    node.set_editor_property('output_type', output)
    fields = []
    for name in inputs:
        field = ue.CustomInput(); field.set_editor_property('input_name', name); fields.append(field)
    node.set_editor_property('inputs', fields)
    for name, source in inputs.items(): edit.connect_material_expressions(source, '', node, name)
    return node

for folder, name, prefix in rows:
    instance = assets.load_asset(root + '/SubstanceTilingMaterials/' + folder + '/' + name)
    if not instance: raise RuntimeError('Missing material ' + name)
    if assets.get_metadata_tag(instance, 'ERArabianPBRRevision') == '1':
        ue.log('ER_ARABIAN_PBR_PRESERVED ' + name); continue
    parent = instance.get_editor_property('parent')
    if parent and 'WorldGridMaterial' not in parent.get_path_name():
        raise RuntimeError('Unexpected non-placeholder parent; inspect before replacing ' + str(parent))
    base_name = 'M_ER_Arabian_' + name.removeprefix('M_')
    base_path = root + '/Runtime/' + base_name
    material = assets.load_asset(base_path) if assets.does_asset_exist(base_path) else None
    if not material:
        material = tools.create_asset(base_name, root + '/Runtime', ue.Material, ue.MaterialFactoryNew())
        material.set_editor_property('used_with_instanced_static_meshes', True)
        material.set_editor_property('tangent_space_normal', False)
        position = expr(material, ue.MaterialExpressionWorldPosition, -900, 0)
        normal = expr(material, ue.MaterialExpressionVertexNormalWS, -900, 150)
        uv = custom(material, 'float3 a=abs(N); if(a.x>a.y && a.x>a.z) return float2(P.y*sign(N.x),-P.z)/200; if(a.y>a.z) return float2(-P.x*sign(N.y),-P.z)/200; return float2(P.x*sign(N.z),-P.y)/200;',
                    {'P': position, 'N': normal}, ue.CustomMaterialOutputType.CMOT_FLOAT2, -650, 0)
        for i, (role, prop) in enumerate([('basecolor', ue.MaterialProperty.MP_BASE_COLOR),
                                         ('normal', ue.MaterialProperty.MP_NORMAL),
                                         ('roughness', ue.MaterialProperty.MP_ROUGHNESS),
                                         ('ambientocclusion', ue.MaterialProperty.MP_AMBIENT_OCCLUSION),
                                         ('metallic', ue.MaterialProperty.MP_METALLIC)]):
            names = [prefix + '_' + role]
            if role == 'ambientocclusion': names += [prefix + '_ambientocclusion', prefix + '_ambient_occlusion', prefix + '_Ambient_Occlusion']
            texture = next((assets.load_asset(root + '/SubstanceTilingMaterials/' + folder + '/' + n)
                            for n in names if assets.does_asset_exist(root + '/SubstanceTilingMaterials/' + folder + '/' + n)), None)
            if not texture: raise RuntimeError('Missing baked map ' + folder + '/' + role)
            texture.set_editor_property('srgb', role == 'basecolor')
            texture.set_editor_property('compression_settings', ue.TextureCompressionSettings.TC_NORMALMAP if role == 'normal' else ue.TextureCompressionSettings.TC_DEFAULT if role == 'basecolor' else ue.TextureCompressionSettings.TC_MASKS)
            if not assets.save_loaded_asset(texture): raise RuntimeError('Cannot save texture settings')
            sample = expr(material, ue.MaterialExpressionTextureSample, -400, i * 170)
            sample.set_editor_property('texture', texture)
            sample.set_editor_property('sampler_type', ue.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'normal' else ue.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'basecolor' else ue.MaterialSamplerType.SAMPLERTYPE_MASKS)
            edit.connect_material_expressions(uv, '', sample, 'Coordinates')
            if role == 'normal':
                output = custom(material, 'float3 a=abs(N); if(a.x>a.y && a.x>a.z) return normalize(float3(T.z*sign(N.x),T.x*sign(N.x),-T.y)); if(a.y>a.z) return normalize(float3(-T.x*sign(N.y),T.z*sign(N.y),-T.y)); return normalize(float3(T.x*sign(N.z),-T.y,T.z*sign(N.z)));',
                                {'N': normal, 'T': sample}, ue.CustomMaterialOutputType.CMOT_FLOAT3, 0, i * 170)
                edit.connect_material_property(output, '', prop)
            else: edit.connect_material_property(sample, 'RGB' if role == 'basecolor' else 'R', prop)
        edit.recompile_material(material)
        if not assets.save_loaded_asset(material): raise RuntimeError('Cannot save PBR parent')
    edit.set_material_instance_parent(instance, material)
    assets.set_metadata_tag(instance, 'ERArabianPBRRevision', '1')
    if not assets.save_loaded_asset(instance): raise RuntimeError('Cannot save private instance')
    receipt.append({'instance': instance.get_path_name(), 'previous_parent': parent.get_path_name() if parent else None, 'parent': material.get_path_name()})
target = Path(ue.Paths.project_dir()) / 'Assets/maps/SourceAssets/ArabianEnvironment/pbr_repair.json'
if receipt: target.write_text(json.dumps(receipt, indent=2) + '\n')
ue.log('ER_ARABIAN_PBR_COMPLETE changed=' + str(len(receipt)))
