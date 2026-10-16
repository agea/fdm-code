"""Import the credited public Freesound HQ preview and bind owned explosion audio."""
from pathlib import Path
import unreal as ue
project = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir()))
folder = '/Game/EclipseRecoil/Effects/Grenade/Audio'
tools = ue.AssetToolsHelpers.get_asset_tools()
source = project/'Assets/weapons/SourceAssets/Freesound/162363/ER_Grenade_Explosion.wav'
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
assert .5 < sound.get_editor_property('duration') < .8
assert ue.get_default_object(bp.generated_class()).get_editor_property('explosion_sound') == sound
ue.log('ER_GRENADE_AUDIO_READY mono=1 duration=%.3f inner_cm=500 falloff_cm=7500 concurrency=8' % sound.get_editor_property('duration'))
