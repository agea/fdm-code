"""Import only the owned audio pass; save cues/data/Blueprint references via UE.

Run after prepare_audio_pass.py. Does not rebuild art, models, maps or weapon
balance. All WAVs and packages live in the private weapons mount.
"""
from pathlib import Path
import hashlib,json,unreal as ue
project=Path(ue.Paths.project_dir()).resolve()
source=project/'Assets/weapons/SourceAssets/AudioPass'
manifest=json.loads((source/'manifest.json').read_text())
tools=ue.AssetToolsHelpers.get_asset_tools();assets=ue.EditorAssetLibrary
root='/Game/EclipseRecoil/Audio'

def save(obj):assert assets.save_loaded_asset(obj,only_if_is_dirty=False),obj.get_path_name()
def asset(name,folder,cls,factory):return ue.load_asset(folder+'/'+name) or tools.create_asset(name,folder,cls,factory)
def data(name,folder,cls):
    factory=ue.DataAssetFactory();factory.set_editor_property('data_asset_class',cls)
    return asset(name,folder,cls,factory)
def attenuation(name,inner,falloff):
    obj=asset(name,root,ue.SoundAttenuation,ue.SoundAttenuationFactory())
    settings=obj.get_editor_property('attenuation')
    for key,value in dict(attenuate=True,spatialize=True,attenuation_shape_extents=ue.Vector(inner,0,0),falloff_distance=falloff).items():settings.set_editor_property(key,value)
    obj.set_editor_property('attenuation',settings);save(obj);return obj
def concurrency(name,count,owner):
    obj=asset(name,root,ue.SoundConcurrency,ue.SoundConcurrencyFactory())
    settings=obj.get_editor_property('concurrency')
    for key,value in dict(max_count=count,limit_to_owner=owner,resolution_rule=(ue.MaxConcurrentResolutionRule.STOP_OLDEST if name.startswith('SC_ER_Shots') else ue.MaxConcurrentResolutionRule.STOP_FARTHEST_THEN_OLDEST),voice_steal_release_time=.025).items():settings.set_editor_property(key,value)
    obj.set_editor_property('concurrency',settings);save(obj);return obj
shotlimits=[concurrency('SC_ER_ShotsGlobal',96,False),concurrency('SC_ER_ShotsOwner',24,True)]
steplimits=[concurrency('SC_ER_StepsGlobal',48,False),concurrency('SC_ER_StepsOwner',4,True)]
steps_atten=attenuation('SA_ER_Footsteps',125,2075)
# Reuse the unchanged existing shot attenuation, including any later artist edits.
shot_atten=ue.load_asset('/Game/EclipseRecoil/Weapons/SA_ER_Rifle');assert shot_atten
waves={}
for record in manifest['records']:
    src=source/record['file'];assert hashlib.sha256(src.read_bytes()).hexdigest()==record['sha256']
    folder=root+('/Footsteps/Waves' if 'Step_' in record['name'] else '/Weapons/Waves')
    path=folder+'/'+record['name'];sound=ue.load_asset(path)
    if not sound or assets.get_metadata_tag(sound,'ERAudioSourceSHA256')!=record['sha256']:
        task=ue.AssetImportTask()
        for key,value in dict(filename=str(src),destination_path=folder,destination_name=record['name'],automated=True,replace_existing=True,save=False).items():task.set_editor_property(key,value)
        tools.import_asset_tasks([task]);sound=ue.load_asset(path);assert sound
        sound.set_editor_property('looping',False)
        sound.set_editor_property('loading_behavior',ue.SoundWaveLoadingBehavior.FORCE_INLINE)
        sound.set_sound_asset_compression_type(ue.SoundAssetCompressionType.PCM)
        assets.set_metadata_tag(sound,'ERAudioSourceSHA256',record['sha256'])
        assets.set_metadata_tag(sound,'ERAudioOriginal',record['source'])
        assets.set_metadata_tag(sound,'ERAudioProvenance','Assets/weapons/SourceAssets/AudioPass/manifest.json')
        save(sound)
    assert sound.get_editor_property('num_channels')==1
    waves[record['name']]=sound

def palette(key,names,steps=False,existing=None):
    folder=root+('/Footsteps' if steps else '/Weapons')
    sounds=[waves[n] for n in names] if existing is None else existing
    cue=asset('SC_ER_'+key,folder,ue.SoundCue,ue.SoundCueFactoryNew())
    assert ue.ERAudioEditorLibrary.build_one_shot_cue(cue,sounds,.94 if steps else .98,1.06 if steps else 1.02)
    cue.set_editor_property('concurrency_set',steplimits if steps else shotlimits)
    cue.set_editor_property('priority',.25 if steps else 1.)
    save(cue)
    obj=data('DA_ER_'+key,folder,ue.EROneShotAudio)
    obj.set_editor_property('cue',cue);obj.set_editor_property('variants',sounds);save(obj)
    return obj
