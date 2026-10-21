"""Import user-supplied Sonniss HE and smoke audio."""
from pathlib import Path
import unreal as ue
project = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir()))
folder = '/Game/EclipseRecoil/Effects/Grenade/Audio'
tools = ue.AssetToolsHelpers.get_asset_tools()
source = project/'Assets/weapons/SourceAssets/SonnissGDC2024/ER_Grenade_Explosion.wav'
task = ue.AssetImportTask()
for key, value in dict(filename=str(source), destination_path=folder,
                       destination_name='S_ER_GrenadeExplosion', automated=True,
                       replace_existing=True, save=True).items():
    task.set_editor_property(key, value)
tools.import_asset_tasks([task])
sound = ue.load_asset(folder+'/S_ER_GrenadeExplosion')
assert sound, 'WAV import failed'
sound.set_editor_property('looping', False)
sound.set_editor_property('loading_behavior', ue.SoundWaveLoadingBehavior.FORCE_INLINE)
sound.set_sound_asset_compression_type(ue.SoundAssetCompressionType.PCM)
def asset(name, cls, factory):
    return ue.load_asset(folder+'/'+name) or tools.create_asset(name, folder, cls, factory)
atten = asset('SA_ER_GrenadeExplosion', ue.SoundAttenuation, ue.SoundAttenuationFactory())
settings = atten.get_editor_property('attenuation')
for key, value in dict(attenuate=True, spatialize=True,
                       attenuation_shape_extents=ue.Vector(500, 0, 0), falloff_distance=7500.).items():
    settings.set_editor_property(key, value)
atten.set_editor_property('attenuation', settings)
concurrency = asset('SC_ER_GrenadeExplosion', ue.SoundConcurrency, ue.SoundConcurrencyFactory())
settings = concurrency.get_editor_property('concurrency')
settings.set_editor_property('max_count', 8)
settings.set_editor_property('resolution_rule', ue.MaxConcurrentResolutionRule.STOP_FARTHEST_THEN_OLDEST)
concurrency.set_editor_property('concurrency', settings)
bp = ue.load_asset('/Game/EclipseRecoil/Effects/Grenade/BP_ERGrenadeExplosion')
ue.BlueprintEditorLibrary.compile_blueprint(bp)
cdo = ue.get_default_object(bp.generated_class())
for key, value in dict(explosion_sound=sound, explosion_attenuation=atten, explosion_concurrency=concurrency).items():
    cdo.set_editor_property(key, value)
ue.BlueprintEditorLibrary.compile_blueprint(bp)
for obj in [sound, atten, concurrency, bp]:
    assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
assert sound.get_editor_property('num_channels') == 1
assert 2.3 < sound.get_editor_property('duration') < 2.4
assert ue.get_default_object(bp.generated_class()).get_editor_property('explosion_sound') == sound
ue.log('ER_GRENADE_AUDIO_READY mono=1 duration=%.3f inner_cm=500 falloff_cm=7500 concurrency=8' % sound.get_editor_property('duration'))

task = ue.AssetImportTask()
for key, value in dict(filename=str(source.parent/'ER_Smoke_Release.wav'), destination_path=folder,
                       destination_name='S_ER_SmokeRelease', automated=True, replace_existing=True, save=True).items():
    task.set_editor_property(key, value)
tools.import_asset_tasks([task])
smoke = ue.load_asset(folder+'/S_ER_SmokeRelease')
smoke.set_editor_property('looping', False)
smoke.set_editor_property('loading_behavior', ue.SoundWaveLoadingBehavior.FORCE_INLINE)
smoke.set_sound_asset_compression_type(ue.SoundAssetCompressionType.PCM)
smoke_bp = ue.load_asset('/Game/EclipseRecoil/Effects/Smoke/BP_ERSmokeCloud')
ue.BlueprintEditorLibrary.compile_blueprint(smoke_bp)
for key, value in dict(release_sound=smoke, release_attenuation=atten, release_concurrency=concurrency).items():
    ue.get_default_object(smoke_bp.generated_class()).set_editor_property(key, value)
# Native default is the actual previous .65 playback gain * 10**(-10/20).
# Preserve subsequent Blueprint tuning on reruns; never normalize the source WAV.
if ue.EditorAssetLibrary.get_metadata_tag(smoke_bp, 'ERReleaseGainVersion') != '1':
    ue.EditorAssetLibrary.set_metadata_tag(smoke_bp, 'ERReleaseGainVersion', '1')
ue.log('ER_SMOKE_RELEASE_GAIN %.8f' % ue.get_default_object(smoke_bp.generated_class()).get_editor_property('release_volume'))
ue.BlueprintEditorLibrary.compile_blueprint(smoke_bp)
for obj in [smoke, smoke_bp]:
    assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
assert smoke.get_editor_property('num_channels') == 1
assert 5.99 < smoke.get_editor_property('duration') < 6.01
ue.log('ER_SMOKE_AUDIO_READY duration=6 fade_start=4.5 fade_duration=1.5')
