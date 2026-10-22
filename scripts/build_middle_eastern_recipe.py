#!/usr/bin/env python3
"""Build the new alley recipe from the archived, project-owned street graph."""
import hashlib
import json
import shutil
import tarfile
from pathlib import Path

project = Path(__file__).resolve().parents[1]
out = project / 'Assets/maps/SourceAssets/MiddleEasternAlleys'
out.mkdir(parents=True, exist_ok=True)
archive = project / 'Saved/MapRefactorBackup/2026-10-19/retired-maps-and-arabian.tar'
prefix = 'Assets/maps/SourceAssets/MapTemplates/DustTown/'
base = out / 'street-graph-source.json'
if not base.exists():
    with tarfile.open(archive) as package:
        base.write_bytes(package.extractfile(prefix + 'recipe.json').read())
recipe = json.loads(base.read_text())
for name in ['SM_ER_DustArch.obj', 'SM_ER_DustDoorLeft.obj', 'SM_ER_DustDoorRight.obj']:
    target = out / name
    if not target.exists():
        with tarfile.open(archive) as package: target.write_bytes(package.extractfile(prefix + name).read())
recipe.update(revision=1, id='desert.middle-eastern-alleys',
              reference='User minimap, 22 October 2026; https://www.johnsto.co.uk/design/making-dust2/; original seeded adaptation',
              covered_tunnels=True)
recipe['streets'].append(dict(name='Lower_Mid', points=[[25,55],[45,55]], width=[300,360], elevated=False))
recipe['invariants'].append('Lower B tunnel connects to Mid; twelve requested mud/concrete house variants; all three sand sources blended spatially')
(out / 'recipe.json').write_text(json.dumps(recipe, indent=2) + '\n')
attachment = Path('/var/folders/r4/xwb65b25699_ckq94myyghc80000gn/T/codex-clipboard-4d4c08b3-c7a0-4797-8402-827fa9c6b138.png')
if attachment.exists(): shutil.copy2(attachment, out / 'reference_minimap.png')
provenance = out / 'plan-provenance.json'
archive_sha = hashlib.sha256(archive.read_bytes()).hexdigest() if archive.exists() else json.loads(provenance.read_text())['source_archive_sha256']
provenance.write_text(json.dumps(dict(
    graph_origin='Project-owned DustTown revision4, archived 19 October; no old level or Arabian vendor packages restored',
    source_archive_sha256=archive_sha,
    sources={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('*.obj')},
    reference='https://www.johnsto.co.uk/design/making-dust2/',
    gameplay='Two A/B combat courts in the existing team deathmatch; no new bomb/capture rules'), indent=2) + '\n')
print('ER_ALLEYS_RECIPE', len(recipe['streets']), 'streets')
