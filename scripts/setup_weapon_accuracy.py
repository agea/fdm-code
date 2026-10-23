"""Save only workbook accuracy fields on the six existing firearm data assets."""
import json
import runpy
from pathlib import Path
import unreal as ue

profiles = json.loads((Path(ue.Paths.project_dir()) / 'docs/references/cs2-weapon-ballistics-2026-03-18.json').read_text())['weapons']

def mode(values):
    result = ue.ERWeaponAccuracyMode()
    for prop, value in values.items():
        result.set_editor_property(prop, value)
    return result

for key, profile in profiles.items():
    data = ue.load_asset(f'/Game/EclipseRecoil/Weapons/Sketchfab/{key}/DA_{key}')
    assert data and data.payload == ue.ERPayload.BULLET, key
    spec = profile['accuracy']
    data.set_editor_property('hip_accuracy', mode(spec['hip']))
    data.set_editor_property('scoped_accuracy', mode(spec['scoped']))
    data.set_editor_property('use_worksheet_accuracy', True)
    data.set_editor_property('shot_interval', spec['cycle_time'])
    for name in ('hip', 'scoped'):
        saved = data.get_editor_property(name + '_accuracy')
        for prop, value in spec[name].items():
            assert abs(saved.get_editor_property(prop) - value) < .0001, (key, name, prop)
    runpy.run_path(str(Path(ue.Paths.project_dir())/'scripts/weapon_gameplay_overrides.py'))['apply'](data,key)
    assert ue.EditorAssetLibrary.save_loaded_asset(data, only_if_is_dirty=False), key
    ue.log(f'ER_WORKSHEET_ACCURACY_READY {key} hip={spec["hip"]} scoped={spec["scoped"]}')
