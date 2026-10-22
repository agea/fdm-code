#!/usr/bin/env python3
"""Stage the four requested, normally downloaded Sketchfab sources privately."""
import argparse
import hashlib
import json
import shutil
import urllib.request
import zipfile
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1] / 'Assets/maps/SourceAssets/MiddleEasternAlleys'
SOURCES = {
    'fdd6f27663c049638afbbe30f9e9c93d': ('sand', 'tiled-pbr-material-sand'),
    'cbc349f96250408b8f75fa1171d0cadb': ('rocky_sand', 'rocky-sand-tile-texture'),
    '201fa0a7659c42cc865cf2ada9427bf6': ('loose_sand', 'loose-sand-tile-texture'),
    'bd4917fa72c84e41aa1e5299871f1b2e': ('houses', 'desert-mud-concrete-house-lowpoly'),
}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--downloads', type=Path, default=Path.home() / 'Downloads')
    args = p.parse_args()
    ROOT.mkdir(parents=True, exist_ok=True)
    entries = []
    for uid, (key, slug) in SOURCES.items():
        directory = ROOT / uid
        directory.mkdir(exist_ok=True)
        metadata = directory / 'metadata.json'
        if not metadata.exists():
            with urllib.request.urlopen('https://api.sketchfab.com/v3/models/' + uid, timeout=30) as response:
                metadata.write_text(json.dumps(json.load(response), ensure_ascii=False, indent=2) + '\n')
        data = json.loads(metadata.read_text())
        archive = directory / 'original.zip'
        if not archive.exists(): shutil.copy2(args.downloads / (slug + '.zip'), archive)
        with zipfile.ZipFile(archive) as package:
            if sum(m.file_size for m in package.infolist()) > 512 * 1024**2: raise ValueError('Oversized source')
            for member in package.infolist():
                path = Path(member.filename)
                if path.is_absolute() or '..' in path.parts or (member.external_attr >> 16) & 0o170000 == 0o120000:
                    raise ValueError('Unsafe archive member')
                # Sand demonstration meshes are not runtime assets.
                if member.is_dir() or path.suffix.lower() not in {'.png', '.jpeg', '.jpg', '.fbx'}: continue
                if path.suffix.lower() == '.fbx' and key != 'houses': continue
                target = directory / 'original' / path
                if not target.exists(): package.extract(member, directory / 'original')
        user, license = data['user'], data['license']
        entry = dict(uid=uid, key=key, name=data['name'], source=data['viewerUrl'],
                     author=user['displayName'], author_url=user['profileUrl'], license=license,
                     archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(), maps={})
        if key != 'houses':
            for role, token in [('Color', 'basecolor' if key != 'sand' else 'base_color'), ('Normal', 'normal'),
                                ('Roughness', 'roughness'), ('AO', 'ambientocclusion' if key != 'sand' else 'ambient_occlusion')]:
                candidates = [f for f in (directory / 'original/textures').iterdir()
                              if token in f.stem.lower() and '@' not in f.name]
                if len(candidates) != 1: raise ValueError(f'Ambiguous {key} {role}: {candidates}')
                source = candidates[0]
                im = Image.open(source)
                if im.mode.startswith('I;16') or im.mode == 'I': im = im.convert('I').point(lambda v: v / 257).convert('L')
                im = im.convert('RGB' if role in {'Color', 'Normal'} else 'L')
                im.thumbnail((2048, 2048), Image.Resampling.LANCZOS)
                target = directory / 'prepared' / (role + '.png')
                target.parent.mkdir(exist_ok=True)
                im.save(target)
                entry['maps'][role] = str(target.relative_to(ROOT))
        entry['files'] = {str(f.relative_to(directory)): hashlib.sha256(f.read_bytes()).hexdigest()
                          for f in (directory / 'original').rglob('*') if f.is_file()}
        credits = f"{entry['name']} — {entry['author']}\n{entry['source']}\n{entry['author_url']}\n{license['url']}\nChanges: Unreal adaptation, texture resizing/world projection; house origins and units normalized.\n"
        (directory / 'CREDITS.txt').write_text(credits)
        (directory / 'provenance.json').write_text(json.dumps(entry, ensure_ascii=False, indent=2) + '\n')
        entries.append(entry)
        print('STAGED', key, entry['name'], license['slug'])
    (ROOT / 'manifest.json').write_text(json.dumps(entries, ensure_ascii=False, indent=2) + '\n')
    (ROOT / 'CREDITS.txt').write_text('\n'.join((ROOT / e['uid'] / 'CREDITS.txt').read_text() for e in entries))

if __name__ == '__main__': main()
