"""Import the original menu WAV into private maps content and bind the controller."""
from pathlib import Path
import hashlib
import json
import unreal as ue

project = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir()))
source = project / 'Assets/maps/SourceAssets/MenuMusic/RR-Menu-Loop.wav'
folder = '/Game/EclipseRecoil/Art/Menu/Audio'
task = ue.AssetImportTask()
for key, value in dict(filename=str(source), destination_path=folder,
                       destination_name='S_RR_MenuLoop', automated=True,
                       replace_existing=True, save=True).items():
    task.set_editor_property(key, value)
ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound = ue.load_asset(folder + '/S_RR_MenuLoop')
assert sound, 'Menu WAV import failed'
sound.set_editor_property('looping', True)
sound.set_editor_property('sound_group', ue.SoundGroup.SOUNDGROUP_MUSIC)
sound.set_editor_property('compression_quality', 80)
sound.set_sound_asset_compression_type(ue.SoundAssetCompressionType.BINK_AUDIO)
sound.set_editor_property('loading_behavior', ue.SoundWaveLoadingBehavior.PRIME_ON_LOAD)
bp = ue.load_asset('/Game/EclipseRecoil/Core/BP_ERPlayerController')
assert bp, 'Missing player controller Blueprint'
ue.get_default_object(bp.generated_class()).set_editor_property('menu_music', sound)
ue.get_default_object(bp.generated_class()).set_editor_property('menu_music_volume', .35)
ue.BlueprintEditorLibrary.compile_blueprint(bp)
for asset in [sound, bp]:
    assert ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
assert sound.get_editor_property('num_channels') == 2
assert 149 < sound.get_editor_property('duration') < 151
assert ue.get_default_object(bp.generated_class()).get_editor_property('menu_music') == sound
report = dict(source=source.name, sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
              asset=sound.get_path_name(), duration=sound.get_editor_property('duration'),
              channels=2, codec='BinkAudio', quality=80, looping=True, volume=.35)
(source.parent / 'import-receipt.json').write_text(json.dumps(report, indent=2) + '\n')
ue.log('ER_MENU_MUSIC_IMPORT_PASS ' + json.dumps(report))
