"""Rehome the staged Epic Content Examples dependency closure through Unreal.

Read/stage in an isolated project mounted to the downloaded Content folder;
never save or rename packages in the original ContentExamples project.
"""
import hashlib,json,re
from pathlib import Path
import unreal as ue

project=Path(__file__).resolve().parents[1]
root='/Game/EclipseRecoil/Effects/Grenade/Epic'
tools=ue.AssetToolsHelpers.get_asset_tools()
registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
phase=re.search(r'EREpicPhase=(stage|rehome)',ue.SystemLibrary.get_command_line())
phase=phase.group(1) if phase else 'rehome'
if phase=='stage':
    stage=project/'Saved/EpicGrenadeMigration'
    (stage/'Content').mkdir(parents=True,exist_ok=True)
    (stage/'EpicGrenadeMigration.uproject').write_text(json.dumps({'FileVersion':3,'EngineAssociation':'5.8','Plugins':[{'Name':name,'Enabled':True} for name in ['PythonScriptPlugin','EditorScriptingUtilities']]}))
    roots=['/Game/ExampleContent/Blueprint_Communication/Effects/P_Explosion_Bomb','/Game/ExampleContent/Effects/Textures/T_Smoke_Tiled_D']
    for name in roots:
        if not ue.load_asset(name):raise RuntimeError('Missing downloaded Epic package '+name)
    tools.migrate_packages(roots,str(stage/'Content'),ue.MigrationOptions(prompt=False,ignore_dependencies=False,asset_conflict=ue.AssetMigrationConflict.SKIP))
    ue.log('ER_EPIC_STAGE_COMPLETE')
else:
    expected=(project/'Saved/EpicGrenadeMigration').resolve()
    if Path(ue.Paths.project_dir()).resolve()!=expected:
        raise RuntimeError('Rehome is allowed only in the isolated '+str(expected)+' project; use -EREpicPhase=stage to read the source')
    renames=[]
    for data in registry.get_assets_by_path('/Game/ExampleContent',recursive=True):
        if str(data.asset_class_path.asset_name)=='ObjectRedirector':continue
        destination=root+'/'+str(data.package_name).removeprefix('/Game/ExampleContent/')
        renames.append(ue.AssetRenameData(data.get_asset(),*destination.rsplit('/',1)))
    if renames and not tools.rename_assets(renames):raise RuntimeError('Cannot rehome Epic effects')
    if not ue.EditorAssetLibrary.save_directory(root,only_if_is_dirty=False,recursive=True):raise RuntimeError('Cannot save Epic closure')
    # Use the explicit saved closure because the registry may still contain old paths.
    paths=['/Game/'+str(p.relative_to(Path(ue.Paths.project_content_dir())).with_suffix('')) for p in (Path(ue.Paths.project_content_dir())/'EclipseRecoil/Effects/Grenade/Epic').rglob('*.uasset')]
    tools.migrate_packages(paths,str(project/'Content'),ue.MigrationOptions(prompt=False,ignore_dependencies=False,asset_conflict=ue.AssetMigrationConflict.SKIP))
    receipt={'source':'Epic Content Examples, UE 5.8, downloaded by user','source_project':'/Users/agea/Documents/Unreal Projects/ContentExamples','source_unchanged':True,'packages':paths}
    source_content=Path(receipt['source_project'])/'Content/ExampleContent'
    receipt['source_sha256']={str(p.relative_to(source_content)):hashlib.sha256(p.read_bytes()).hexdigest() for name in paths if (p:=source_content/(name.removeprefix(root+'/')+'.uasset')).is_file()}
    out=project/'Assets/weapons/SourceAssets/EpicContentExamples';out.mkdir(parents=True,exist_ok=True)
    (out/'migration.json').write_text(json.dumps(receipt,indent=2)+'\n')
    ue.log('ER_EPIC_MIGRATION_COMPLETE packages='+str(len(paths)))
