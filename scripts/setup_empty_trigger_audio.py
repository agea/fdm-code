"""Import user-approved Freesound 387482; change only firearms' empty-trigger sound."""
from pathlib import Path
import hashlib,json,unreal as ue
project=Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir()))
source=project/'Assets/weapons/SourceAssets/Freesound/387482'
manifest=json.loads((source/'manifest.json').read_text())
wav=source/manifest['runtime']
assert hashlib.sha256(wav.read_bytes()).hexdigest()==manifest['runtime_sha256']
assets=ue.EditorAssetLibrary
folder='/Game/EclipseRecoil/Audio/Weapons/Waves'
name='S_ER_EmptyTrigger'
task=ue.AssetImportTask()
for k,v in dict(filename=str(wav),destination_path=folder,destination_name=name,automated=True,replace_existing=True,save=False).items():task.set_editor_property(k,v)
ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
sound=ue.load_asset(folder+'/'+name);assert sound
sound.set_editor_property('looping',False)
sound.set_editor_property('loading_behavior',ue.SoundWaveLoadingBehavior.FORCE_INLINE)
sound.set_sound_asset_compression_type(ue.SoundAssetCompressionType.PCM)
assets.set_metadata_tag(sound,'ERAudioSourceSHA256',manifest['runtime_sha256'])
assets.set_metadata_tag(sound,'Attribution',(source/'CREDIT.txt').read_text())
assert sound.get_editor_property('num_channels')==1
assert assets.save_loaded_asset(sound,False)
for key in ['Pistol','SMG','Shotgun','Assault','Carbine','Sniper','Launcher']:
    weapon=ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/'+key+'/DA_'+key);assert weapon
    weapon.set_editor_property('empty_trigger_sound',sound)
    assert assets.save_loaded_asset(weapon,False)
ue.log('ER_EMPTY_TRIGGER_READY firearms=7 Freesound=387482')
