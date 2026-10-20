"""Remove only the seven texture sets approved on 20 October 2026.

Unreal commandlet; dry run unless -ERApplyUnusedTextureCleanup is supplied.
Checks all package referencers, verifies a private rollback archive, deletes
through Unreal, then prunes matching sources/manifests/menu credits. No Git/LFS
history, Downloads, other material libraries or release builds are touched.
"""
from pathlib import Path
import csv
import hashlib
import json
import re
import runpy
import shutil
import tarfile
import unreal as ue

project=Path(ue.Paths.project_dir()).resolve()
maps=project/'Assets/maps'
sf=maps/'SourceAssets/SketchfabTextures'
legacy=maps/'SourceAssets/ProceduralTextures'
selection=runpy.run_path(str(project/'scripts/sketchfab_texture_selection.py'))
removed_sf={
    'fb32b41a61f447a8bb4e6218d7920a02':'floor_material',
    'ebe968851be0443e9392ad87cbf64eab':'concrete_material',
    '72d94fe5b67142ee80fff34f4aeb4054':'corroded_metal',
    '7bd8520796e34e0bb0c6c4c22c3f1d3d':'floor_tiled_ii',
}
# These two collection listings were inspected but never imported/downloaded.
unused_metadata={'c6043a0abd59403dabb792eab04031b0','ce2fce219f634f328abc6280b2409613'}
removed_legacy={'wood_floor':'08_Pavimento_Legno','floor_tiles_02':'07_Pavimento_Piastrelle','OfficeCeiling001':'10_Controsoffitto'}
namespaces={
    '/Game/EclipseRecoil/Art/Industrial/SketchfabTextures':('M_ER_SF_',set(removed_sf.values())),
    '/Game/EclipseRecoil/Art/ProceduralTextures':('M_ER_PBR_',set(removed_legacy)),
}
apply='-ERApplyUnusedTextureCleanup' in ue.SystemLibrary.get_command_line()
assets=ue.EditorAssetLibrary
registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
options=ue.AssetRegistryDependencyOptions(True,True,True,True,True)
candidates={}
for namespace,(prefix,ids) in namespaces.items():
    for item in registry.get_assets_by_path(namespace,recursive=True):
        package=str(item.package_name)
        if package.rsplit('/',1)[-1] in {prefix+key for key in ids} or package.rsplit('/',2)[-2] in ids:
            if str(item.asset_class_path.asset_name) not in {'Material','Texture2D'}:
                raise RuntimeError('Unexpected candidate class '+package)
            candidates[package]=str(item.asset_class_path.asset_name)
references={package:sorted(str(ref) for ref in registry.get_referencers(package,options) if str(ref)!=package) for package in candidates}
outside={package:[ref for ref in refs if ref not in candidates] for package,refs in references.items()}
if any(outside.values()):raise RuntimeError('Outside references prevent deletion: '+json.dumps(outside))
sources=[sf/uid for uid in set(removed_sf)|unused_metadata]+[legacy/folder for folder in removed_legacy.values()]
for folder in sources:
    if folder.is_symlink() or folder.parent not in {sf,legacy}:raise RuntimeError('Unsafe source path '+str(folder))
package_files=[maps/'Content'/Path(package.removeprefix('/Game/')+'.uasset') for package in candidates]
if any(not path.is_file() for path in package_files):raise RuntimeError('Candidate package file is absent')
source_files=[path for folder in sources if folder.exists() for path in folder.rglob('*') if path.is_file()]
changed_metadata=[sf/name for name in ['collection-manifest.json','download-manifest.json','materials-manifest.json','import-receipt.json','CREDITS.txt']]
changed_metadata += [legacy/name for name in ['manifest.json','SHA256SUMS.txt','FONTI.csv','README.md','IMPORT_VERIFICATION.txt']]
changed_metadata += [project/'Branding/Credits.txt']
removed_files=set(package_files+source_files)
protected_files={path for root in [maps/'Content',sf,legacy] for path in root.rglob('*') if path.is_file()}-removed_files-set(changed_metadata)
def digest(path):
    with path.open('rb') as handle:return hashlib.file_digest(handle,'sha256').hexdigest()
