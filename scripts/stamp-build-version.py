#!/usr/bin/env python3
"""Stamp a build's commit calendar date and immutable source commit for runtime UI."""
from datetime import datetime
from pathlib import Path
import re
import subprocess


def source_commit(project):
    # Isolated packaging snapshots intentionally have no Git database.
    if (project / '.git').exists():
        commit = subprocess.check_output(
            ['git', '-C', str(project), 'rev-parse', 'HEAD'], text=True).strip()
    else:
        commit = (project / 'Build/SourceCommit.txt').read_text().strip()
    if not re.fullmatch(r'[0-9a-f]{40}|[0-9a-f]{64}', commit):
        raise ValueError('A complete source commit hash is required for the build version')
    return commit


def ensure_clean(project):
    status = subprocess.check_output(
        ['git', '-C', str(project), 'status', '--porcelain',
         '--untracked-files=all', '--ignore-submodules=none'], text=True)
    if status.strip():
        raise ValueError('Working tree is dirty; commit or stash changes (including untracked files and submodules) before releasing')


def source_date(project):
    if (project / '.git').exists():
        value = subprocess.check_output(
            ['git', '-C', str(project), 'show', '-s', '--format=%cI', 'HEAD'], text=True).strip()
    else:
        value = (project / 'Build/SourceCommitDate.txt').read_text().strip()
    # Keep the calendar date in the commit's recorded timezone on every host.
    return datetime.fromisoformat(value)


def stamp(project):
    version = source_date(project).strftime('%y.%m.%d.') + source_commit(project)[:8]
    path = project / 'Build/GameVersion.txt'
    path.parent.mkdir(parents=True, exist_ok=True)
    contents = version + '\n'
    if not path.exists() or path.read_text() != contents:
        path.write_text(contents, encoding='utf-8')
    print('Game version: ' + version, flush=True)
    return version


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check-clean', action='store_true')
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    if args.check_clean:
        ensure_clean(project)
    else:
        stamp(project)
