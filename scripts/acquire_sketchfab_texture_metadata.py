#!/usr/bin/env python3
"""Preserve public metadata/credits for the owner's RR texture selection.

Original downloads are obtained through the normal signed-in Sketchfab UI.
No browser credentials or viewer-only texture resources are used.
"""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import hashlib
import json
import urllib.request
from sketchfab_texture_selection import SOURCES

ROOT = Path(__file__).resolve().parents[1] / 'Assets/maps/SourceAssets/SketchfabTextures'
COLLECTION = 'https://sketchfab.com/agea1/collections/rr-792d73a736504af1b1bd0e05599fc256'
MODELS = SOURCES

def acquire(pair):
    uid, key = pair
    directory = ROOT / uid
    directory.mkdir(parents=True, exist_ok=True)
    metadata = directory / 'metadata.json'
    if metadata.exists():
        data = json.loads(metadata.read_text())
    else:
        with urllib.request.urlopen('https://api.sketchfab.com/v3/models/' + uid, timeout=30) as response:
            data = json.load(response)
        metadata.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n')
    author, license = data['user'], data.get('license') or {}
    credit = (f"{data['name']}\nby {author['displayName']} ({author['profileUrl']})\n"
              f"Source: {data['viewerUrl']}\nLicense: {license.get('fullName', 'Unspecified')}\n"
              f"{license.get('url', '')}\nCollection: {COLLECTION}\n"
              "Preserved metadata: Sketchfab public API, 20 October 2026.\n"
              "Adaptations, if imported: texture-only Unreal PBR material, world projection, scale/normal conversion.\n")
    (directory / 'CREDITS.txt').write_text(credit)
    preview = directory / 'preview.jpg'
    if not preview.exists():
        images = data.get('thumbnails', {}).get('images', [])
        if images:
            image = min(images, key=lambda x: abs(x.get('width', 0) - 640))
            with urllib.request.urlopen(image['url'], timeout=30) as response:
                preview.write_bytes(response.read())
    return dict(uid=uid, key=key, name=data['name'], source=data['viewerUrl'],
                author=author['displayName'], author_url=author['profileUrl'], license=license,
                downloadable=data.get('isDownloadable'), metadata_sha256=hashlib.sha256(metadata.read_bytes()).hexdigest())

if __name__ == '__main__':
    ROOT.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(max_workers=4) as pool:
        entries = list(pool.map(acquire, MODELS.items()))
    (ROOT / 'collection-manifest.json').write_text(json.dumps(dict(collection=COLLECTION, assets=entries), ensure_ascii=False, indent=2) + '\n')
    (ROOT / 'CREDITS.txt').write_text('\n\n'.join((ROOT / entry['uid'] / 'CREDITS.txt').read_text() for entry in entries))
    for entry in entries:
        print(entry['key'], entry['name'], entry['author'], entry['license'].get('slug'), 'downloadable=' + str(entry['downloadable']))
