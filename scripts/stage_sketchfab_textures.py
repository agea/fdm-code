#!/usr/bin/env python3
"""Archive authenticated original texture downloads and retain creator credits."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import zipfile
from sketchfab_texture_selection import SOURCES

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--downloads',type=Path,default=Path.home()/'Downloads')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]/'Assets/maps/SourceAssets/SketchfabTextures'
    manifest=json.loads((root/'collection-manifest.json').read_text())
    report=[]
    for entry in manifest['assets']:
        if entry['uid'] not in SOURCES:continue
        if not entry['downloadable']:continue
        directory=root/entry['uid'];archive=directory/'original.zip'
        slug=entry['source'].rsplit('/',1)[1].rsplit('-',1)[0]
        source=args.downloads/(slug+'.zip')
        if not archive.exists():
            if not source.exists():
                print('PENDING',entry['key']);continue
            if not zipfile.is_zipfile(source):raise ValueError('Incomplete ZIP '+str(source))
            shutil.copy2(source,archive)
        with zipfile.ZipFile(archive) as package:
            members=package.infolist()
            if sum(m.file_size for m in members)>8*1024**3:raise ValueError('Oversized package')
            for member in members:
                path=Path(member.filename)
                if path.is_absolute() or '..' in path.parts or (member.external_attr>>16)&0o170000==0o120000:
                    raise ValueError('Unsafe archive member '+member.filename)
                if not member.is_dir() and path.suffix.lower() in {'.png','.jpg','.jpeg','.tga','.tif','.tiff','.exr','.txt','.md','.pdf','.gltf','.json','.mtl'}:
                    target=directory/'original'/path
                    if not target.exists():package.extract(member,directory/'original')
        files=[]
        for path in sorted((directory/'original').rglob('*')):
            if path.is_file():files.append(dict(file=str(path.relative_to(directory)),bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
        receipt=dict(**entry,archive_bytes=archive.stat().st_size,archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),files=files)
        (directory/'provenance.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n')
        report.append(receipt);print('STAGED',entry['key'],len(files),'files')
    (root/'download-manifest.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')

if __name__=='__main__':main()
