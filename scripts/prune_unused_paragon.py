"""Review and optionally remove unreachable Paragon packages through Unreal.

Requires a fresh audit-assets.py report. Default is a dry run. Removal requires
-ERPruneParagon and a separate SHA-256-verified archive manifest. Packages with
any referencer outside the batch are retained, including their dependencies.
"""
import json
import hashlib
from pathlib import Path
import unreal as ue

project = Path(ue.Paths.project_dir()).resolve()
directory = project / 'Saved/AssetAudit'
audit = json.loads((directory / 'asset-audit.json').read_text())
roots = ('/Game/ParagonTwinblast/', '/Game/ParagonDrongo/')
records = {p['package']: p for p in audit['packages']
           if p['status'] == 'candidate_review' and p['package'].startswith(roots)
           and (project / p['path']).is_file()}
registry = ue.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
registry.wait_for_completion()
options = ue.AssetRegistryDependencyOptions(include_hard_package_references=True,
    include_soft_package_references=True, include_searchable_names=False,
    include_soft_management_references=True, include_hard_management_references=True)
batch = set(records)
refs = {p: {str(r) for r in registry.get_referencers(p, options)} for p in batch}
# Keep a complete dependency island: removing a blocked package from the batch
# also blocks anything it still refers to. Do not modify outside referencers.
while True:
    blocked = {p for p in batch if refs[p] - batch}
    if not blocked:
        break
    batch -= blocked
plan = {'packages': [records[p] for p in sorted(batch)],
        'retained_with_outside_referencers': sorted(set(records)-batch),
        'bytes': sum(records[p]['bytes'] for p in batch), 'deleted': False}
(directory / 'paragon-prune-plan.json').write_text(json.dumps(plan, indent=2)+'\n')
ue.log('ER_PARAGON_PRUNE_READY packages=%d bytes=%d retained=%d' % (len(batch),plan['bytes'],len(records)-len(batch)))
if '-ERPruneParagon' in ue.SystemLibrary.get_command_line().split():
    backup = json.loads((directory / 'paragon-backup-manifest.json').read_text())
    if not backup.get('verified') or not Path(backup['archive']).is_file():
        raise RuntimeError('Missing independent verified archive')
    saved = {p['path']: p for p in backup['files']}
    for p in batch:
        record = records[p]
        if record['path'] not in saved or saved[record['path']]['bytes'] != record['bytes']:
            raise RuntimeError('Backup does not cover: ' + p)
        with (project / record['path']).open('rb') as current:
            if hashlib.file_digest(current, 'sha256').hexdigest() != saved[record['path']]['sha256']:
                raise RuntimeError('Package changed since backup: ' + p)
    # Nothing outside this independently archived batch refers to it. Use
    # Unreal's scoped package-file deletion, without loading obsolete demo
    # Worlds/Blueprints. ForceDeleteObjects in UE 5.8 can crash cleaning their
    # loaded World objects, and loading them also compiles retired VR nodes.
    filenames = [str(project / records[p]['path']) for p in sorted(batch)]
    if not ue.ERCharacter.delete_unloaded_paragon_packages(sorted(batch)):
        raise RuntimeError('Unreal package deletion failed')
    if any(Path(path).exists() for path in filenames):
        raise RuntimeError('Unreal left files behind')
    registry.scan_modified_asset_files(filenames)
    plan['deleted'] = True
    (directory / 'paragon-prune-plan.json').write_text(json.dumps(plan,indent=2)+'\n')
    ue.log('ER_PARAGON_PRUNED packages=%d bytes=%d' % (len(batch),plan['bytes']))
