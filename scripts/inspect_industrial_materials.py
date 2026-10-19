"""Read industrial material dependencies and export the wire mask for inspection."""
from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir());assets=ue.EditorAssetLibrary
for name in ['MI_Fence_01','M_MetalGrid01','MI_Container01','MI_Platform_01']:
    obj=assets.load_asset('/Game/EclipseRecoil/Art/FactoryEnvironment/Materials/'+name)
    if not obj:continue
    print('ER_WIRE_MATERIAL',name,obj.get_editor_property('parent') if isinstance(obj,ue.MaterialInstanceConstant) else '',obj.get_base_material().get_editor_property('blend_mode'))
    if isinstance(obj,ue.MaterialInstanceConstant):
        print('ER_WIRE_PARAMETERS',name,obj.get_editor_property('base_property_overrides'),obj.get_editor_property('texture_parameter_values'))
for name in ['T_ChainlinkFence_MSK','T_ChainlinkFence_BC']:
    texture=assets.load_asset('/Game/EclipseRecoil/Art/FactoryEnvironment/Textures/'+name)
    task=ue.AssetExportTask();task.object=texture;task.filename=str(root/'Saved/Testing'/(name+'.png'));task.automated=True;task.prompt=False;task.replace_identical=True
    task.exporter=ue.TextureExporterPNG();ue.Exporter.run_asset_export_task(task)
    print('ER_WIRE_EXPORT',task.filename)
