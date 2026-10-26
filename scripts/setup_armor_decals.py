"""Import the original owner fabric atlas intact; four instances select quadrants."""
from pathlib import Path
import unreal as ue

ROOT = '/Game/EclipseRecoil/Effects/Wounds'
source = Path(ue.Paths.project_dir()) / 'SourceAssets/BulletImpacts/Wounds/ArmorImpacts.png'
if not source.is_file():
    raise RuntimeError('Missing owner fabric atlas: ' + str(source))
tools = ue.AssetToolsHelpers.get_asset_tools()
edit = ue.MaterialEditingLibrary


def save(asset):
    if not ue.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError('Cannot save ' + asset.get_path_name())


task = ue.AssetImportTask()
for key, value in dict(filename=str(source), destination_path=ROOT,
                       destination_name='T_ER_ArmorImpacts', automated=True,
                       save=True, replace_existing=True).items():
    task.set_editor_property(key, value)
tools.import_asset_tasks([task])
texture = ue.load_asset(ROOT + '/T_ER_ArmorImpacts')
if not texture:
    raise RuntimeError('Armor atlas import failed')
texture.set_editor_property('srgb', True)
texture.set_editor_property('address_x', ue.TextureAddress.TA_CLAMP)
texture.set_editor_property('address_y', ue.TextureAddress.TA_CLAMP)
save(texture)
mat = ue.load_asset(ROOT + '/M_ER_ArmorImpactAtlas') or tools.create_asset('M_ER_ArmorImpactAtlas', ROOT, ue.Material, ue.MaterialFactoryNew())
edit.delete_all_material_expressions(mat)
mat.set_editor_property('material_domain', ue.MaterialDomain.MD_DEFERRED_DECAL)
mat.set_editor_property('blend_mode', ue.BlendMode.BLEND_TRANSLUCENT)


def node(cls, **properties):
    result = edit.create_material_expression(mat, cls)
    for key, value in properties.items():
        result.set_editor_property(key, value)
    return result


def connect(a, output, b, pin):
    if not edit.connect_material_expressions(a, output, b, pin):
        raise RuntimeError('Cannot connect ' + pin)


uv = node(ue.MaterialExpressionTextureCoordinate)
scale = node(ue.MaterialExpressionMultiply, const_b=.5)
connect(uv, '', scale, 'A')
offset = node(ue.MaterialExpressionVectorParameter, parameter_name='AtlasOffset', default_value=ue.LinearColor(0, 0, 0, 0))
mask = node(ue.MaterialExpressionComponentMask, r=True, g=True)
connect(offset, '', mask, '')
add = node(ue.MaterialExpressionAdd)
connect(scale, '', add, 'A')
connect(mask, '', add, 'B')
sample = node(ue.MaterialExpressionTextureSample, texture=texture)
connect(add, '', sample, 'UVs')
for channel, prop in [('RGB', ue.MaterialProperty.MP_BASE_COLOR), ('A', ue.MaterialProperty.MP_OPACITY)]:
    if not edit.connect_material_property(sample, channel, prop):
        raise RuntimeError('Cannot connect material output')
roughness = node(ue.MaterialExpressionConstant, r=.95)
edit.connect_material_property(roughness, '', ue.MaterialProperty.MP_ROUGHNESS)
edit.recompile_material(mat)
save(mat)
for i, (x, y) in enumerate([(0, 0), (.5, 0), (0, .5), (.5, .5)], 1):
    name = f'MI_ER_ArmorImpact_{i}'
    instance = ue.load_asset(ROOT + '/' + name) or tools.create_asset(name, ROOT, ue.MaterialInstanceConstant, ue.MaterialInstanceConstantFactoryNew())
    edit.set_material_instance_parent(instance, mat)
    edit.set_material_instance_vector_parameter_value(instance, 'AtlasOffset', ue.LinearColor(x, y, 0, 0))
    save(instance)
ue.log('ER_ARMOR_DECALS PASS variants=4 original_atlas_alpha=preserved')
