"""Audit Wraith dependencies and remove only backed-up staging Murdock packages.

Run in an isolated project using the game's editor module and staging Content.
The main game's packages and staging Gadget/Wraith are never deletion candidates.
"""
import hashlib,json
from pathlib import Path
import unreal as u
core=Path(__file__).resolve().parents[1]
stage=core.parent/'_imports/ParagonStaging'
out=core/'Saved/Testing/WraithReplacement';out.mkdir(parents=True,exist_ok=True)
backup=core.parent/'_backups/staging-murdock-20261030'
registry=u.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True);registry.wait_for_completion()
roots=['/Game/ParagonWraith/Characters/Heroes/Wraith/'+s for s in [
 'Meshes/Wraith','Meshes/Wraith_Extents','Skins/ODGreen/Meshes/Wraith_ODGreen','Skins/LunarOps/Meshes/Wraith_LunarOps']]
options=u.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=False,include_searchable_names=False,include_soft_management_references=False,include_hard_management_references=False)
seen=set();pending=list(roots)
while pending:
 p=pending.pop()
 if p in seen or not p.startswith('/Game/'):continue
 seen.add(p);pending.extend(str(x) for x in registry.get_dependencies(p,options))
assert all(p.startswith('/Game/ParagonWraith/') for p in seen),sorted(p for p in seen if not p.startswith('/Game/ParagonWraith/'))
files={p:'Content/'+p.removeprefix('/Game/')+'.uasset' for p in sorted(seen)}
assert all((stage/f).is_file() for f in files.values())
(out/'wraith-source-closure.json').write_text(json.dumps(files,indent=2))
batch=sorted({str(a.package_name) for a in registry.get_assets_by_path('/Game/ParagonMurdock',recursive=True)})
manifest=json.loads((backup/'backup_manifest.json').read_text())
for p in batch:
 rel='Content/'+p.removeprefix('/Game/')+'.uasset'
 if not (stage/rel).exists():rel=rel[:-7]+'.umap'
 assert hashlib.sha256((stage/rel).read_bytes()).hexdigest()==manifest[rel],rel
if batch:
 assert u.ERCharacter.delete_unloaded_paragon_packages(batch),'Murdock deletion blocked by an outside reference or loaded package'
assert not list((stage/'Content/ParagonMurdock').rglob('*.uasset'))
receipt=out/'staging-cleanup.json'
if batch or not receipt.exists():
 receipt.write_text(json.dumps({'removed_murdock_packages':len(batch),'wraith_import_packages':len(files),'backup':str(backup),'gadget_retained':True},indent=2))
u.log('ER_WRAITH_STAGING_READY murdock_removed=%d wraith_packages=%d gadget_retained=1'%(len(batch),len(files)))
