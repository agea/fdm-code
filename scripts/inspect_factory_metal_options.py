"""Read migrated Factory metal materials and export their source color textures for comparison."""
from pathlib import Path
import json
import unreal as ue
root=Path(ue.Paths.project_dir())
out=root/'Saved/Testing/factory-metal-options';out.mkdir(parents=True,exist_ok=True)
assets=ue.EditorAssetLibrary;lib=ue.MaterialEditingLibrary
names=['MI_MetalBeam01','MI_MetalBeam_Painted01','MI_MetalWallPart_01','MI_MetalWall_Painted_02','MI_Platform01','MI_PlatformRailing_01','MI_Platform_01','MI_Container01']
result=[];textures={}
for name in names:
    obj=assets.load_asset('/Game/EclipseRecoil/Art/FactoryEnvironment/Materials/'+name)
    if not obj:continue
    row={'name':name,'path':obj.get_path_name(),'parent':str(obj.get_editor_property('parent')),'textures':{},'scalars':{},'vectors':{}}
    for kind in ['texture','scalar','vector']:
        get_names=getattr(lib,'get_'+kind+'_parameter_names')
        get_value=getattr(lib,'get_material_instance_'+kind+'_parameter_value')
        for param in get_names(obj):
            value=get_value(obj,param)
            row[{'texture':'textures','scalar':'scalars','vector':'vectors'}[kind]][str(param)]=str(value.get_path_name() if kind=='texture' and value else value)
            if kind=='texture' and value and value.get_name().endswith(('_BC','_A')):textures[value.get_name()]=value
    result.append(row)
for name,texture in textures.items():
    task=ue.AssetExportTask();task.object=texture;task.filename=str(out/(name+'.png'));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=ue.TextureExporterPNG()
    if not ue.Exporter.run_asset_export_task(task):raise RuntimeError('Cannot export '+name)
(out/'materials.json').write_text(json.dumps(result,indent=2)+'\n')
print('ER_FACTORY_METAL_OPTIONS_PASS materials='+str(len(result))+' textures='+str(len(textures)))
