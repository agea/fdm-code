"""Update only the active XM1014 data; preserve its authored presentation."""
import unreal as ue
path='/Game/EclipseRecoil/Weapons/Sketchfab/Shotgun/DA_Shotgun'
data=ue.load_asset(path)
assert data, path
for key,value in dict(pellets=6,damage=20.,armor_ratio=1.6,range_modifier=.7,
                      use_body_hit_zones=True,headshot_multiplier=4.,pellet_spread_degrees=3.2,standing_spread_degrees=.25).items():
    data.set_editor_property(key,value)
assert ue.EditorAssetLibrary.save_loaded_asset(data,only_if_is_dirty=False)
ue.log('ER_XM1014_READY pellets=6 damage=20 armor_ratio=1.6 range_modifier=.7 pattern_degrees=3.2 aim_degrees=.25')
