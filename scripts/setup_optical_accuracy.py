"""Update only Sniper/Marksman hip and low-posture motion accuracy through Unreal."""
from pathlib import Path
import runpy
import unreal as ue

profiles = runpy.run_path(str(Path(ue.Paths.project_dir()) / 'scripts/scope_profiles.py'))['SCOPE_PROFILES']
for key in ('Carbine', 'Sniper'):
    data = ue.load_asset(f'/Game/EclipseRecoil/Weapons/Sketchfab/{key}/DA_{key}')
    assert data, key
    for prop in ('no_scope_spread_degrees', 'scoped_low_moving_spread_degrees'):
        value = profiles[key][prop]
        data.set_editor_property(prop, value)
        assert abs(data.get_editor_property(prop) - value) < 1e-5, (key, prop)
    assert ue.EditorAssetLibrary.save_loaded_asset(data, only_if_is_dirty=False), key
    ue.log(f'ER_OPTICAL_ACCURACY_READY {key} hip={data.no_scope_spread_degrees} low_motion={data.scoped_low_moving_spread_degrees}')
