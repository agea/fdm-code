#!/usr/bin/env python3
"""Preserve downloaded original archives in the private weapons repository.

Uses existing public model metadata and browser-authorized downloads. Does not
read browser credentials or run any code supplied with the model. Archive
members are bounded to their own directory and existing inputs are preserved.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import zipfile

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--downloads',type=Path,default=Path.home()/'Downloads')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]/'Assets/weapons/SourceAssets/SketchfabWeapons'
    report=[]
    for metadata in sorted(root.glob('*/metadata.json')):
        data=json.loads(metadata.read_text());directory=metadata.parent
        slug=data['viewerUrl'].split('/')[-1].rsplit('-',1)[0]
        archive=directory/'original.zip'
        if not archive.exists():
            download=args.downloads/(slug+'.zip')
            if not download.exists():
                print('ER_SOURCE_PENDING',slug)
                continue
            shutil.copy2(download,archive)
        with zipfile.ZipFile(archive) as package:
            entries=package.infolist()
            for entry in entries:
                name=Path(entry.filename)
                if name.is_absolute() or '..' in name.parts:raise ValueError('Unsafe archive path '+entry.filename)
            # Keep the complete original ZIP, extracting authoring inputs only.
            # Blend/max/project copies stay available inside that original ZIP.
            for entry in entries:
                path=directory/'original'/entry.filename
                if not path.exists() and not entry.is_dir() and Path(entry.filename).suffix.lower() in {'.fbx','.png','.tga','.jpg','.jpeg','.exr','.txt','.md','.pdf'}:
                    package.extract(entry,directory/'original')
        digest=hashlib.sha256()
        with archive.open('rb') as stream:
            for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
        item=dict(uid=data['uid'],name=data['name'],source=data['viewerUrl'],author=data['user']['displayName'],
                  author_url=data['user']['profileUrl'],license=data['license'],archive_sha256=digest.hexdigest(),
                  archive_bytes=archive.stat().st_size,source_fbx=[str(p.relative_to(directory)) for p in directory.glob('original/**/*.fbx')])
        (directory/'provenance.json').write_text(json.dumps(item,indent=2)+'\n')
        report.append(item);print('ER_SOURCE_STAGED',slug,len(item['source_fbx']))
    (root/'source-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    return len(report)==9

if __name__=='__main__':raise SystemExit(0 if main() else 1)
