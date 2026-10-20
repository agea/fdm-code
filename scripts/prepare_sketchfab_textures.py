#!/usr/bin/env python3
"""Prepare bounded texture-only runtime inputs; retain untouched source ZIPs/maps."""
from pathlib import Path
from PIL import Image, ImageOps
import hashlib
import json
import re
from sketchfab_texture_selection import SOURCES

ROOT=Path(__file__).resolve().parents[1]/'Assets/maps/SourceAssets/SketchfabTextures'
TILES={'concrete_seamless':200,'concrete_2k':200,'brick_tiled':200,'brick_textures':200,
       'wood_05':200,'asphalt_11':400,'grass_02':200,'snow_01':300,'iron_plates':200,'steel_irregular':150}

def classify(path):
    stem=re.sub(r'(?i)(?:^|[_ -])(?:[1248]k|\d{3,4}x\d{3,4})(?=[_ -]|$)','',path.stem)
    name=re.sub('[^a-z0-9]','',stem.lower())
    for role,tokens in [('Color',['basecolor','albedo','diffuse','colour','color','diff']),
                        ('Normal',['normaldx','normalgl','normal','nrm']),
                        ('Roughness',['roughness','rough']),('AO',['ambientocclusion','ao']),
                        ('Metallic',['metallic','metalness']),('Gloss',['glossiness','gloss'])]:
        for token in tokens:
            if token not in name:continue
            prefix,suffix=name.split(token,1)
            suffix=suffix.removeprefix('map').removeprefix('opengl').removeprefix('directx')
            return role,prefix+suffix
    return None,None

def main():
    materials=[]
    for asset in json.loads((ROOT/'download-manifest.json').read_text()):
        if asset['uid'] not in SOURCES:continue
        if asset['key'] not in TILES:continue
        directory=ROOT/asset['uid'];groups={}
        for path in sorted((directory/'original').rglob('*')):
            if path.suffix.lower() not in {'.png','.tga','.jpg','.jpeg','.tif','.tiff'}:continue
            r,group=classify(path)
            if r:groups.setdefault((path.parent,group),{})[r]=path
        # A multi-material download keeps separate map sets; no accidental mix.
        valid=[(group,maps) for group,maps in groups.items() if 'Color' in maps and 'Normal' in maps and ('Roughness' in maps or 'Gloss' in maps)]
        if asset['key']=='brick_tiled':
            # The author's normal/AO maps have numeric names, verified visually;
            # mud_* and radial belong to the demonstration mesh, not the brick tile.
            folder=directory/'original/textures'
            valid=[('brick',{'Color':folder/'07___Default__1__albedo.jpeg','Normal':folder/'4096.jpg',
                            'Roughness':folder/'07___Default__1__roughness.jpeg','Metallic':folder/'07___Default__1__metallic.jpeg','AO':folder/'ao_4096.jpg'})]
        for index,(folder,maps) in enumerate(valid):
            if 'Color' not in maps or 'Normal' not in maps:continue
            if 'Roughness' not in maps and 'Gloss' not in maps:continue
            key=asset['key']+('_'+str(index) if len(valid)>1 else '')
            prepared=directory/'prepared'/key;prepared.mkdir(parents=True,exist_ok=True)
            output={};sources={}
            for r,path in maps.items():
                im=Image.open(path)
                # Pillow clips 16-bit grayscale when converting directly to L/RGB.
                # Keep the source's absolute 0..65535 range instead of turning
                # concrete albedo, AO and roughness into solid white maps.
                if im.mode.startswith('I;16') or im.mode=='I':
                    im=im.convert('I').point(lambda value:value/257).convert('L')
                im=im.convert('RGB' if r in {'Color','Normal'} else 'L')
                im.thumbnail((2048,2048),Image.Resampling.LANCZOS)
                output_role=r
                if r=='Gloss':
                    if 'Roughness' in maps:continue
                    im=ImageOps.invert(im);output_role='Roughness'
                target=prepared/(output_role+'.png');im.save(target)
                output[output_role]=str(target.relative_to(ROOT));sources[output_role]=dict(file=str(path.relative_to(ROOT)),sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            materials.append(dict(id=key,source_uid=asset['uid'],name=asset['name'],source=asset['source'],
                                  author=asset['author'],author_url=asset['author_url'],license=asset['license'],
                                  maps=output,sources=sources,tile_cm=TILES[asset['key']],
                                  preparation_revision=2,
                                  # Green convention is an explicit art setting, never inferred from a generic filename.
                                  flip_normal_green=asset['key'] in {'snow_01','grass_02'},normal_strength=0.65,
                                  normal_convention_evidence='OpenGL inferred from strong height-gradient correlation; green converted for Unreal' if asset['key'] in {'snow_01','grass_02'} else 'Original convention retained; no reliable height-gradient evidence'))
    (ROOT/'materials-manifest.json').write_text(json.dumps(materials,ensure_ascii=False,indent=2)+'\n')
    for material in materials:print('PREPARED',material['id'],','.join(material['maps']))

if __name__=='__main__':main()
