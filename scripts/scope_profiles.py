"""Owned rifle scope tuning; keys preserve the serialized arsenal IDs."""
SCOPE_PROFILES = {
 'Assault':dict(scope_mode=1,zoom_fov=75.),
 'Launcher':dict(scope_mode=1,zoom_fov=75.,scoped_launch_range_multiplier=1.5,iron_sight_offset=(18.493,.034,14.8316),iron_sight_eye_distance=30.,iron_sight_drop=2.5,iron_sight_pitch=3.),
 'Carbine':dict(scope_mode=2,zoom_fov=40.,unscope_after_shot=False,scope_settle_duration=.16,no_scope_spread_degrees=3.,scoped_running_spread_degrees=4.,bloom_per_shot_degrees=.5,max_bloom_degrees=3.,bloom_recovery_delay=.3,bloom_recovery_degrees_per_second=6.),
 'Sniper':dict(scope_mode=2,zoom_fov=40.,unscope_after_shot=True,scope_settle_duration=.25,no_scope_spread_degrees=6.,scoped_running_spread_degrees=7.5,bloom_per_shot_degrees=.8,max_bloom_degrees=3.,bloom_recovery_delay=.2,bloom_recovery_degrees_per_second=2.)
}
