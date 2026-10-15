#!/usr/bin/env python3
"""Reject binary assets or unapproved paths anywhere in public Git history."""
import subprocess
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
ROOT_FILES = {'fdm.uproject', 'README.md', 'AGENTS.md', '.gitignore', '.gitattributes',
              '.gitmodules', 'assets.json', 'ASSETS_REQUIRED.md', 'CODE_EXPORT.json'}
EXTENSIONS = {'Source': {'.cpp', '.h', '.cs'}, 'Config': {'.ini'},
              'scripts': {'.py', '.command', '.sh'}, 'docs': {'.md', '.txt', '.json', '.csv'}}
SUBMODULES = {'Assets/characters', 'Assets/weapons', 'Assets/maps'}


def verify(root):
    revisions = subprocess.check_output(['git', 'rev-list', '--all'], cwd=root, text=True).splitlines()
    failures = []
    count = 0
    for revision in revisions:
        tree = subprocess.check_output(['git', 'ls-tree', '-rz', revision], cwd=root).split(b'\0')
        for item in tree:
            if not item:
                continue
            info, raw_path = item.split(b'\t', 1)
            mode, kind, oid = info.decode().split()
            name = raw_path.decode()
            path = Path(name)
            if mode == '160000' and name in SUBMODULES:
                continue
            allowed = name in ROOT_FILES or (path.parts[0] in EXTENSIONS and path.suffix in EXTENSIONS[path.parts[0]])
            if kind != 'blob' or mode not in ('100644', '100755') or not allowed:
                failures.append(revision[:8] + ': forbidden path/mode ' + name)
                continue
            blob = subprocess.check_output(['git', 'cat-file', 'blob', oid], cwd=root)
            if b'\0' in blob or blob.startswith(b'version https://git-lfs.github.com/spec/v1'):
                failures.append(revision[:8] + ': binary/LFS blob ' + name)
            count += 1
    if not revisions:
        failures.append('No committed history to verify')
    if failures:
        raise RuntimeError('\n'.join(failures))
    return len(revisions), count


if __name__ == '__main__':
    try:
        commits, blobs = verify(ROOT)
        print(f'PUBLIC_TREE_PASS: {commits} commits, {blobs} text entries, asset gitlinks only')
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
