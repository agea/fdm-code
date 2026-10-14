"""Read-only Unreal Asset Registry audit. Run with -run=pythonscript.

Reports reachability, not proof that an unreferenced package is disposable.
No packages are loaded, modified, moved or deleted.
"""
import collections
import json
import re
from pathlib import Path
import unreal as ue

project = Path(ue.Paths.project_dir()).resolve()
registry = ue.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
registry.wait_for_completion()
options = ue.AssetRegistryDependencyOptions(
    include_hard_package_references=True, include_soft_package_references=True,
    include_searchable_names=False, include_soft_management_references=True,
    include_hard_management_references=True)
assets = registry.get_all_assets(True)
packages = {str(a.package_name) for a in assets if str(a.package_name).startswith('/Game/')}
files = {}
for path in (project / 'Content').rglob('*'):
    if path.is_file() and path.suffix in ('.uasset', '.umap'):
        package = '/Game/' + path.relative_to(project / 'Content').with_suffix('').as_posix()
        files[package] = {'path': path.relative_to(project).as_posix(), 'bytes': path.stat().st_size}
dependencies = {p: sorted(str(d) for d in registry.get_dependencies(p, options)) for p in packages}

def closure(roots):
    seen, pending = set(), list(roots)
    while pending:
        package = pending.pop()
        if package in seen or package not in packages:
            continue
        seen.add(package)
        pending.extend(dependencies[package])
    return seen

# All project maps, game Blueprint defaults/catalogs, and weapon data. Runtime
# string references in native code/config are additional explicit roots.
runtime_roots = {p for p in packages if p.startswith('/Game/EclipseRecoil/Maps/')
                 or p.startswith('/Game/EclipseRecoil/Core/')
                 or p.startswith('/Game/EclipseRecoil/Weapons/DA_')}
literal_roots = set()
for folder in ('Source', 'Config'):
    for path in (project / folder).rglob('*'):
        if path.is_file():
            for match in re.findall(r'/Game/[A-Za-z0-9_/]+', path.read_text(errors='replace')):
                if match in packages:
                    literal_roots.add(match)
runtime_roots |= literal_roots
runtime = closure(runtime_roots)
# Keep all owned assets plus literal references used by rebuilding/verifying
# scripts. These are authoring roots, distinct from runtime requirements.
authoring_roots = {p for p in packages if p.startswith('/Game/EclipseRecoil/')
                   and not p.startswith('/Game/EclipseRecoil/Characters/Mercenary/')}
# setup_team_skins.py builds these paths with tb/dr + suffix. Literal scanning
# alone misses them: retain original meshes to rebuild the current variants.
skin_sources = [
    '/Game/ParagonTwinblast/Characters/Heroes/TwinBlast/Skins/Tier2/ActionMovie/Meshes/TwinBlast_ActionHero',
    '/Game/ParagonTwinblast/Characters/Heroes/TwinBlast/Skins/Tier2/ShadowOps/Meshes/TwinBlast_ShadowOps',
    '/Game/ParagonTwinblast/Characters/Heroes/TwinBlast/Meshes/TwinBlast',
    '/Game/ParagonTwinblast/Characters/Heroes/TwinBlast/Skins/Tier1/CS_Twinblast_Apocalyptic/TwinBlast_Wasteland',
    '/Game/ParagonDrongo/Characters/Heroes/Drongo/Meshes/Drongo_GDC',
    '/Game/ParagonDrongo/Characters/Heroes/Drongo/Skins/Scavenger/Meshes/Drongo_Scavenger',
]
authoring_roots.update(p for p in skin_sources if p in packages)
for path in (project / 'scripts').glob('*.py'):
    if path.name in ('audit-assets.py', 'detach_mercenary.py'):
        continue
    for match in re.findall(r'/Game/[A-Za-z0-9_/]+', path.read_text(errors='replace')):
        if match in packages:
            authoring_roots.add(match)
authoring = closure(authoring_roots | runtime_roots)
summary = collections.defaultdict(lambda: collections.Counter())
records = []
for p, info in sorted(files.items()):
    protected = p.startswith(('/Game/__ExternalActors__/', '/Game/__ExternalObjects__/')) or p not in packages
    status = 'runtime_reachable' if p in runtime else 'authoring_reachable' if p in authoring else 'protected_review' if protected else 'candidate_review'
    group = p.split('/')[2]
    summary[group]['total_bytes'] += info['bytes']
    summary[group]['total_packages'] += 1
    summary[group][status + '_bytes'] += info['bytes']
    summary[group][status + '_packages'] += 1
    records.append(dict(package=p, status=status, **info))
report = {
    'schema': 1, 'unreal_version': ue.SystemLibrary.get_engine_version(),
    'method': 'Transitive hard/soft package and management references; editor dependencies can overestimate runtime.',
    'limitations': ['Runtime-generated paths and constructed Python paths may be absent.',
                   'candidate_review is NOT authorization to delete; original skin sources and rebuild inputs may be needed.',
                   'External actors, redirectors, plugin/engine dependencies and edited maps require separate review.',
                   'No cook or load test has been performed by this read-only audit.'],
    'runtime_roots': sorted(runtime_roots), 'authoring_roots': sorted(authoring_roots),
    'skin_source_roots': skin_sources,
    'summary': dict(sorted(summary.items())), 'packages': records,
    'runtime_direct_dependencies': {p: dependencies.get(p, []) for p in sorted(runtime_roots)},
    'reachable_dependencies': {p: dependencies[p] for p in sorted(authoring)},
    'mercenary_runtime_dependencies': {p: dependencies[p] for p in sorted(runtime) if 'Mercenary' in p},
    'registry_missing_files': sorted(set(files) - packages),
}
destination = project / 'Saved/AssetAudit/asset-audit.json'
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(json.dumps(report, indent=2) + '\n')
ue.log('ER_ASSET_AUDIT_COMPLETE packages={} runtime={} authoring={} report={}'.format(len(files), len(runtime), len(authoring), destination))