protected={str(path.relative_to(project)):digest(path) for path in sorted(protected_files)}
report=dict(date='2026-10-20',mode='apply' if apply else 'dry_run',removed_sets=sorted(set(removed_sf.values())|set(removed_legacy)),
            packages=candidates,referencers=references,removed_source_files=len(source_files),
            removed_package_bytes=sum(p.stat().st_size for p in package_files),
            removed_source_bytes=sum(p.stat().st_size for p in source_files),protected_sha256=protected)
out=project/'Saved/AssetAudit/unused-map-textures-cleanup.json';out.parent.mkdir(parents=True,exist_ok=True)
if not apply:
    out.with_name('unused-map-textures-dry-run.json').write_text(json.dumps(report,indent=2)+'\n')
    print('ER_UNUSED_TEXTURES_DRY_RUN '+json.dumps({key:value for key,value in report.items() if key not in {'protected_sha256','referencers','packages'}}))
else:
    if len(candidates)!=37:raise RuntimeError('Expected the audited 37 packages, found '+str(len(candidates)))
    archive_dir=project.parent/'_backups';archive_dir.mkdir(exist_ok=True)
    archive=archive_dir/'unused-map-textures-20261020.tar.gz'
    if archive.exists():raise RuntimeError('Rollback archive already exists; inspect the prior receipt before retrying')
    archived=sorted(removed_files|{p for p in changed_metadata if p.is_file()})
    original={str(path.relative_to(project)):digest(path) for path in archived}
    with tarfile.open(archive,'w:gz',compresslevel=1) as package:
        for path in archived:package.add(path,arcname=str(path.relative_to(project)),recursive=False)
    with tarfile.open(archive,'r:gz') as package:
        if len(package.getmembers())!=len(original):raise RuntimeError('Rollback archive member count mismatch')
        for member in package.getmembers():
            if not member.isfile() or hashlib.file_digest(package.extractfile(member),'sha256').hexdigest()!=original[member.name]:
                raise RuntimeError('Rollback archive verification failed '+member.name)
    report['rollback_archive']=str(archive);report['rollback_sha256']=digest(archive);report['archive_members_sha256']=original
    report['result']='BACKUP_VERIFIED';out.write_text(json.dumps(report,indent=2)+'\n')
    for package in sorted(candidates,key=lambda p:(candidates[p]!='Material',p)):
        if not assets.delete_asset(package):raise RuntimeError('Unreal could not delete '+package)
    for path in package_files:
        if path.exists():raise RuntimeError('Deleted package still exists '+str(path))
    for folder in sources:
        if folder.exists():shutil.rmtree(folder)
    for namespace in namespaces:
        directory=maps/'Content'/namespace.removeprefix('/Game/')
        for folder in sorted((p for p in directory.rglob('*') if p.is_dir()),key=lambda p:len(p.parts),reverse=True):
            if not any(folder.iterdir()):folder.rmdir()

    def save_json(path,value):path.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n')
    collection=json.loads((sf/'collection-manifest.json').read_text())
    collection['assets']=[entry for entry in collection['assets'] if entry['uid'] in selection['SOURCES']]
    save_json(sf/'collection-manifest.json',collection)
    save_json(sf/'download-manifest.json',[entry for entry in json.loads((sf/'download-manifest.json').read_text()) if entry['uid'] in selection['SOURCES']])
    manifest=[entry for entry in json.loads((sf/'materials-manifest.json').read_text()) if entry['id'] in selection['MATERIAL_IDS']]
    save_json(sf/'materials-manifest.json',manifest)
    receipt=json.loads((sf/'import-receipt.json').read_text())
    receipt['materials']={key:value for key,value in receipt['materials'].items() if key in selection['MATERIAL_IDS']}
    save_json(sf/'import-receipt.json',receipt)
    (sf/'CREDITS.txt').write_text('\n\n'.join((sf/entry['uid']/'CREDITS.txt').read_text() for entry in collection['assets']))
    old=json.loads((legacy/'manifest.json').read_text())
    old['assets']=[entry for entry in old['assets'] if entry['id'] not in removed_legacy]
    old['asset_count']=len(old['assets']);old['pruned_unused_sets_date']='2026-10-20'
    save_json(legacy/'manifest.json',old)
    (legacy/'SHA256SUMS.txt').write_text(''.join(entry['sha256']+'  '+entry['file']+'\n' for asset in old['assets'] for entry in asset['files']))
    with (legacy/'FONTI.csv').open(newline='') as handle:
        reader=csv.DictReader(handle);fields=reader.fieldnames;rows=list(reader)
    with (legacy/'FONTI.csv').open('w',newline='') as handle:
        writer=csv.DictWriter(handle,fieldnames=fields);writer.writeheader();writer.writerows(row for row in rows if row['cartella'] not in removed_legacy.values())
    text=(legacy/'README.md').read_text()
    text='\n'.join(line for line in text.splitlines() if not any('| '+folder+' |' in line for folder in removed_legacy.values()))+'\n'
    text=text.replace('10 set di texture','7 set di texture').replace('esattamente i dieci set selezionati','i sette set ancora referenziati dal progetto')
    (legacy/'README.md').write_text(text)
    (legacy/'IMPORT_VERIFICATION.txt').write_text('Current selection after unused-texture cleanup, 20 October 2026.\nAssets: '+str(len(old['assets']))+'\nManifest files: '+str(sum(len(entry['files']) for entry in old['assets']))+'\nInitial acquisition/archive verification retained in the private cleanup rollback archive.\n')
    credits=project/'Branding/Credits.txt';text=credits.read_text()
    text=text.replace('Red Brick 03, Concrete Wall 006, Wood Planks, Stone Wall 02, Rusty Metal 04, Floor Tiles 02, Wood Floor and Asphalt 02.','Red Brick 03, Concrete Wall 006, Wood Planks, Stone Wall 02, Rusty Metal 04 and Asphalt 02.')
    text=text.replace('Plaster 001 and Office Ceiling 001.','Plaster 001.')
    heading='TEXTURES — SKETCHFAB COLLECTION'
    lines=[heading];seen=set()
    for entry in manifest:
        if entry['source_uid'] in seen:continue
        seen.add(entry['source_uid']);lines += [entry['name']+' — '+entry['author']+'.',entry['source'],entry['author_url'],entry['license']['url']]
    lines.append('Creative Commons Attribution 4.0. Changes: texture-only Unreal PBR adaptation, world projection, ground blending/grass tint and runtime resolution up to 2K. Original maps and credits retained privately.')
    text=re.sub(re.escape(heading)+r'\n.*?(?=\n\n|\Z)','\n'.join(lines),text,flags=re.S)
    credits.write_text(text)
    for relative,sha in protected.items():
        if not (project/relative).is_file() or digest(project/relative)!=sha:raise RuntimeError('Retained file changed '+relative)
    if any(assets.does_asset_exist(package) for package in candidates):raise RuntimeError('Deleted package remains registered')
    report.update(result='PASS',retained_sketchfab_materials=len(manifest),retained_sketchfab_sources=len(seen),retained_legacy_sets=len(old['assets']),retained_files_verified=len(protected))
    out.write_text(json.dumps(report,indent=2)+'\n')
    print('ER_UNUSED_TEXTURES_CLEANUP_PASS '+json.dumps({key:value for key,value in report.items() if key not in {'protected_sha256','archive_members_sha256','referencers','packages'}}))
