#!/usr/bin/env python3
"""Export a fresh code-only tree for review, without assets or Git history.

Default: preview. --apply writes BuildArtifacts/code-only (must not exist).
Does not initialize Git, publish, upload or choose an open-source license.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

PROJECT = Path(__file__).resolve().parents[1]
SUFFIXES = {
    'Source': {'.cpp', '.h', '.cs'}, 'Config': {'.ini'},
    'scripts': {'.py', '.command', '.sh'}, 'docs': {'.md', '.txt', '.json'},
}
ROOT_FILES = ('fdm.uproject', 'README.md', 'AGENTS.md', '.gitignore', '.gitattributes', 'assets.json', '.gitmodules')

def source_files(root):
    files = [root / name for name in ROOT_FILES if (root / name).is_file() and not (root / name).is_symlink()]
    for folder, extensions in SUFFIXES.items():
        for path in (root / folder).rglob('*'):
            if (path.is_file() and path.suffix in extensions and not path.is_symlink()
                    and not any(p.is_symlink() for p in path.parents if p != root)):
                files.append(path)
    return sorted(files)

def export(root, destination):
    if destination.exists() or destination.is_symlink():
        raise RuntimeError('Destination must not already exist: ' + str(destination))
    files = source_files(root)
    destination.mkdir(parents=True)
    manifest = []
    for path in files:
        relative = path.relative_to(root)
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
        if relative.as_posix() == '.gitignore':
            with target.open('a') as ignore:
                ignore.write('\n# Restored licensed assets stay outside the public code history.\n/Content/\n/SourceAssets/\n')
        manifest.append({'path': relative.as_posix(), 'bytes': target.stat().st_size,
                         'sha256': hashlib.sha256(target.read_bytes()).hexdigest()})
    (destination / 'CODE_EXPORT.json').write_text(json.dumps({'files': manifest, 'assets_included': False, 'git_history_included': False}, indent=2) + '\n')
    (destination / 'ASSETS_REQUIRED.md').write_text(
        '# Assets required\n\nThis export contains code/configuration only. It cannot run until the\n'
        'matching private Content tree is restored beside fdm.uproject.\n'
        'SourceAssets are needed only for asset rebuilding. Preserve original\n'
        '/Game package paths; obtain licensed packs through their publishers.\n'
        'See docs/REPOSITORY_CLEANUP.md and docs/ASSET_LICENSES.md.\n'
        'No public redistribution rights for third-party assets are implied.\n'
        'Choose a license for your own code before publishing; retain Epic\n'
        'template notices and distinct third-party terms.\n')
    return len(files)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    paths = source_files(PROJECT)
    print(f'{len(paths)} text files, {sum(p.stat().st_size for p in paths) / 1048576:.2f} MiB')
    if args.apply:
        destination = PROJECT / 'BuildArtifacts/code-only'
        print('Exported', export(PROJECT, destination), 'files to', destination)
    else:
        print('Preview only. --apply creates a new BuildArtifacts/code-only tree; no publishing.')

if __name__ == '__main__':
    main()
