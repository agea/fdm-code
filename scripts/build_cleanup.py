"""Remove completed Mac build payloads before measuring free disk space."""
import json
from pathlib import Path
import shutil
import time


def cleanup_old_builds(roots, *, dry_run=False):
    """Keep receipts/logs, incomplete releases and builds awaiting upload.

    Only inspect immediate children of explicit build roots. Never follow a
    symlink or use a deletion path supplied by a receipt.
    """
    for root in dict.fromkeys(Path(p).resolve() for p in roots):
        if not root.is_dir():
            continue
        for run in sorted(root.iterdir()):
            if run.is_symlink() or not run.is_dir():
                continue
            release = run / 'release.json'
            receipt = release if release.exists() else run / 'receipt.json'
            if receipt.is_symlink() or not receipt.is_file():
                continue
            try:
                data = json.loads(receipt.read_text())
            except (OSError, ValueError):
                continue
            if not isinstance(data, dict):
                continue
            expected = 'uploaded' if receipt == release else 'packaged'
            if data.get('status') != expected:
                continue
            names = ('build/project', 'build/dist', 'scratch', 'diagnostics') if receipt == release else ('project', 'dist')
            targets = [run / name for name in names]
            # Xcode's retained notarization archive is also a build payload.
            archive_root = run / 'build' if receipt == release else run
            if not archive_root.is_symlink():
                targets.extend(archive_root.glob('RespawnAndRage*.xcarchive'))
            targets = [p for p in targets if p.is_dir() and
                       not p.is_symlink() and not p.parent.is_symlink() and
                       p.resolve().is_relative_to(run)]
            if not targets:
                continue
            for target in targets:
                print(('Would remove old build: ' if dry_run else 'Removing old build: ') + str(target), flush=True)
            if dry_run:
                continue
            # Mark before deleting: an interrupted cleanup cannot be resumed as a package.
            data['payload_removed_at'] = time.strftime('%Y-%m-%dT%H:%M:%S%z')
            if receipt != release:
                data['scratch_removed'] = True
            temporary = receipt.with_suffix('.json.tmp')
            temporary.write_text(json.dumps(data, indent=2) + '\n')
            temporary.replace(receipt)
            for target in targets:
                shutil.rmtree(target)


def build_roots(workspace, project):
    return (workspace, project.parent / 'builds/macos', Path('/private/tmp/rr-sm5-releases'))
