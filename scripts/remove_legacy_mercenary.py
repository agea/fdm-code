"""Unreal-only removal gate for the two retired Mercenary trees.

Default: preview and reference validation, no deletion. After reviewing the
verified migration, add -ERDeleteLegacyMercenary to the Unreal command line.
Keeps the independent Characters/FirstPerson rig and poses untouched.
"""
import json
from pathlib import Path
import unreal as ue

folders = ['/Game/Mercenary_Male_WaveforceStudio', '/Game/EclipseRecoil/Characters/Mercenary']
registry = ue.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
registry.wait_for_completion()
options = ue.AssetRegistryDependencyOptions(include_hard_package_references=True,
    include_soft_package_references=True, include_searchable_names=False,
    include_soft_management_references=True, include_hard_management_references=True)
packages = {str(asset.package_name) for folder in folders
            for asset in registry.get_assets_by_path(folder, recursive=True, include_only_on_disk_assets=True)}
outside = {p: [str(r) for r in registry.get_referencers(p, options) if str(r) not in packages]
           for p in packages}
outside = {p: refs for p, refs in outside.items() if refs}
project = Path(ue.Paths.project_dir()).resolve()
result = {'folders': folders, 'packages': len(packages), 'external_referencers': outside, 'deleted': False}
path = project / 'Saved/AssetAudit/mercenary-removal-ready.json'
path.parent.mkdir(parents=True, exist_ok=True)
path.write_text(json.dumps(result, indent=2)+'\n')
if outside:
    raise RuntimeError('Removal refused: packages outside the retired trees still refer to them: ' + json.dumps(outside))
if '-ERDeleteLegacyMercenary' in ue.SystemLibrary.get_command_line().split():
    # The user-selected opt-in is the only mutating path. Unreal owns package
    # removal and updates the registry; do not unlink binary files manually.
    for folder in folders:
        if ue.EditorAssetLibrary.does_directory_exist(folder) and not ue.EditorAssetLibrary.delete_directory(folder):
            raise RuntimeError('Unreal could not remove: ' + folder)
    result['deleted'] = True
    path.write_text(json.dumps(result, indent=2)+'\n')
    ue.log('ER_MERCENARY_REMOVED packages=%d' % len(packages))
else:
    ue.log('ER_MERCENARY_REMOVAL_READY packages=%d external_referencers=0 deleted=0' % len(packages))
