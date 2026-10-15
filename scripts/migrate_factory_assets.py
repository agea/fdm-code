"""Unreal migration of a selected Factory dependency closure via an isolated project.

Run `stage` in the supplied Factory project, then `rehome` in the scratch
Saved/FactoryMigration project. Vendor packages are renamed/saved through Unreal
before migration to the existing private Art mount. No source package is saved.
"""
import hashlib,json,re
from pathlib import Path
import unreal as ue
PROJECT=Path(__file__).resolve().parents[1]
PRIVATE=PROJECT/'Assets/maps/SourceAssets/FactoryEnvironment'
selection_bytes=(PRIVATE/'selection.json').read_bytes()
SCRATCH=PROJECT/'Saved'/('FactoryMigration-'+hashlib.sha256(selection_bytes).hexdigest()[:12])
match=re.search(r'ERFactoryPhase=(\w+)',ue.SystemLibrary.get_command_line())
if not match:raise RuntimeError('Pass -ERFactoryPhase=stage or rehome')
phase=match.group(1)
registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
tools=ue.AssetToolsHelpers.get_asset_tools()
options=ue.MigrationOptions(prompt=False,ignore_dependencies=False,asset_conflict=ue.AssetMigrationConflict.SKIP)
manifest=json.loads((PRIVATE/'selection.json').read_text())
roots=manifest['source_meshes']
if phase=='stage' and (SCRATCH/'migration_complete.json').exists():
    ue.log('ER_FACTORY_MIGRATION_PRESERVED '+str(SCRATCH))
elif phase=='rehome' and (SCRATCH/'migration_complete.json').exists():
    ue.log('ER_FACTORY_MIGRATION_PRESERVED '+str(SCRATCH))
elif phase=='stage':
    (SCRATCH/'Content').mkdir(parents=True,exist_ok=True)
    project=SCRATCH/'FactoryMigration.uproject'
    if not project.exists():project.write_text(json.dumps({'FileVersion':3,'EngineAssociation':'5.8','Plugins':[{'Name':'PythonScriptPlugin','Enabled':True},{'Name':'EditorScriptingUtilities','Enabled':True}]}))
    for path in roots:
        if not ue.load_asset(path):raise RuntimeError('Missing downloaded package '+path)
    tools.migrate_packages(roots,str(SCRATCH/'Content'),options)
    for path in roots:
        if not (SCRATCH/'Content'/(path.removeprefix('/Game/')+'.uasset')).is_file():raise RuntimeError('Missing staged mesh '+path)
    ue.log('ER_FACTORY_STAGE_COMPLETE roots='+str(len(roots)))
elif phase=='rehome':
    assets=registry.get_assets_by_path('/Game',recursive=True)
    renames=[]
    for data in assets:
        old=str(data.package_name)
        if old.startswith('/Game/EclipseRecoil/Art/FactoryEnvironment/'):continue
        if str(data.asset_class_path.asset_name)=='ObjectRedirector':continue
        new='/Game/EclipseRecoil/Art/FactoryEnvironment/'+old.removeprefix('/Game/')
        renames.append(ue.AssetRenameData(data.get_asset(),new.rsplit('/',1)[0],new.rsplit('/',1)[1]))
    if renames and not tools.rename_assets(renames):raise RuntimeError('Cannot namespace migrated Factory packages')
    if not ue.EditorAssetLibrary.save_directory('/Game/EclipseRecoil/Art/FactoryEnvironment',only_if_is_dirty=False,recursive=True):raise RuntimeError('Cannot save namespaced packages')
    final_roots=['/Game/EclipseRecoil/Art/FactoryEnvironment/'+p.removeprefix('/Game/') for p in roots]
    # Renamed packages can precede the registry's dependency refresh. Migrate
    # the explicit staged closure, including textures/functions/materials.
    content=SCRATCH/'Content'
    closure=['/Game/'+str(p.relative_to(content).with_suffix('')) for p in (content/'EclipseRecoil/Art/FactoryEnvironment').rglob('*.uasset') if p.stem!='SM_Floor2m']
    tools.migrate_packages(closure,str(PROJECT/'Content'),options)
    # All generated packages must resolve under the private Art link.
    files=[p for p in (PROJECT/'Assets/maps/Content/EclipseRecoil/Art/FactoryEnvironment').rglob('*') if p.is_file()]
    if not files:raise RuntimeError('No private packages migrated')
    receipt={'phase':'complete','root_meshes':final_roots,'renamed_packages':len(renames),'private_files':len(files),'private_bytes':sum(p.stat().st_size for p in files)}
    (PRIVATE/'migration_receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    (SCRATCH/'migration_complete.json').write_text(json.dumps(receipt,indent=2)+'\n')
    ue.log('ER_FACTORY_MIGRATION_COMPLETE '+json.dumps(receipt))
else:raise RuntimeError('Unknown phase '+phase)
