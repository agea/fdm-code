"""Inspect the current solid steel barrier and shared industrial prop materials."""
import unreal as ue
assets=ue.EditorAssetLibrary
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
recipe=template.get_editor_property('central_building')
module=recipe.get_editor_property('fence_module')
print('ER_STEEL_BARRIER',module.get_path_name(),module.get_material(0).get_path_name(),module.get_material(0).get_base_material().get_editor_property('blend_mode'))
for name in ['MI_Container01','MI_Platform_01']:
    obj=assets.load_asset('/Game/EclipseRecoil/Art/FactoryEnvironment/Materials/'+name)
    if obj:print('ER_INDUSTRIAL_MATERIAL',name,obj.get_base_material().get_editor_property('blend_mode'))
