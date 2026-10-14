"""Create project-owned smoke defaults from the user-supplied Niagara effect.
Run after migration and compiling the native Niagara adapter.
"""
from pathlib import Path
import json
import unreal as ue

assets=ue.EditorAssetLibrary
tools=ue.AssetToolsHelpers.get_asset_tools()
folder='/Game/EclipseRecoil/Effects/Smoke'
assets.make_directory(folder)
original='/Game/InteractiveSmoke/ToolObjects/PS/NS_InterSmoke_Sim'
system=ue.load_asset(folder+'/NS_ER_InteractiveSmoke') or assets.duplicate_asset(original,folder+'/NS_ER_InteractiveSmoke')
if not system or not ue.ERSmokeCloud.prepare_owned_system(system): raise RuntimeError('Cannot configure owned smoke system')
if not assets.save_loaded_asset(system,False): raise RuntimeError('Cannot save smoke system')
name='BP_ERSmokeCloud'
bp=ue.load_asset(folder+'/'+name)
if not bp:
    factory=ue.BlueprintFactory();factory.set_editor_property('parent_class',ue.ERSmokeCloud)
    bp=tools.create_asset(name,folder,ue.Blueprint,factory)
d=ue.get_default_object(bp.generated_class())
for key,value in dict(system=system,sim_size=ue.Vector(600,600,400),grid_resolution=40,duration=12,
                      smoke_color=ue.LinearColor(.33,.33,.33,1)).items():d.set_editor_property(key,value)
d.get_editor_property('fumes').set_asset(system)
ue.BlueprintEditorLibrary.compile_blueprint(bp)
if not assets.save_loaded_asset(bp,False):raise RuntimeError('Cannot save smoke Blueprint')
projectile=ue.load_asset('/Game/EclipseRecoil/Weapons/BP_ERProjectile')
ue.get_default_object(projectile.generated_class()).set_editor_property('smoke_cloud_class',bp.generated_class())
ue.BlueprintEditorLibrary.compile_blueprint(projectile)
assets.save_loaded_asset(projectile,False)
report={'source_project_engine':'5.3','engine':ue.SystemLibrary.get_engine_version(),
        'source_system':original,'runtime_system':system.get_path_name(),'cloud':bp.get_path_name(),
        'grid_resolution':40,'sim_size_cm':[600,600,400],'duration':12,
        'max_clouds':4,'collision':False,'bot_vision_changed':False}
(Path(ue.Paths.project_dir())/'SourceAssets/InteractiveSmoke/integration-report.json').write_text(json.dumps(report,indent=2)+'\n')
ue.log('ER_SMOKE_SETUP_COMPLETE '+json.dumps(report))