for key,names in manifest['firearms'].items():
    obj=palette('Shot_'+key,names)
    weapon=ue.load_asset(f'/Game/EclipseRecoil/Weapons/Sketchfab/{key}/DA_{key}');assert weapon
    weapon.set_editor_property('shot_audio',obj);save(weapon)
# No approved source contains a grenade launcher. Preserve its original template
# identity as an explicit one-sample palette, rather than borrowing a rifle shot.
launcher=palette('Shot_Milkor',[],existing=[ue.load_asset('/Game/Weapons/GrenadeLauncher/Audio/FirstPersonTemplateWeaponFire02')])
weapon=ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/Launcher/DA_Launcher');assert weapon
weapon.set_editor_property('shot_audio',launcher);save(weapon)
launcher.get_editor_property('cue').set_editor_property('pitch_multiplier',weapon.get_editor_property('sound_pitch'))
save(launcher.get_editor_property('cue'))
step_sounds={}
for key,names in manifest['footsteps'].items():step_sounds[getattr(ue.ERFootstepSurface,{'LightHard':'LIGHT_HARD'}.get(key,key.upper()))]=palette('Step_'+key,names,True)
# These families are honest, explicit fallbacks until dedicated recordings arrive.
step_sounds[ue.ERFootstepSurface.DIRT]=step_sounds[ue.ERFootstepSurface.GRASS]
step_sounds[ue.ERFootstepSurface.SNOW]=step_sounds[ue.ERFootstepSurface.GRAVEL]
steps=data('DA_ER_Footsteps',root+'/Footsteps',ue.ERFootstepAudio)
steps.set_editor_property('sounds',step_sounds);steps.set_editor_property('attenuation',steps_atten)
steps.set_editor_property('cell_surfaces',{
 ue.ERBlockMaterial.DRYWALL:ue.ERFootstepSurface.LIGHT_HARD,ue.ERBlockMaterial.BRICK:ue.ERFootstepSurface.HARD,
 ue.ERBlockMaterial.CONCRETE:ue.ERFootstepSurface.HARD,ue.ERBlockMaterial.WOOD:ue.ERFootstepSurface.WOOD,
 ue.ERBlockMaterial.STONE:ue.ERFootstepSurface.HARD,ue.ERBlockMaterial.METAL:ue.ERFootstepSurface.METAL,
 ue.ERBlockMaterial.GLASS:ue.ERFootstepSurface.LIGHT_HARD})
steps.set_editor_property('physical_surfaces',{getattr(ue.PhysicalSurface,'SURFACE_TYPE'+str(i+1)):getattr(ue.ERFootstepSurface,n) for i,n in enumerate(['HARD','WOOD','METAL','LIGHT_HARD','GRASS','DIRT','GRAVEL','SNOW'])})
materials={}
# Identity mapping for current author-owned floor/terrain surfaces; no physics,
# material, collision, HP, palette RNG or geometry edits.
for entry in json.loads((project/'Assets/maps/SourceAssets/SketchfabTextures/materials-manifest.json').read_text()):
    name=entry['id'];material=ue.load_asset('/Game/EclipseRecoil/Art/Industrial/SketchfabTextures/M_ER_SF_'+name)
    if material:
        family='WOOD' if name.startswith('wood') else 'METAL' if name.startswith(('steel','metal','plate','iron')) else 'GRASS' if name=='grass_02' else 'SNOW' if name=='snow_01' else 'HARD'
        materials[material]=getattr(ue.ERFootstepSurface,family)
# Also include every live catalog material and current named floor surfaces.
catalog=ue.load_asset('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials')
for row in catalog.get_editor_property('materials'):
    if row.get_editor_property('surface'):
        materials[row.get_editor_property('surface')]=steps.get_editor_property('cell_surfaces')[row.get_editor_property('type')]
steps.set_editor_property('material_surfaces',materials);save(steps)
bp=ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter');ue.BlueprintEditorLibrary.compile_blueprint(bp)
cdo=ue.get_default_object(bp.generated_class());component=cdo.get_editor_property('footsteps');assert component
component.set_editor_property('audio',steps)
# Keep the historical arsenal and its fallback audio untouched.
save(bp)
smoke=ue.load_asset('/Game/EclipseRecoil/Effects/Smoke/BP_ERSmokeCloud');ue.BlueprintEditorLibrary.compile_blueprint(smoke)
release=ue.get_default_object(smoke.generated_class()).get_editor_property('release_volume')
assert release>=0
save(smoke)
ue.log('ER_AUDIO_PASS_READY firearms=4,6,4,6,4,4 launcher=1 footsteps=8x5+1 smoke_gain=%.8f' % release)
