"""Apply the central R&R XM1014 override, preserving spread/recoil/presentation."""
import runpy
from pathlib import Path
import unreal as ue
rules=runpy.run_path(str(Path(ue.Paths.project_dir())/'scripts/weapon_gameplay_overrides.py'))
data=ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/Shotgun/DA_Shotgun')
assert data
rules['apply'](data,'Shotgun')
assert ue.EditorAssetLibrary.save_loaded_asset(data,only_if_is_dirty=False)
ue.log('ER_SHOTGUN_READY pellets=8 damage=22 interval=.35')
