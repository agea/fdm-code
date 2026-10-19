"""Own a masked wire material/mesh; preserve Factory originals and cell catalog."""
import unreal as ue
assets=ue.EditorAssetLibrary;tools=ue.AssetToolsHelpers.get_asset_tools();lib=ue.MaterialEditingLibrary
folder='/Game/EclipseRecoil/Art/Industrial'
material_path=folder+'/M_ER_Chainlink'
if not assets.does_asset_exist(material_path):
    material=tools.create_asset('M_ER_Chainlink',folder,ue.Material,ue.MaterialFactoryNew())
    material.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED)
    material.set_editor_property('two_sided',True)
    material.set_editor_property('used_with_instanced_static_meshes',True)
    color=lib.create_material_expression(material,ue.MaterialExpressionConstant3Vector,-400,0)
    color.set_editor_property('constant',ue.LinearColor(.22,.25,.28,1))
    lib.connect_material_property(color,'',ue.MaterialProperty.MP_BASE_COLOR)
    mask=lib.create_material_expression(material,ue.MaterialExpressionTextureSample,-400,200)
    mask.set_editor_property('texture',assets.load_asset('/Game/EclipseRecoil/Art/FactoryEnvironment/Textures/T_ChainlinkFence_MSK'))
    mask.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    lib.connect_material_property(mask,'R',ue.MaterialProperty.MP_OPACITY_MASK)
    for prop,value in [(ue.MaterialProperty.MP_ROUGHNESS,.65),(ue.MaterialProperty.MP_METALLIC,.8)]:
        node=lib.create_material_expression(material,ue.MaterialExpressionConstant,-200,400)
        node.set_editor_property('r',value);lib.connect_material_property(node,'',prop)
    lib.recompile_material(material)
    if not assets.save_loaded_asset(material):raise RuntimeError('Cannot save owned wire material')
material=assets.load_asset(material_path)
if assets.get_metadata_tag(material,'ERWireSamplerRevision')!='1':
    node=lib.get_material_property_input_node(material,ue.MaterialProperty.MP_OPACITY_MASK)
    node.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    lib.recompile_material(material);assets.set_metadata_tag(material,'ERWireSamplerRevision','1')
    if not assets.save_loaded_asset(material):raise RuntimeError('Cannot save the linear-color mask sampler')
mesh_path=folder+'/SM_ER_Chainlink'
if not assets.does_asset_exist(mesh_path):
    mesh=assets.duplicate_asset('/Game/EclipseRecoil/Art/FactoryEnvironment/Meshes/SM_Fence_01',mesh_path)
    if not mesh:raise RuntimeError('Cannot duplicate industrial wire mesh')
    mesh.set_material(1,material)
    if not assets.save_loaded_asset(mesh):raise RuntimeError('Cannot save owned wire mesh')
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
if assets.get_metadata_tag(template,'ERIndustrialWireRevision')!='1':
    recipe=template.get_editor_property('central_building');recipe.set_editor_property('fence_mesh',assets.load_asset(mesh_path))
    template.set_editor_property('central_building',recipe)
    for seed in [0,42,7919,-17]:
        if not template.validate_layout(seed):raise RuntimeError('Invalid wire revision')
    assets.set_metadata_tag(template,'ERIndustrialWireRevision','1')
    if not assets.save_loaded_asset(template):raise RuntimeError('Cannot save wire binding')
print('ER_INDUSTRIAL_WIRE_COMPLETE')
