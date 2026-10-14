#!/usr/bin/env python3
"""Prepare an isolated runtime test project with both mercenary trees absent.

Hardlinks save disk; run only read-only verifiers or -game in this fixture.
Never run import/setup/save commands in it. Original assets are not removed.
"""
import os
from pathlib import Path
import shutil

project = Path(__file__).resolve().parents[1]
destination = project / 'BuildArtifacts/no-mercenary-test'
if destination.exists():
    raise RuntimeError('Fixture already exists; preserve/remove it explicitly before a new preparation')
destination.mkdir(parents=True)
(destination / 'Saved/Testing').mkdir(parents=True)
for folder in ('Config', 'Source', 'Binaries', 'scripts'):
    shutil.copytree(project / folder, destination / folder, ignore=shutil.ignore_patterns('__pycache__'))
shutil.copy2(project / 'fdm.uproject', destination / 'fdm.uproject')
count = 0
import runpy
content_files = runpy.run_path(str(project / 'scripts/asset_files.py'))['content_files']
for path in content_files(project):
    if not path.is_file() or path.is_symlink():
        continue
    relative = path.relative_to(project)
    if relative.parts[:2] == ('Content', 'Mercenary_Male_WaveforceStudio') or relative.parts[:4] == ('Content', 'EclipseRecoil', 'Characters', 'Mercenary'):
        continue
    target = destination / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    os.link(path, target)
    count += 1
assert not (destination / 'Content/Mercenary_Male_WaveforceStudio').exists()
assert not (destination / 'Content/EclipseRecoil/Characters/Mercenary').exists()
(destination / 'READ_ONLY_ASSETS.txt').write_text('Content is hardlinked. Run game/read-only verification only; never save/import/setup assets here.\n')
print('ER_NO_MERCENARY_FIXTURE_READY assets=%d project=%s' % (count, destination / 'fdm.uproject'))
