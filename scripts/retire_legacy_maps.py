"""Archive and remove the user-retired maps/Arabian art through Unreal only."""
import hashlib,json,tarfile
from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir());assets=ue.EditorAssetLibrary
if not assets.does_asset_exist('/Game/EclipseRecoil/Maps/L_CentralBuilding'):
    raise RuntimeError('Create and validate the replacement level first')
registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
retired={'/Game/EclipseRecoil/Maps/'+x for x in ['L_MovementLab','L_DestructionLab','L_ProceduralLab','L_Killhouse','L_DustMirror']}
retired|={'/Game/EclipseRecoil/Art/MapTemplates/'+x for x in ['DA_Killhouse','DA_DustMirror']}
for prefix in ['/Game/EclipseRecoil/Art/ArabianEnvironment','/Game/EclipseRecoil/Art/MapTemplates/DustTown']:
    retired|={str(d.package_name) for d in registry.get_assets_by_path(prefix,recursive=True)}
retired={x for x in retired if assets.does_asset_exist(x)}
backup=root/'Saved/MapRefactorBackup/2026-10-19';backup.mkdir(parents=True,exist_ok=True)
receipt_path=backup/'receipt.json'
if not retired:
    print('ER_RETIRE_ALREADY_COMPLETE');raise SystemExit(0)
options=ue.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True,include_searchable_names=True)
outside=[]
for package in sorted(retired):
    for ref in registry.get_referencers(package,options):
        ref=str(ref)
        if ref.startswith('/Game/') and ref not in retired:outside.append(package+' <- '+ref)
if outside:raise RuntimeError('Outside references prevent retirement:\n'+'\n'.join(outside))
files=[]
for package in retired:
    base=root/'Content'/package.removeprefix('/Game/')
    for ext in ['.umap','.uasset','.ubulk','.uexp']:
        f=base.with_suffix(ext)
        if f.is_file():files.append(f)
# Include the obsolete Arabian sources in the recoverable archive, leaving
# the original downloaded FactoryEnvironmentCollect project untouched.
arabian=root/'Assets/maps/SourceAssets/ArabianEnvironment'
source_files=[f for f in arabian.rglob('*') if f.is_file()] if arabian.exists() else []
legacy_sources=root/'Assets/maps/SourceAssets/MapTemplates'
source_files += [f for f in legacy_sources.rglob('*') if f.is_file() and f.name!='industrial_reference.json']
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
preserved=[root/'Assets/maps/Content/EclipseRecoil/Maps/L_Depot.umap',root/'Assets/maps/Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset']
before={str(f.relative_to(root)):sha(f) for f in preserved}
archive=backup/'retired-maps-and-arabian.tar'
manifest={str(f.relative_to(root)):sha(f) for f in files+source_files}
if not archive.exists():
    with tarfile.open(archive,'w') as out:
        for f in files+source_files:out.add(f,arcname=str(f.relative_to(root)),recursive=False)
with tarfile.open(archive) as inp:
    for name,digest in manifest.items():
        if hashlib.sha256(inp.extractfile(name).read()).hexdigest()!=digest:raise RuntimeError('Backup verification failed '+name)
    manifest={m.name:hashlib.sha256(inp.extractfile(m).read()).hexdigest() for m in inp.getmembers() if m.isfile()}
archived_packages=sorted('/Game/'+name.removeprefix('Content/').rsplit('.',1)[0] for name in manifest if name.startswith('Content/') and name.endswith(('.uasset','.umap')))
receipt={'archive':str(archive),'files':manifest,'retired_packages':archived_packages,'preserved_sha256':before}
receipt_path.write_text(json.dumps(receipt,indent=2)+'\n')
if '-ERRetireLegacyMaps' not in ue.SystemLibrary.get_command_line():
    print('ER_RETIRE_PREVIEW packages='+str(len(retired))+' backup='+str(archive))
else:
    lab='/Game/EclipseRecoil/Maps/L_ProceduralLab'
    if lab in retired:
        if not ue.ERMapTemplate.delete_archived_procedural_lab():raise RuntimeError('Cannot remove archived unloaded lab package')
        retired.remove(lab)
    # Unreal removes maps/redirectors/material dependency cycles as one explicit
    # set, only after the outside-reference and verified archive checks above.
    objects=[assets.load_asset(x) for x in sorted(retired)]
    if any(x is None for x in objects):raise RuntimeError('Cannot load retirement set')
    if objects and not assets.delete_loaded_assets(objects):raise RuntimeError('Unreal did not delete the complete retirement set')
    objects.clear();ue.SystemLibrary.collect_garbage()
    remaining=[x for x in retired if assets.does_asset_exist(x)]
    for package in remaining:
        print('ER_RETIRE_RETRY',package,'class=',registry.get_asset_by_object_path(package+'.'+package.rsplit('/',1)[-1]).asset_class_path)
        if not assets.delete_asset(package):raise RuntimeError('Cannot delete retired package '+package)
    ue.SystemLibrary.collect_garbage()
    remaining=[x for x in retired if assets.does_asset_exist(x)]
    if remaining:raise RuntimeError('Retirement incomplete '+str(remaining))
    for f in source_files:f.unlink()
    (legacy_sources/'README.md').write_text('# Map archetypes\n\nThe active industrial recipe is authored in DA_CentralBuilding through Unreal.\nSee core/docs/MAP_ARCHETYPES.md and scripts/setup_central_building.py.\nLegacy Killhouse/Dust sources were archived with their retired maps on 19 October 2026.\n')
    if {str(f.relative_to(root)):sha(f) for f in preserved}!=before:raise RuntimeError('Depot or material catalog changed')
    print('ER_RETIRE_COMPLETE packages='+str(len(archived_packages))+' verified_backup='+str(archive))
