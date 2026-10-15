#!/usr/bin/env python3
"""Read nested Workshop VPKs with the original converter and Source 2 Viewer.

Writes only private measurements/provenance; extracted resources stay in Saved.
No Source 2 mesh/texture is imported into Unreal. Supply an official
Source2Viewer-CLI executable via --viewer (https://s2v.app/).
"""
import argparse, collections, hashlib, json, re, subprocess, sys
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory_vpk', type=Path)
    parser.add_argument('--viewer', type=Path, required=True)
    parser.add_argument('--converter', type=Path, default=Path('/Users/agea/git/csgopen/eclipse-recoil/scripts/csgopen'))
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    sys.dont_write_bytecode = True
    sys.path.insert(0, str(args.converter))
    from sourcebsp import VpkArchive
    outer = VpkArchive(args.directory_vpk)
    names = [n for n in outer.entries if n.endswith('.vpk') and 'skybox' not in n]
    if len(names) != 1: raise ValueError('Expected one main nested map')
    args.work.mkdir(parents=True, exist_ok=True)
    nested = args.work / Path(names[0]).name
    data = outer.read(names[0])  # CRC checked by the read-only archive reader.
    if not nested.exists() or hashlib.sha256(nested.read_bytes()).digest() != hashlib.sha256(data).digest():
        nested.write_bytes(data)
    inner = VpkArchive(nested)
    log = args.work / 'entities-export.log'
    with log.open('w') as out:
        subprocess.run([str(args.viewer.resolve()), '-i', str(nested.resolve()), '-e', 'vents_c', '-d',
                        '-o', str((args.work / 'entities').resolve())], stdout=out, stderr=subprocess.STDOUT, check=True)
    entities = []
    for file in (args.work / 'entities').rglob('*.vents'):
        for chunk in re.split(r'====\d+====', file.read_text()):
            entity = {}
            for key in ['classname', 'origin', 'angles', 'model']:
                match = re.search(r'^' + key + r'\s+(.*)$', chunk, re.M)
                if match: entity[key] = match.group(1).strip().strip('"')
            if entity: entities.append(entity)
    classes = dict(collections.Counter(e.get('classname', 'unknown') for e in entities))
    report = dict(schema=1, workshop_id=args.directory_vpk.parent.name,
                  map=names[0], format='Source 2 nested VPK', status='full_geometry_present',
                  directory_sha256=hashlib.sha256(args.directory_vpk.read_bytes()).hexdigest(),
                  map_sha256=hashlib.sha256(data).hexdigest(), outer_entries=len(outer.entries),
                  map_entries=len(inner.entries), entities_by_class=classes,
                  spawns=[e for e in entities if e.get('classname') in ['info_player_terrorist', 'info_player_counterterrorist']],
                  map_resources=[n for n in inner.entries if n.endswith(('.vmap_c', '.vwrld_c', '.vwnod_c', '.vents_c'))],
                  adaptation={'critical_features': ['separate upper crossing and lower passage', 'narrow timber door passages'],
                              'method': 'Authored balanced arena with paired seeded cover offsets; Source geometry supplies reference only.',
                              'art': 'Owned MOUT_Arabian fixed architecture; existing project destruction catalog.',
                              'limits': 'Not a lossless conversion. Original CS2 materials are external dependencies; none imported. Mirrored source is retained as reference, not claimed as a rebuilt exact Dust II.'})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('SOURCE2_TEMPLATE_ANALYZED', report['map'], 'entries=' + str(len(inner.entries)), 'spawns=' + str(len(report['spawns'])))

if __name__ == '__main__': main()
