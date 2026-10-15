"""Load and verify the private urban/desert dependency closure in Unreal."""
import hashlib, json
from pathlib import Path
import unreal as ue

root = Path(ue.Paths.project_dir())
folder = '/Game/EclipseRecoil/Art/ArabianEnvironment'
registry = ue.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
options = ue.AssetRegistryDependencyOptions(include_hard_package_references=True,
                                           include_soft_package_references=True)
problems, count = [], 0
for data in registry.get_assets_by_path(folder, recursive=True):
    path = str(data.package_name); obj = data.get_asset(); count += 1
    if not obj: problems.append('Cannot load ' + path)
    if isinstance(obj, ue.MaterialInstanceConstant) and not obj.get_editor_property('parent'):
        problems.append('Missing material parent ' + path)
    for dep in registry.get_dependencies(data.package_name, options):
        dep = str(dep)
        if dep.startswith('/Game/') and not dep.startswith(folder + '/'):
            problems.append(path + ' -> external ' + dep)
        if dep.startswith(folder + '/') and not ue.EditorAssetLibrary.does_asset_exist(dep):
            problems.append('Missing ' + dep)
    if isinstance(obj, ue.StaticMesh):
        for slot in obj.static_materials:
            if not slot.material_interface or not slot.material_interface.get_path_name().startswith(folder + '/'):
                problems.append('Unresolved material ' + path)
receipt = json.loads((root / 'Assets/maps/SourceAssets/ArabianEnvironment/migration_receipt.json').read_text())
runtime = registry.get_assets_by_path(folder + '/Materials/Runtime', recursive=True)
if count != receipt['packages'] + len(runtime): problems.append('Package count mismatch')
if problems: raise RuntimeError('\n'.join(problems))
catalog = root / 'Assets/maps/Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset'
ue.log('ER_ARABIAN_VERIFY_PASS packages=' + str(count) + ' catalog_sha256=' + hashlib.sha256(catalog.read_bytes()).hexdigest())
