"""Plan/apply the user-approved runtime-only character cleanup through Unreal.

Dry run by default. -ERPruneCharacters requires an independently verified archive
and rejects changed packages or any referencer outside the complete batch.
"""
import hashlib
import json
from pathlib import Path
import unreal as ue

project = Path(ue.Paths.project_dir()).resolve()
directory = project/'Saved/AssetAudit'
report = json.loads((directory/'asset-audit.json').read_text())
roots = ('/Game/ParagonTwinblast/', '/Game/ParagonDrongo/', '/Game/ParagonPhase/',
         '/Game/ParagonLtBelica/', '/Game/EclipseRecoil/Characters/', '/Game/Characters/')
records = {r['package']: r for r in report['packages']
           if r['package'].startswith(roots)
           and (project/r['path']).is_file()
           and not r['package'].startswith('/Game/EclipseRecoil/Characters/FirstPerson/')
           and r['status'] not in ['runtime_reachable', 'protected_review']}
owner = (project/'Assets/characters/Content').resolve()
for record in records.values():
    assert (project/record['path']).resolve().is_relative_to(owner), record['path']
registry = ue.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
registry.wait_for_completion()
options = ue.AssetRegistryDependencyOptions(include_hard_package_references=True,
    include_soft_package_references=True, include_searchable_names=False,
    include_soft_management_references=True, include_hard_management_references=True)
batch = set(records)
referencers = {p: {str(r) for r in registry.get_referencers(p, options)} for p in batch}
while True:
    blocked = {p for p in batch if referencers[p]-batch}
    if not blocked:
        break
    batch -= blocked
plan = dict(schema=1, packages=[records[p] for p in sorted(batch)],
            bytes=sum(records[p]['bytes'] for p in batch), deleted=False,
            retained_with_outside_referencers={p: sorted(referencers[p]-batch) for p in sorted(set(records)-batch)},
            preserved_fps_authoring='/Game/EclipseRecoil/Characters/FirstPerson/')
(directory/'character-prune-plan.json').write_text(json.dumps(plan, indent=2)+'\n')
ue.log('ER_CHARACTER_PRUNE_READY packages=%d bytes=%d retained=%d' % (len(batch), plan['bytes'], len(records)-len(batch)))
if '-ERPruneCharacters' in ue.SystemLibrary.get_command_line().split():
    receipt_path = directory/'character-cleanup-backup.json'
    backup = json.loads(receipt_path.read_text())
    assert backup['verified'] and Path(backup['archive']).is_file()
    saved = {r['path']: r for r in backup['files']}
    for package in batch:
        record = records[package]
        original = saved[record['path']]
        file = project/record['path']
        assert file.stat().st_size == original['bytes'] == record['bytes'], package
        with file.open('rb') as handle:
            assert hashlib.file_digest(handle, 'sha256').hexdigest() == original['sha256'], package
        assert not file.with_suffix('.uexp').exists() and not file.with_suffix('.ubulk').exists(), 'Unsupported sidecar'
    # Legacy API name; native validation is explicitly scoped to the six
    # character namespaces above and rechecks all outside referencers.
    (directory/'paragon-backup-manifest.json').write_text(receipt_path.read_text())
    assert ue.ERCharacter.delete_unloaded_paragon_packages(sorted(batch)), 'Unreal rejected deletion'
    assert not any((project/records[p]['path']).exists() for p in batch)
    registry.scan_modified_asset_files([str(project/records[p]['path']) for p in batch])
    plan['deleted'] = True
    (directory/'character-prune-plan.json').write_text(json.dumps(plan, indent=2)+'\n')
    ue.log('ER_CHARACTER_PRUNED packages=%d bytes=%d' % (len(batch), plan['bytes']))
