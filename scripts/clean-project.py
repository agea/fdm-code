#!/usr/bin/env python3
"""Clean explicitly selected generated files. Default: dry run, logs only.

Never touches Content, SourceAssets, .git, saved settings or autosaves.
Run --help for profiles. Close Unreal and compilers before --apply.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import time
import re
import csv

PROJECT = Path(__file__).resolve().parents[1]
DIRECTORIES = {
    'logs': ('Saved/Logs', 'Saved/Testing'),
    'screenshots': ('Saved/Screenshots', 'Saved/Testing'),
    'cache': ('DerivedDataCache', 'Intermediate/CachedAssetRegistry', 'Saved/ShaderDebugInfo'),
    'build': ('Intermediate', 'Binaries'),
}

def generated_files(root, profiles, older_than=0):
    selected = set()
    cutoff = time.time() - older_than * 86400
    for profile in profiles:
        for relative in DIRECTORIES[profile]:
            directory = root / relative
            if directory.is_symlink() or not directory.is_dir():
                continue
            for current, dirs, files in os.walk(directory, followlinks=False):
                dirs[:] = [d for d in dirs if not (Path(current) / d).is_symlink()]
                for filename in files:
                    path = Path(current) / filename
                    if path.is_symlink() or not path.is_file():
                        continue
                    if profile == 'logs' and path.suffix.lower() != '.log':
                        continue
                    if profile == 'screenshots' and path.suffix.lower() not in ('.png', '.jpg', '.jpeg', '.bmp', '.exr'):
                        continue
                    if path.stat().st_mtime <= cutoff:
                        selected.add(path)
    if 'logs' in profiles:
        selected.update(p for p in root.glob('*.log') if p.is_file() and not p.is_symlink() and p.stat().st_mtime <= cutoff)
    return sorted(selected)

def tracked_paths(root):
    result = subprocess.run(['git', '-C', str(root), 'ls-files', '-z'], check=True, capture_output=True)
    return {root / os.fsdecode(p) for p in result.stdout.split(b'\0') if p}

def ensure_idle():
    if os.name == 'nt':
        result = subprocess.run(['tasklist', '/FO', 'CSV'], check=True, capture_output=True, text=True)
    else:
        result = subprocess.run(['ps', '-axo', 'comm='], check=True, capture_output=True, text=True)
    names = ([row[0].removesuffix('.exe') for row in csv.reader(result.stdout.splitlines()) if row]
             if os.name == 'nt' else [Path(line.strip()).name.removesuffix('.exe') for line in result.stdout.splitlines()])
    active = any(re.fullmatch(r'UnrealEditor(?:-Mac-\w+)?|UnrealBuildTool|ShaderCompileWorker', name) for name in names)
    # UBT runs under dotnet on Mac/Linux. UnrealEditorServices is a Finder
    # integration helper, not an open Editor/game or compiler.
    if os.name != 'nt' and 'dotnet' in names:
        args = subprocess.run(['ps', '-axo', 'args='], check=True, capture_output=True, text=True)
        active |= 'UnrealBuildTool.dll' in args.stdout
    if active:
        raise RuntimeError('Close Unreal Editor/game and Unreal build/shader processes before --apply.')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', nargs='+', choices=tuple(DIRECTORIES), default=['logs'],
                        help='logs (default), screenshots, cache (DDC), build (Intermediate/Binaries; requires recompilation)')
    parser.add_argument('--older-than', type=float, default=0, metavar='DAYS')
    parser.add_argument('--apply', action='store_true', help='Delete only listed generated files; otherwise preview.')
    args = parser.parse_args()
    if args.older_than < 0:
        parser.error('--older-than must be non-negative')
    if not (PROJECT / 'fdm.uproject').is_file():
        raise RuntimeError('Expected fdm.uproject beside the scripts directory.')
    paths = generated_files(PROJECT, args.profile, args.older_than)
    tracked = tracked_paths(PROJECT)
    conflicts = [p for p in paths if p in tracked]
    if conflicts:
        raise RuntimeError('Refusing to touch tracked/indexed files: ' + ', '.join(str(p.relative_to(PROJECT)) for p in conflicts))
    total = sum(p.stat().st_size for p in paths)
    print(f'{"APPLY" if args.apply else "DRY RUN"}: {len(paths)} files, {total / 1048576:.2f} MiB; profiles={",".join(args.profile)}')
    for path in paths:
        print(f'{path.stat().st_size:>12}  {path.relative_to(PROJECT)}')
    if args.apply:
        ensure_idle()
        for path in paths:
            # Recheck containment and symlinks immediately before removal.
            path.resolve().relative_to(PROJECT)
            if path.is_symlink() or any(p.is_symlink() for p in path.parents if p != PROJECT):
                raise RuntimeError('Path became a symlink: ' + str(path))
            path.unlink()
        print('Removed generated files. Settings, autosaves, assets and Git/LFS untouched.')
    else:
        print('Nothing removed. Repeat with --apply after closing Unreal to execute.')

if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, subprocess.SubprocessError, ValueError) as error:
        print('Cleanup refused:', error, file=sys.stderr)
        sys.exit(1)
