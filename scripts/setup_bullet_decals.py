"""Import the owner supplied ink atlas and bind four firearm decals through Unreal.

Run independently to update only decal assets and the weapon Blueprint defaults.
The source atlas is imported intact; quadrant selection preserves its alpha.
"""
from pathlib import Path
import unreal as ue

ROOT = '/Game/EclipseRecoil/Effects/Shots'
tools = ue.AssetToolsHelpers.get_asset_tools()
edit = ue.MaterialEditingLibrary

def load(path):
    asset = ue.load_asset(path)
    if not asset:
        raise RuntimeError('Missing asset: ' + path)
    return asset


def save(asset):
    if not ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Cannot save ' + asset.get_path_name())


def material(name):
    asset = ue.load_asset(ROOT + '/' + name) or tools.create_asset(name, ROOT, ue.Material, ue.MaterialFactoryNew())
    edit.delete_all_material_expressions(asset)
    return asset


def expression(mat, cls, **properties):
    node = edit.create_material_expression(mat, cls)
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node


def connect(node, output, other, pin):
    if not edit.connect_material_expressions(node, output, other, pin):
        raise RuntimeError('Cannot connect ' + pin)


def output(node, pin, prop):
    if not edit.connect_material_property(node, pin, prop):
        raise RuntimeError('Cannot connect material output ' + str(prop))


# Import the original alpha atlas intact. Four material instances select quadrants.
source = Path(ue.Paths.project_dir()) / 'SourceAssets/BulletImpacts/FourInkImpacts.png'
if not source.is_file():
    raise RuntimeError("Missing impact atlas: " + str(source))
task = ue.AssetImportTask()
for key, value in {'filename': str(source), 'destination_path': ROOT,
                   'destination_name': 'T_ER_BulletImpacts', 'automated': True,
                   'save': True, 'replace_existing': True}.items():
    task.set_editor_property(key, value)
tools.import_asset_tasks([task])
texture = load(ROOT + '/T_ER_BulletImpacts')
texture.set_editor_property('srgb', True)
texture.set_editor_property('address_x', ue.TextureAddress.TA_CLAMP)
texture.set_editor_property('address_y', ue.TextureAddress.TA_CLAMP)
save(texture)

mat = material('M_ER_BulletImpactAtlas')
mat.set_editor_property('material_domain', ue.MaterialDomain.MD_DEFERRED_DECAL)
mat.set_editor_property('blend_mode', ue.BlendMode.BLEND_TRANSLUCENT)
uv = expression(mat, ue.MaterialExpressionTextureCoordinate)
offset = expression(mat, ue.MaterialExpressionVectorParameter,
                    parameter_name='AtlasOffset', default_value=ue.LinearColor(0, 0, 0, 0))
mask = expression(mat, ue.MaterialExpressionComponentMask, r=True, g=True)
connect(offset, '', mask, '')
scale = expression(mat, ue.MaterialExpressionMultiply, const_b=.5)
connect(uv, '', scale, 'A')
add = expression(mat, ue.MaterialExpressionAdd)
connect(scale, '', add, 'A')
connect(mask, '', add, 'B')
sample = expression(mat, ue.MaterialExpressionTextureSample, texture=texture)
connect(add, '', sample, 'UVs')
output(sample, 'RGB', ue.MaterialProperty.MP_BASE_COLOR)
output(sample, 'A', ue.MaterialProperty.MP_OPACITY)
rough = expression(mat, ue.MaterialExpressionConstant, r=.95)
output(rough, '', ue.MaterialProperty.MP_ROUGHNESS)
edit.recompile_material(mat)
save(mat)
variants = []
for index, (x, y) in enumerate([(0, 0), (.5, 0), (0, .5), (.5, .5)]):
    name = 'MI_ER_BulletImpact_' + str(index + 1)
    instance = ue.load_asset(ROOT + '/' + name) or tools.create_asset(name, ROOT, ue.MaterialInstanceConstant, ue.MaterialInstanceConstantFactoryNew())
    edit.set_material_instance_parent(instance, mat)
    edit.set_material_instance_vector_parameter_value(instance, 'AtlasOffset', ue.LinearColor(x, y, 0, 0))
    save(instance)
    variants.append(instance)

weapon = load('/Game/EclipseRecoil/Weapons/BP_ERWeapon_Rifle')
defaults = ue.get_default_object(weapon.generated_class())
defaults.set_editor_property('impact_decal_variants', variants)
save(weapon)
ue.log('ER_BULLET_DECALS_PASS: four original ink atlas quadrants assigned; alpha preserved')
