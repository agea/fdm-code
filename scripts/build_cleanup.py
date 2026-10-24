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


def remove_uploaded_build(run, project):
    """Remove only the selected successful run, including its logs and receipts."""
    run, project = Path(run), Path(project).resolve()
    if run.is_symlink():
        raise ValueError('Refusing a symlinked build directory: ' + str(run))
    resolved = run.resolve()
    if resolved.is_relative_to(project) or project.is_relative_to(resolved):
        raise ValueError('Build cleanup must stay outside the source project')
    # Check the state at the selected path; never take deletion paths from JSON.
    receipt = next((run / name for name in ('release.json', 'summary.json', 'receipt.json')
                    if (run / name).is_file()), None)
    if receipt is None or receipt.is_symlink():
        raise ValueError('Missing build receipt: ' + str(run))
    data = json.loads(receipt.read_text(encoding='utf-8-sig'))
    if not isinstance(data, dict) or data.get('status') != 'uploaded':
        raise ValueError('Only successfully uploaded builds can be removed: ' + str(run))
    print('Removing uploaded build, logs and receipts: ' + str(run), flush=True)
    shutil.rmtree(run)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Remove a successfully uploaded build and all its artifacts.')
    parser.add_argument('--uploaded-run', type=Path, required=True)
    args = parser.parse_args()
    remove_uploaded_build(args.uploaded_run, Path(__file__).resolve().parents[1])
