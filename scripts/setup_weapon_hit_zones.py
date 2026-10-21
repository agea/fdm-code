"""Apply only workbook ballistics to six owned firearm assets through Unreal."""
import json
from pathlib import Path
import unreal as ue

project = Path(ue.Paths.project_dir())
profiles = json.loads((project / 'docs/references/cs2-weapon-ballistics-2026-03-18.json').read_text())['weapons']
for key, profile in profiles.items():
    data = ue.load_asset(f'/Game/EclipseRecoil/Weapons/Sketchfab/{key}/DA_{key}')
    assert data and data.get_editor_property('payload') == ue.ERPayload.BULLET, key
    settings = {prop: profile[prop] for prop in ('damage', 'armor_ratio', 'range_modifier', 'headshot_multiplier')}
    settings['use_body_hit_zones'] = True
    for prop, value in settings.items():
        data.set_editor_property(prop, value)
        assert abs(float(data.get_editor_property(prop)) - float(value)) < .00001, (key, prop)
    assert ue.EditorAssetLibrary.save_loaded_asset(data, only_if_is_dirty=False), key
    ue.log(f'ER_WEAPON_ZONES_READY {key} {settings}')
