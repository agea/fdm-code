"""Remove retired translucent variants after saving current Blueprint defaults.

Uses the existing engine LevelColorationLitMaterial plus a transient gray tint at runtime.
No skin copies, textures or new material assets are generated.
"""
import unreal as ue
assets = ue.EditorAssetLibrary
root = '/Game/EclipseRecoil/Characters/Protection'
base = ue.load_asset('/Engine/EngineDebugMaterials/LevelColorationLitMaterial')
assert base
names = [str(n) for n in ue.MaterialEditingLibrary.get_vector_parameter_names(base)]
assert 'Color' in names, names
for name in ['BP_ERCharacter', 'BP_ERTrainingTarget']:
    bp = ue.load_asset('/Game/EclipseRecoil/Core/'+name)
    assert bp
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assert ue.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
paths = assets.list_assets(root, recursive=True, include_folder=False)
for path in paths:
    outside = [r for r in assets.find_package_referencers_for_asset(path, load_assets_to_confirm=True)
               if not r.startswith(root+'/')]
    if outside:
        raise RuntimeError('Retired protection asset still referenced: '+path+' '+str(outside))
if assets.does_directory_exist(root):
    assert assets.delete_directory(root), 'Cannot delete retired protection variants'
assert not assets.list_assets(root, recursive=True, include_folder=False)
ue.log('ER_PROTECTION_GRAY_READY deleted_variants=%d new_assets=0 base=%s' % (len(paths),base.get_path_name()))
