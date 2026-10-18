#!/usr/bin/env python3
"""Stamp a build's local calendar date and immutable source commit for runtime UI."""
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


def stamp(project):
    version = datetime.now().strftime('%y.%m.%d.') + source_commit(project)[:8]
    path = project / 'Build/GameVersion.txt'
    path.parent.mkdir(parents=True, exist_ok=True)
    contents = version + '\n'
    if not path.exists() or path.read_text() != contents:
        path.write_text(contents, encoding='utf-8')
    print('Game version: ' + version, flush=True)
    return version


if __name__ == '__main__':
    stamp(Path(__file__).resolve().parents[1])
