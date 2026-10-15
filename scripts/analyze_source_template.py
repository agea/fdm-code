#!/usr/bin/env python3
"""Read a BSP/workshop ZIP using the existing read-only Source converter.

Only measurements and provenance are written; no Valve meshes/textures are
imported. Keep reports and extracted radar images in the private maps mount.
"""
import argparse, collections, dataclasses, hashlib, importlib.util, io, json, sys, zipfile
from pathlib import Path


def analyze(source, converter):
    sys.dont_write_bytecode = True
    sys.path.insert(0, str(converter))
    spec = importlib.util.spec_from_file_location('sourcebsp', converter / 'sourcebsp.py')
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    data = source.read_bytes()
    name = source.name
    if zipfile.is_zipfile(io.BytesIO(data)):
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            maps = [n for n in archive.namelist() if n.lower().endswith('.bsp')]
            if len(maps) != 1: raise ValueError('Expected exactly one BSP; provide a specific BSP instead')
            name = maps[0]; data = archive.read(name)
    bsp = module.SourceBsp.read(data, name)
    # Sum actual triangle area, rather than face counts distorted by BSP splits.
    areas = collections.Counter()
    for triangle in bsp.playable_triangles():
        a,b,c = triangle.points
        u = [b[i]-a[i] for i in range(3)]; v = [c[i]-a[i] for i in range(3)]
        cross = (u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
        areas[triangle.material] += sum(x*x for x in cross)**.5 * .5 * .0254**2
    props = collections.Counter(p.model for p in bsp.static_props)
    return {'schema':1,'map':name,'workshop_id':source.parent.name,'archive_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
            'bsp_sha256':hashlib.sha256(data).hexdigest(),'bsp_version':bsp.version,'source_units_to_cm':2.54,
            'coordinate_mapping':'Unreal centimetres = (Source X, -Source Y, Source Z) * 2.54; spawn centre Z=100cm on ground',
            'world_bounds_source':bsp.world_bounds(),'entities_by_class':dict(collections.Counter(e.get('classname') for e in bsp.entities)),
            'brush_count':len(bsp.brushes),'static_prop_count':len(bsp.static_props),
            'spawns_source':[dataclasses.asdict(s) for s in bsp.spawns()],
            'spawn_entities':[e for e in bsp.entities if e.get('classname','').startswith('info_player_')],
            'material_area_m2':dict(areas.most_common()),'static_prop_models':dict(props.most_common()),
            'static_props':[dataclasses.asdict(p) for p in bsp.static_props],
            'adaptation':{'shell':'industrial shed, fixed concrete floor and corrugated metal shell',
                'partitions':'project Wood/Concrete/Drywall destruction profiles; geometry inferred as authored assembly slots',
                'cover':'cargo containers, crate/pallet stacks, barrels, bunker rooms and central tower; substitute meshes allowed',
                'variation':'bounded group offsets on a 50cm grid; fixed spawn bands and side circulation lanes',
                'gameplay':'existing FDM team deathmatch; original buy/bomb entities are evidence only',
                'limits':'inspired procedural adaptation, not lossless BSP reconstruction; source models and textures not imported'}}

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path)
    parser.add_argument('--converter',type=Path,default=Path('/Users/agea/git/csgopen/eclipse-recoil/scripts/csgopen'))
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    report=analyze(args.source,args.converter)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print('SOURCE_TEMPLATE_ANALYZED',report['map'],'props='+str(report['static_prop_count']),'spawns='+str(len(report['spawns_source'])),args.output)
