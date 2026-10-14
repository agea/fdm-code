"""Import the project-local Fab derivative and wire a cosmetic HE burst through Unreal."""
from pathlib import Path
import json
import unreal as ue

source = Path(ue.Paths.project_dir()).resolve() / 'SourceAssets/BombCrackGround'
folder = '/Game/EclipseRecoil/Effects/Grenade'
tools = ue.AssetToolsHelpers.get_asset_tools()
assets = ue.EditorAssetLibrary
assets.make_directory(folder)

def save(obj):
    if not assets.save_loaded_asset(obj, only_if_is_dirty=False): raise RuntimeError('Save failed: '+obj.get_path_name())

def task_for(filename, name):
    task = ue.AssetImportTask()
    for key,value in dict(filename=str(filename), destination_path=folder, destination_name=name,
                          automated=True, save=True).items(): task.set_editor_property(key,value)
    return task

mesh = ue.load_asset(folder+'/SK_ER_BombDebris')
if not mesh:
    task = task_for(source/'SK_ER_BombDebris.fbx','SK_ER_BombDebris')
    options = ue.FbxImportUI()
    for key,value in dict(automated_import_should_detect_type=False, mesh_type_to_import=ue.FBXImportType.FBXIT_SKELETAL_MESH,
        import_as_skeletal=True, import_mesh=True, import_animations=True, import_materials=False,
        import_textures=False, create_physics_asset=False).items(): options.set_editor_property(key,value)
    options.get_editor_property('anim_sequence_import_data').set_editor_property('use_default_sample_rate',True)
    options.get_editor_property('anim_sequence_import_data').set_editor_property('preserve_local_transform',True)
    task.set_editor_property('options', options)
    tools.import_asset_tasks([task])
    ue.log('ER_BOMB_IMPORTED '+str(task.get_editor_property('imported_object_paths')))
    mesh = ue.load_asset(folder+'/SK_ER_BombDebris')
if not mesh: raise RuntimeError('No imported skeletal burst')
animation = ue.load_asset(folder+'/A_ER_BombDebris')
if not animation:
    candidates = [ue.load_asset(path) for path in assets.list_assets(folder,recursive=False)]
    sequences = [asset for asset in candidates if isinstance(asset,ue.AnimSequence)]
    if len(sequences)!=1: raise RuntimeError('Expected one animation: '+str(sequences))
    old = sequences[0].get_path_name().split('.')[0]
    if not assets.rename_asset(old, folder+'/A_ER_BombDebris'): raise RuntimeError('Cannot name burst animation')
    animation = ue.load_asset(folder+'/A_ER_BombDebris')

for surface in ['Surface','Side']:
    samples = {}
    for kind in ['BaseColor','Normal','Roughness']:
        name = 'T_Bomb_'+surface+'_'+kind
        texture = ue.load_asset(folder+'/'+name)
        if not texture:
            source_name = 'Surface' if surface=='Surface' else 'side'
            task = task_for(source/'textures'/('AA__'+source_name+'_'+kind+'.png'),name)
            tools.import_asset_tasks([task]); texture = ue.load_asset(folder+'/'+name)
        if not texture: raise RuntimeError('Missing '+name)
        texture.set_editor_property('max_texture_size',1024)
        texture.set_editor_property('srgb',kind=='BaseColor')
        if kind=='Normal': texture.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP)
        elif kind=='Roughness': texture.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_GRAYSCALE)
        save(texture); samples[kind]=texture
    name = 'M_Bomb_'+surface
    material = ue.load_asset(folder+'/'+name) or tools.create_asset(name,folder,ue.Material,ue.MaterialFactoryNew())
    ue.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property('used_with_skeletal_mesh',True)
    material.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED)
    for index,kind in enumerate(samples):
        sample = ue.MaterialEditingLibrary.create_material_expression(material,ue.MaterialExpressionTextureSample,-400,index*180)
        sample.set_editor_property('texture',samples[kind])
        if kind=='Normal': sample.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif kind=='Roughness': sample.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
        prop = {'BaseColor':ue.MaterialProperty.MP_BASE_COLOR,'Normal':ue.MaterialProperty.MP_NORMAL,'Roughness':ue.MaterialProperty.MP_ROUGHNESS}[kind]
        ue.MaterialEditingLibrary.connect_material_property(sample,'R' if kind=='Roughness' else 'RGB',prop)
    fade = ue.MaterialEditingLibrary.create_material_expression(material,ue.MaterialExpressionScalarParameter,-600,600)
    fade.set_editor_property('parameter_name','DebrisFade'); fade.set_editor_property('default_value',1.)
    dither = ue.MaterialEditingLibrary.create_material_expression(material,ue.MaterialExpressionMaterialFunctionCall,-300,600)
    dither.set_material_function(ue.load_asset('/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA'))
    if not ue.MaterialEditingLibrary.connect_material_expressions(fade,'',dither,'Alpha Threshold'): raise RuntimeError('Fade connection failed')
    ue.MaterialEditingLibrary.connect_material_property(dither,'',ue.MaterialProperty.MP_OPACITY_MASK)
    ue.MaterialEditingLibrary.recompile_material(material); save(material)

if not ue.ERGrenadeExplosion.assign_debris_materials(mesh,ue.load_asset(folder+'/M_Bomb_Surface'),ue.load_asset(folder+'/M_Bomb_Side')):
    raise RuntimeError('Native material assignment failed')
mesh.set_editor_property('positive_bounds_extension',ue.Vector(600,600,500))
mesh.set_editor_property('negative_bounds_extension',ue.Vector(600,600,30))
save(mesh); save(animation)
name = 'BP_ERGrenadeExplosion'
bp = ue.load_asset(folder+'/'+name)
if not bp:
    factory = ue.BlueprintFactory(); factory.set_editor_property('parent_class',ue.ERGrenadeExplosion)
    bp = tools.create_asset(name,folder,ue.Blueprint,factory)
defaults = ue.get_default_object(bp.generated_class())
defaults.get_editor_property('debris').set_skeletal_mesh_asset(mesh)
defaults.get_editor_property('flash').set_static_mesh(ue.load_asset('/Engine/BasicShapes/Sphere'))
defaults.get_editor_property('flash').set_material(0,ue.load_asset('/Game/EclipseRecoil/Weapons/LowPoly/M_ER_Explosion'))
for key,value in dict(debris_mesh=mesh, debris_animation=animation,
    flash_mesh=ue.load_asset('/Engine/BasicShapes/Sphere'),
    flash_material=ue.load_asset('/Game/EclipseRecoil/Weapons/LowPoly/M_ER_Explosion'),
    burst_scale=.75, play_rate=1.5).items(): defaults.set_editor_property(key,value)
ue.BlueprintEditorLibrary.compile_blueprint(bp); save(bp)
projectile = ue.load_asset('/Game/EclipseRecoil/Weapons/BP_ERProjectile')
ue.get_default_object(projectile.generated_class()).set_editor_property('grenade_explosion_class',bp.generated_class())
ue.BlueprintEditorLibrary.compile_blueprint(projectile); save(projectile)
report = {'mesh':mesh.get_path_name(), 'animation':animation.get_path_name(),
          'length':animation.get_play_length(), 'runtime_duration':animation.get_play_length()/1.5,
          'materials':[m.material_interface.get_path_name() for m in mesh.get_editor_property('materials')],
          'effect':bp.get_path_name(), 'source':'SourceAssets/BombCrackGround', 'floor':False}
(source/'import-report.json').write_text(json.dumps(report,indent=2)+'\n')
ue.log('ER_GRENADE_EFFECT_READY '+json.dumps(report))
