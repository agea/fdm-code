"""Save only worksheet recoil fields on the six owned firearm data assets."""
import json
from pathlib import Path
import unreal as ue

profiles = json.loads((Path(ue.Paths.project_dir()) / 'docs/references/cs2-weapon-ballistics-2026-03-18.json').read_text())['weapons']
for key, profile in profiles.items():
    data = ue.load_asset(f'/Game/EclipseRecoil/Weapons/Sketchfab/{key}/DA_{key}')
    assert data and data.payload == ue.ERPayload.BULLET, key
    spec = profile['recoil']
    for name in ('hip', 'scoped'):
        mode = ue.ERWeaponRecoilMode()
        for prop, value in spec[name].items():
            mode.set_editor_property(prop, value)
        data.set_editor_property(name + '_recoil', mode)
        for prop, value in spec[name].items():
            assert abs(data.get_editor_property(name + '_recoil').get_editor_property(prop) - value) < .0001
    data.set_editor_property('random_recoil_pattern', spec['pattern'] == 'Random')
    data.set_editor_property('recoil_pattern_seed', spec['project_pattern_seed'])
    data.set_editor_property('use_worksheet_recoil', True)
    assert ue.EditorAssetLibrary.save_loaded_asset(data, only_if_is_dirty=False), key
    ue.log(f'ER_WORKSHEET_RECOIL_READY {key} hip={spec["hip"]} scoped={spec["scoped"]} pattern={spec["pattern"]}')
