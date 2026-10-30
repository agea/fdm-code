"""Show a tracer on each authorized shot for all saved bullet weapon data."""
import unreal as ue
registry=ue.AssetRegistryHelpers.get_asset_registry()
assets=ue.EditorAssetLibrary
count=0
for record in registry.get_assets_by_path('/Game/EclipseRecoil/Weapons',recursive=True):
    if str(record.asset_class_path.asset_name)!='ERWeaponData':continue
    data=record.get_asset()
    if data.get_editor_property('payload')!=ue.ERPayload.BULLET:continue
    data.set_editor_property('tracer_every_n_shots',1)
    assert assets.save_loaded_asset(data,False)
    assert data.get_editor_property('tracer_every_n_shots')==1
    count+=1
    ue.log('ER_EVERY_SHOT_TRACER '+data.get_path_name())
assert count>=6,count
ue.log('ER_EVERY_SHOT_TRACERS_READY weapons='+str(count))
