"""Migrate only the active rifle and launcher scope profiles."""
import unreal as ue
from pathlib import Path
import runpy
profiles=runpy.run_path(str(Path(ue.Paths.project_dir())/'scripts/scope_profiles.py'))['SCOPE_PROFILES']
for key,values in profiles.items():
 data=ue.load_asset(f'/Game/EclipseRecoil/Weapons/Sketchfab/{key}/DA_{key}')
 assert data,key
 for prop,value in values.items():data.set_editor_property(prop,ue.Vector(*value) if prop=='iron_sight_offset' else value)
 assert ue.EditorAssetLibrary.save_loaded_asset(data,only_if_is_dirty=False)
 ue.log(f'ER_SCOPE_READY {key} {values}')
