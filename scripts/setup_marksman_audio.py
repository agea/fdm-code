"""Import user-approved Freesound 855843; change only Carbine's shot palette."""
from pathlib import Path
import hashlib,json,unreal as ue
project=Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir()))
source=project/'Assets/weapons/SourceAssets/Freesound/855843'
manifest=json.loads((source/'manifest.json').read_text())
wav=source/manifest['runtime']
assert hashlib.sha256(wav.read_bytes()).hexdigest()==manifest['runtime_sha256']
assets=ue.EditorAssetLibrary
folder='/Game/EclipseRecoil/Audio/Weapons/Waves'
name='S_ER_Marksman_Seroutonin'
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
weapon=ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/Carbine/DA_Carbine');assert weapon
palette=ue.load_asset('/Game/EclipseRecoil/Audio/Weapons/DA_ER_Shot_Carbine');assert palette
cue=palette.get_editor_property('cue');assert cue
assert ue.ERAudioEditorLibrary.build_one_shot_cue(cue,[sound],.98,1.02)
palette.set_editor_property('variants',[sound]);weapon.set_editor_property('shot_audio',palette)
for obj in [cue,palette,weapon]:assert assets.save_loaded_asset(obj,False)
assert len(palette.get_editor_property('variants'))==1
ue.log('ER_MARKSMAN_AUDIO_READY Freesound=855843 weapon=Carbine variants=1 duration='+str(manifest['duration']))
