"""Enumerate logical Content paths in ordinary and mounted checkouts."""
import json


def content_files(project):
    manifest = project / 'assets.json'
    if manifest.is_file() and (project / 'Assets').is_dir():
        data = json.loads(manifest.read_text())
        roots = [project / mount for repo in data['repositories'].values()
                 for mount in repo['mounts'] if mount.startswith('Content/')]
    else:
        roots = [project / 'Content']
    for root in roots:
        # Starting traversal at each declared mount follows that directory link,
        # but does not follow arbitrary nested symlinks or walk .git/SourceAssets.
        for path in root.rglob('*'):
            if path.is_file():
                yield path
