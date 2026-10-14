"""Create and assign a procedural deferred decal for rifle impacts."""
import unreal as ue


ROOT = "/Game/EclipseRecoil/Weapons"
MATERIAL_PATH = ROOT + "/M_ER_BulletHole"
PROJECTILE_PATH = ROOT + "/BP_ERProjectile"


def load(path):
    asset = ue.load_asset(path)
    if not asset:
        raise RuntimeError("Missing asset: " + path)
    return asset


def save(asset):
    if not ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError("Cannot save " + asset.get_path_name())


material = ue.load_asset(MATERIAL_PATH)
if not material:
    material = ue.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_ER_BulletHole", ROOT, ue.Material, ue.MaterialFactoryNew())
if not material:
    raise RuntimeError("Cannot create bullet impact material")

material.set_editor_property("material_domain", ue.MaterialDomain.MD_DEFERRED_DECAL)
material.set_editor_property("blend_mode", ue.BlendMode.BLEND_TRANSLUCENT)
ue.MaterialEditingLibrary.delete_all_material_expressions(material)

texcoord = ue.MaterialEditingLibrary.create_material_expression(
    material, ue.MaterialExpressionTextureCoordinate, -600, 0)
texture_input = ue.CustomInput()
texture_input.set_editor_property("input_name", "UV")
opacity_output = ue.CustomOutput()
opacity_output.set_editor_property("output_name", "Opacity")
opacity_output.set_editor_property("output_type", ue.CustomMaterialOutputType.CMOT_FLOAT1)

# Analytic crater, broken stone rim and short radial cracks. The output stays
# monochrome so it reads on the depot's light walls without importing textures.
surface = ue.MaterialEditingLibrary.create_material_expression(
    material, ue.MaterialExpressionCustom, -300, 0)
surface.set_editor_property("description", "Procedural bullet crater and chips")
surface.set_editor_property("output_type", ue.CustomMaterialOutputType.CMOT_FLOAT3)
surface.set_editor_property("inputs", [texture_input])
surface.set_editor_property("additional_outputs", [opacity_output])
surface.set_editor_property("code", r"""
float2 p = UV * 2.0 - 1.0;
float r = length(p);
float a = atan2(p.y, p.x);
float edge = 0.43 + 0.035*sin(a*7.0 + 0.4) + 0.022*sin(a*13.0 - 1.1) + 0.012*sin(a*23.0);
float hole = 1.0 - smoothstep(edge - 0.025, edge + 0.055, r);
float center = 1.0 - smoothstep(0.105, 0.205, r);
float rim = exp(-pow((r - edge*0.70)/0.075, 2.0));
float grain = sin(p.x*83.0 + sin(p.y*47.0))*sin(p.y*71.0 + p.x*29.0);
float cracks = 0.0;
[unroll] for (int i = 0; i < 9; ++i)
{
    float ray = 6.2831853 * (float)i / 9.0 + 0.16*sin(float(i)*8.17);
    float da = abs(atan2(sin(a-ray), cos(a-ray)));
    float reach = 0.55 + 0.20*sin(float(i)*5.31 + 0.7);
    float span = step(edge*0.82, r) * (1.0 - smoothstep(reach-0.035, reach+0.035, r));
    float width = 0.006 + 0.003*(1.0-r);
    cracks = max(cracks, (1.0-smoothstep(width, width+0.008, da*r))*span);
}
float chippedRim = rim * (0.38 + 0.22*sin(a*11.0 + grain*2.0));
float value = lerp(0.30 + 0.10*grain, 0.012 + 0.012*grain, center);
value = lerp(value, 0.48 + chippedRim + 0.06*grain, saturate(rim));
value = lerp(value, 0.035, cracks*0.9);
Opacity = saturate(max(hole, cracks*0.72));
value = saturate(value);
return float3(value, value, value);
""")

normal = ue.MaterialEditingLibrary.create_material_expression(
    material, ue.MaterialExpressionCustom, -300, 240)
normal.set_editor_property("description", "Shallow bowl normal for the crater")
normal.set_editor_property("output_type", ue.CustomMaterialOutputType.CMOT_FLOAT3)
normal.set_editor_property("inputs", [texture_input])
normal.set_editor_property("code", r"""
float2 p = UV * 2.0 - 1.0;
float lengthP = max(length(p), 0.0001);
float2 direction = p / lengthP;
float ridge = lengthP - (0.43 + 0.035*sin(atan2(p.y,p.x)*7.0 + 0.4));
float slope = 0.20 * exp(-pow(ridge/0.13, 2.0)) - 0.36 * exp(-pow(lengthP/0.15, 2.0));
return normalize(float3(direction * slope, 1.0));
""")

if not ue.MaterialEditingLibrary.connect_material_expressions(texcoord, "", surface, "UV"):
    raise RuntimeError("Cannot connect crater UVs")
if not ue.MaterialEditingLibrary.connect_material_expressions(texcoord, "", normal, "UV"):
    raise RuntimeError("Cannot connect crater normal UVs")
if not ue.MaterialEditingLibrary.connect_material_property(surface, "", ue.MaterialProperty.MP_BASE_COLOR):
    raise RuntimeError("Cannot connect crater color")
if not ue.MaterialEditingLibrary.connect_material_property(surface, "Opacity", ue.MaterialProperty.MP_OPACITY):
    raise RuntimeError("Cannot connect crater opacity")
if not ue.MaterialEditingLibrary.connect_material_property(normal, "", ue.MaterialProperty.MP_NORMAL):
    raise RuntimeError("Cannot connect crater normal")
roughness = ue.MaterialEditingLibrary.create_material_expression(
    material, ue.MaterialExpressionConstant, -300, 480)
roughness.set_editor_property("r", 0.96)
if not ue.MaterialEditingLibrary.connect_material_property(roughness, "", ue.MaterialProperty.MP_ROUGHNESS):
    raise RuntimeError("Cannot connect crater roughness")

ue.MaterialEditingLibrary.recompile_material(material)
save(material)

projectile = load(PROJECTILE_PATH)
projectile_cdo = ue.get_default_object(projectile.generated_class())
projectile_cdo.set_editor_property("impact_decal_material", material)
save(projectile)
ue.log("ER_BULLET_IMPACT_SAVED: procedural crater decal assigned to projectile defaults")
