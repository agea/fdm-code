"""Plan and optionally delete Phase/Murdock packages no longer reachable from the game.

Default is a dry run writing Saved/AssetAudit/phase-murdock-prune-plan.json.
-ERDeletePhaseMurdock deletes the batch, which must already be copied to
../_backups/phase-murdock-20261030 with matching SHA-256 (backup_manifest.json).
Packages with any referencer outside the batch are retained with their dependencies.
"""
import hashlib, json, re
from pathlib import Path
import unreal as ue

project = Path(ue.Paths.project_dir()).resolve()
registry = ue.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True); registry.wait_for_completion()
names = re.compile('phase|murdock', re.I)
batch = set()
for root in ('/Game/ParagonPhase', '/Game/ParagonMurdock', '/Game/EclipseRecoil/Characters'):
    for data in registry.get_assets_by_path(root, recursive=True):
        package = str(data.package_name)
        if root != '/Game/EclipseRecoil/Characters' or names.search(package): batch.add(package)
options = ue.AssetRegistryDependencyOptions(include_hard_package_references=True,
    include_soft_package_references=True, include_searchable_names=False,
    include_soft_management_references=True, include_hard_management_references=True)
refs = {p: {str(r) for r in registry.get_referencers(p, options)} for p in batch}
candidates = set(batch)
while True:
    blocked = {p for p in batch if refs[p] - batch}
    if not blocked: break
    batch -= blocked
def filename(p):
    return ue.PackageTools.package_name_to_filename(p) if hasattr(ue, 'PackageTools') else None
files = {}
for p in sorted(batch):
    rel = 'Content/' + p[len('/Game/'):] + '.uasset'
    if not (project / rel).exists(): rel = rel[:-7] + '.umap'
    files[p] = rel
retained = sorted(candidates - batch)
plan = dict(packages=files, retained=retained,
            outside_referencers={p: sorted(refs[p] - candidates) for p in retained if refs[p] - candidates},
            bytes=sum((project / f).stat().st_size for f in files.values()))
out = project / 'Saved/AssetAudit/phase-murdock-prune-plan.json'
out.parent.mkdir(parents=True, exist_ok=True); out.write_text(json.dumps(plan, indent=2) + '\n')
ue.log('ER_PHASE_MURDOCK_PLAN packages=%d retained=%d bytes=%d' % (len(files), len(retained), plan['bytes']))
if '-ERDeletePhaseMurdock' in ue.SystemLibrary.get_command_line().split():
    backup = project.parent / '_backups/phase-murdock-20261030'
    manifest = json.loads((backup / 'backup_manifest.json').read_text())
    for p, rel in files.items():
        with (project / rel).open('rb') as current:
            if manifest.get(rel) != hashlib.file_digest(current, 'sha256').hexdigest():
                raise RuntimeError('Backup does not cover: ' + rel)
    if not ue.ERCharacter.delete_unloaded_paragon_packages(sorted(files)):
        raise RuntimeError('Unreal package deletion failed')
    left = [rel for rel in files.values() if (project / rel).exists()]
    if left: raise RuntimeError('Unreal left files behind: %s' % left[:5])
    registry.scan_modified_asset_files([str(project / rel) for rel in files.values()])
    ue.log('ER_PHASE_MURDOCK_DELETED packages=%d' % len(files))
