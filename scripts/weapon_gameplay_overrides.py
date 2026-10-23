"""R&R gameplay overrides, applied after immutable reference data."""
OVERRIDES = {'Shotgun': dict(pellets=8, damage=22., shot_interval=.35,
                           magazine_size=7, armor_ratio=1.6, range_modifier=.7,
                           headshot_multiplier=4.)}
def apply(data, key):
    for prop, value in OVERRIDES.get(key, {}).items():
        data.set_editor_property(prop, value)
