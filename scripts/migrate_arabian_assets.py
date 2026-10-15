"""Stage and namespace a small MOUT_Arabian closure through Unreal only.

Run stage in Saved/ArabianInspection (read-only source mount), then rehome in
Saved/ArabianMigration. Source packages are never saved or renamed.
"""
import hashlib, json, re
from pathlib import Path
import unreal as ue

ROOT = Path(__file__).resolve().parents[1]
PRIVATE = ROOT / 'Assets/maps/SourceAssets/ArabianEnvironment'
SCRATCH = ROOT / 'Saved/ArabianMigration'
DEST = '/Game/EclipseRecoil/Art/ArabianEnvironment'
ROOTS = ['/Game/MOUT_Arabian/Meshes/ModularAssets/' + n for n in
         ['SM_VillageKit_2x2', 'SM_VillageKit_2x1', 'SM_VillageKit_Door_01a']]
ROOTS += ['/Game/MOUT_Arabian/Materials/SubstanceTilingMaterials/M_StoneFloor_SBSAR/M_StoneFloor']
PRIVATE.mkdir(parents=True, exist_ok=True)
match = re.search(r'ERArabianPhase=(\w+)', ue.SystemLibrary.get_command_line())
if not match: raise RuntimeError('Specify ERArabianPhase=stage or rehome')
phase = match.group(1)
registry = ue.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
tools = ue.AssetToolsHelpers.get_asset_tools()
options = ue.MigrationOptions(prompt=False, ignore_dependencies=False,
                              asset_conflict=ue.AssetMigrationConflict.SKIP)
if (PRIVATE / 'migration_receipt.json').exists():
    ue.log('ER_ARABIAN_MIGRATION_PRESERVED')
elif phase == 'stage':
    (SCRATCH / 'Content').mkdir(parents=True, exist_ok=True)
    (SCRATCH / 'ArabianMigration.uproject').write_text(json.dumps({
        'FileVersion': 3, 'EngineAssociation': '5.8', 'Plugins': [
            {'Name': 'PythonScriptPlugin', 'Enabled': True},
            {'Name': 'EditorScriptingUtilities', 'Enabled': True}]}))
    for path in ROOTS:
        if not ue.load_asset(path): raise RuntimeError('Cannot load ' + path)
    tools.migrate_packages(ROOTS, str(SCRATCH / 'Content'), options)
    for path in ROOTS:
        if not (SCRATCH / 'Content' / (path.removeprefix('/Game/') + '.uasset')).exists():
            raise RuntimeError('Incomplete stage ' + path)
    ue.log('ER_ARABIAN_STAGE_COMPLETE')
elif phase == 'rehome':
    renames = []
    for data in registry.get_assets_by_path('/Game/MOUT_Arabian', recursive=True):
        if str(data.asset_class_path.asset_name) == 'ObjectRedirector': continue
        new = DEST + '/' + str(data.package_name).removeprefix('/Game/MOUT_Arabian/')
        renames.append(ue.AssetRenameData(data.get_asset(), new.rsplit('/', 1)[0], new.rsplit('/', 1)[1]))
    if renames and not tools.rename_assets(renames): raise RuntimeError('Namespace migration failed')
    if not ue.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True):
        raise RuntimeError('Cannot save private namespace')
    base = SCRATCH / 'Content/EclipseRecoil/Art/ArabianEnvironment'
    closure = ['/Game/EclipseRecoil/Art/ArabianEnvironment/' + str(p.relative_to(base).with_suffix(''))
               for p in base.rglob('*.uasset')]
    if not closure: raise RuntimeError('Empty dependency closure')
    tools.migrate_packages(closure, str(ROOT / 'Content'), options)
    files = list((ROOT / 'Assets/maps/Content/EclipseRecoil/Art/ArabianEnvironment').rglob('*.uasset'))
    if len(files) != len(closure): raise RuntimeError('Incomplete final migration')
    receipt = {'source_project': '/Users/agea/Documents/Unreal Projects/FactoryEnvironmentCollect',
               'source_namespace': '/Game/MOUT_Arabian', 'source_roots': ROOTS,
               'destination': DEST, 'packages': len(files),
               'bytes': sum(p.stat().st_size for p in files),
               'hashes': {str(p.relative_to(ROOT / 'Assets/maps')): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
    (PRIVATE / 'migration_receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    ue.log('ER_ARABIAN_MIGRATION_COMPLETE packages=' + str(len(files)))
else:
    raise RuntimeError('Unknown phase')
