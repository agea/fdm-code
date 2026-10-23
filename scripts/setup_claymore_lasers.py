"""Create only the native Claymore's emissive team-color beam material."""
import unreal as ue
path='/Game/EclipseRecoil/Weapons/Sketchfab/Claymore/M_ClaymoreLaser'
material=ue.load_asset(path)
if not material:
    material=ue.AssetToolsHelpers.get_asset_tools().create_asset('M_ClaymoreLaser',path.rsplit('/',1)[0],ue.Material,ue.MaterialFactoryNew())
    material.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT)
    color=ue.MaterialEditingLibrary.create_material_expression(material,ue.MaterialExpressionVectorParameter,0,0)
    color.set_editor_property('parameter_name','Color')
    color.set_editor_property('default_value',ue.LinearColor(.2,.65,1,1))
    ue.MaterialEditingLibrary.connect_material_property(color,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
    ue.MaterialEditingLibrary.recompile_material(material)
    assert ue.EditorAssetLibrary.save_loaded_asset(material)
ue.log('ER_CLAYMORE_LASER_MATERIAL_READY')
