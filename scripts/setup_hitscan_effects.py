"""Build owned Niagara shot effects and four decal atlas variants via Unreal.

Preserves weapon statistics, existing flash/recoil, maps and vendor assets.
Source PNG lives in the private weapons submodule; no runtime Downloads access.
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


# Keep the isolated decal migration as the authoritative atlas builder.
import runpy
variants = runpy.run_path(str(Path(__file__).with_name('setup_bullet_decals.py')))['variants']

# A camera-facing Niagara sprite stretched along User.Alignment renders the
# whole authorized segment. SourceMode=Emitter renders the segment independently of physical bullets.
beam_mat = material('M_ER_Tracer')
beam_mat.set_editor_property('shading_model', ue.MaterialShadingModel.MSM_UNLIT)
beam_mat.set_editor_property('blend_mode', ue.BlendMode.BLEND_ADDITIVE)
beam_mat.set_editor_property('used_with_niagara_sprites', True)
color = expression(beam_mat, ue.MaterialExpressionConstant3Vector, constant=ue.LinearColor(4, 2.2, .65, 1))
output(color, '', ue.MaterialProperty.MP_EMISSIVE_COLOR)
uv = expression(beam_mat, ue.MaterialExpressionTextureCoordinate)
fade = expression(beam_mat, ue.MaterialExpressionCustom, output_type=ue.CustomMaterialOutputType.CMOT_FLOAT1,
                  code='return saturate(1-abs(UV.x*2-1));')
arg = ue.CustomInput()
arg.set_editor_property('input_name', 'UV')
fade.set_editor_property('inputs', [arg])
connect(uv, '', fade, 'UV')
alpha = expression(beam_mat, ue.MaterialExpressionMultiply, const_b=.7)
connect(fade, '', alpha, 'A')
output(alpha, '', ue.MaterialProperty.MP_OPACITY)
edit.recompile_material(beam_mat)
save(beam_mat)
beam = ue.load_asset(ROOT + '/NS_ER_Tracer') or tools.create_asset('NS_ER_Tracer', ROOT, ue.NiagaraSystem, ue.NiagaraSystemFactoryNew())
if not ue.ERShotEffects.configure_tracer(beam, beam_mat):
    raise RuntimeError('Cannot configure Niagara tracer')
save(beam)
# A faint, short, circular glint; no spark fan or directional flare.
impact = ue.load_asset(ROOT + '/NS_ER_Impact') or tools.create_asset('NS_ER_Impact', ROOT, ue.NiagaraSystem, ue.NiagaraSystemFactoryNew())
impact_mat = material('M_ER_ImpactSprite')
impact_mat.set_editor_property('shading_model', ue.MaterialShadingModel.MSM_UNLIT)
impact_mat.set_editor_property('blend_mode', ue.BlendMode.BLEND_ADDITIVE)
impact_mat.set_editor_property('used_with_niagara_sprites', True)
color = expression(impact_mat, ue.MaterialExpressionConstant3Vector, constant=ue.LinearColor(.5, .35, .18, 1))
output(color, '', ue.MaterialProperty.MP_EMISSIVE_COLOR)
uv = expression(impact_mat, ue.MaterialExpressionTextureCoordinate)
soft = expression(impact_mat, ue.MaterialExpressionCustom, output_type=ue.CustomMaterialOutputType.CMOT_FLOAT1,
                  code='float r=length(UV*2-1); return .12*pow(saturate(1-r),3);')
arg = ue.CustomInput()
arg.set_editor_property('input_name', 'UV')
soft.set_editor_property('inputs', [arg])
connect(uv, '', soft, 'UV')
output(soft, '', ue.MaterialProperty.MP_OPACITY)
edit.recompile_material(impact_mat)
save(impact_mat)
if not ue.ERShotEffects.configure_impact(impact, impact_mat):
    raise RuntimeError('Cannot configure subtle impact Niagara')
save(impact)

# Reuse the engine cube as a small, matte black spent casing.
casing_mat = material('M_ER_CasingDark')
dark = expression(casing_mat, ue.MaterialExpressionConstant3Vector, constant=ue.LinearColor(.008,.009,.01,1))
output(dark, '', ue.MaterialProperty.MP_BASE_COLOR)
rough = expression(casing_mat, ue.MaterialExpressionConstant, r=.95)
output(rough, '', ue.MaterialProperty.MP_ROUGHNESS)
edit.recompile_material(casing_mat)
save(casing_mat)
casing = ue.load_asset(ROOT + '/BP_ER_SpentCasing')
if not casing:
    factory = ue.BlueprintFactory()
    factory.set_editor_property('parent_class', ue.ERSpentCasing)
    casing = tools.create_asset('BP_ER_SpentCasing', ROOT, ue.Blueprint, factory)
casing_defaults = ue.get_default_object(casing.generated_class())
casing_defaults.get_editor_property('mesh').set_static_mesh(load('/Engine/BasicShapes/Cube'))
casing_defaults.get_editor_property('mesh').set_material(0, casing_mat)
ue.BlueprintEditorLibrary.compile_blueprint(casing)
save(casing)

# Softer muzzle flash, keeping the existing first-person rendering setup.
flash = material('M_ER_MuzzleFlashSoft')
flash.set_editor_property('shading_model', ue.MaterialShadingModel.MSM_UNLIT)
flash.set_editor_property('blend_mode', ue.BlendMode.BLEND_ADDITIVE)
color = expression(flash, ue.MaterialExpressionConstant3Vector, constant=ue.LinearColor(1.2,.6,.15,1))
output(color, '', ue.MaterialProperty.MP_EMISSIVE_COLOR)
fresnel = expression(flash, ue.MaterialExpressionFresnel, exponent=2.)
invert = expression(flash, ue.MaterialExpressionOneMinus)
connect(fresnel, '', invert, '')
opacity = expression(flash, ue.MaterialExpressionMultiply, const_b=.15)
connect(invert, '', opacity, 'A')
output(opacity, '', ue.MaterialProperty.MP_OPACITY)
edit.recompile_material(flash)
save(flash)

character = load('/Game/EclipseRecoil/Core/BP_ERCharacter')
character_defaults = ue.get_default_object(character.generated_class())
character_defaults.set_editor_property('tracer_system', beam)
character_defaults.set_editor_property('casing_class', casing.generated_class())
character_defaults.set_editor_property('muzzle_flash_material', flash)
save(character)
weapon = load('/Game/EclipseRecoil/Weapons/BP_ERWeapon_Rifle')
defaults = ue.get_default_object(weapon.generated_class())
defaults.set_editor_property('impact_decal_variants', variants)
defaults.set_editor_property('impact_system', impact)
save(weapon)
ue.log('ER_HITSCAN_EFFECTS_SAVED: Niagara tracer, impact burst, four alpha decal variants, subtle glints and dark casings')
