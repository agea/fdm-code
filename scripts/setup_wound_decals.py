"""Import the four owner-supplied PNGs intact and author only wound materials.
Run with UnrealEditor <project> -run=pythonscript -script=<this file> -nullrhi.
"""
from pathlib import Path
import unreal as ue

ROOT = '/Game/EclipseRecoil/Effects/Wounds'
source = Path(ue.Paths.project_dir()) / 'SourceAssets/BulletImpacts/Wounds'
tools = ue.AssetToolsHelpers.get_asset_tools()
edit = ue.MaterialEditingLibrary


def save(asset):
    if not ue.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError('Cannot save ' + asset.get_path_name())


textures = []
for i in range(1, 5):
    filename = source / f'wound{i}.png'
    if not filename.is_file():
        raise RuntimeError('Missing owner source: ' + str(filename))
    task = ue.AssetImportTask()
    for key, value in dict(filename=str(filename), destination_path=ROOT,
                           destination_name=f'T_ER_Wound_{i}', automated=True,
                           save=True, replace_existing=True).items():
        task.set_editor_property(key, value)
    tools.import_asset_tasks([task])
    texture = ue.load_asset(ROOT + f'/T_ER_Wound_{i}')
    if not texture:
        raise RuntimeError('Import failed: ' + str(filename))
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('address_x', ue.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y', ue.TextureAddress.TA_CLAMP)
    save(texture)
    textures.append(texture)

mat = ue.load_asset(ROOT + '/M_ER_Wound') or tools.create_asset('M_ER_Wound', ROOT, ue.Material, ue.MaterialFactoryNew())
edit.delete_all_material_expressions(mat)
mat.set_editor_property('material_domain', ue.MaterialDomain.MD_DEFERRED_DECAL)
mat.set_editor_property('blend_mode', ue.BlendMode.BLEND_TRANSLUCENT)
sample = edit.create_material_expression(mat, ue.MaterialExpressionTextureSampleParameter2D)
sample.set_editor_property('parameter_name', 'WoundTexture')
sample.set_editor_property('texture', textures[0])
for channel, prop in [('RGB', ue.MaterialProperty.MP_BASE_COLOR), ('A', ue.MaterialProperty.MP_OPACITY)]:
    if not edit.connect_material_property(sample, channel, prop):
        raise RuntimeError('Material connection failed')
roughness = edit.create_material_expression(mat, ue.MaterialExpressionConstant)
roughness.set_editor_property('r', .85)
edit.connect_material_property(roughness, '', ue.MaterialProperty.MP_ROUGHNESS)
edit.recompile_material(mat)
save(mat)
for i, texture in enumerate(textures, 1):
    name = f'MI_ER_Wound_{i}'
    instance = ue.load_asset(ROOT + '/' + name) or tools.create_asset(name, ROOT, ue.MaterialInstanceConstant, ue.MaterialInstanceConstantFactoryNew())
    edit.set_material_instance_parent(instance, mat)
    edit.set_material_instance_texture_parameter_value(instance, 'WoundTexture', texture)
    save(instance)
ue.log('ER_WOUND_ASSETS PASS variants=4 original_alpha=preserved')
