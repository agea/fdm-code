#!/usr/bin/env python3
"""Draw actual Village block footprints from the native automation manifest."""
import json
from pathlib import Path

root=Path(__file__).resolve().parents[1]
out=root/'Saved/Testing'
rows=json.loads((out/'village-variety.json').read_text())
selected=[rows[seed] for seed in [42,1,5,9,13]]
styles=['Galleria aperta','Portico coperto','Baracca di base']
colors={1:'#69846b',2:'#b88a4d',3:'#a75b43'}
svg=["<svg xmlns='http://www.w3.org/2000/svg' width='1280' height='1280' viewBox='0 0 1280 760'><rect width='1280' height='760' fill='#f2f0e8'/><g font-family='Helvetica,Arial,sans-serif' fill='#24352c'>", "<text x='40' y='48' font-size='28' font-weight='bold'>Village · cinque varianti dello stesso template</text>","<text x='40' y='77' font-size='16'>Piante ricavate dai blocchi effettivamente generati in Unreal · simmetria a 180°</text>"]
for i,r in enumerate(selected):
    left=30+i*250
    svg.append(f"<rect x='{left}' y='102' width='235' height='564' rx='12' fill='#fffdf6'/><text x='{left+18}' y='134' font-size='20' font-weight='bold'>{len(r['blocks'])} {'blocco' if len(r['blocks'])==1 else 'blocchi'} · seed {r['seed']}</text><text x='{left+18}' y='160' font-size='15'>{r['width']/100:g} × {r['length']/100:g} m</text>")
    scale=min(194/r['width'],420/r['length']); cx=left+117.5;cy=400
    def rect(x,y,w,d,fill,stroke='#344d3c'):
        return f"<rect x='{cx+(x-w/2)*scale:.2f}' y='{cy+(y-d/2)*scale:.2f}' width='{w*scale:.2f}' height='{d*scale:.2f}' fill='{fill}' stroke='{stroke}' stroke-width='1.3'/>"
    svg.append(rect(0,0,r['width'],r['length'],'#e1e8d9'))
    for sign in [-1,1]:
        svg.append(rect(0,sign*(r['length']/2-220),r['width']-120,320,'#c0cfba','#a2b79c'))
        svg.append(f"<text x='{cx:.1f}' y='{cy+sign*(r['length']/2-220)*scale+4:.1f}' text-anchor='middle' font-size='10'>SPAWN</text>")
    for b in r['blocks']:
        svg.append(rect(b['x'],b['y'],b['w'],b['d'],colors[b['floors']]))
        svg.append(f"<text x='{cx+b['x']*scale:.1f}' y='{cy+b['y']*scale+4:.1f}' text-anchor='middle' font-size='12' font-weight='bold' fill='white'>{b['floors']}P</text>")
    for wall in r.get('side_walls',[]):
        svg.append(rect(wall['x'],wall['y'],wall['w'],wall['d'],'#657782','#657782'))
    svg.append(f"<text x='{left+18}' y='644' font-size='15'>{styles[r['base']]}</text>")
for i,(floors,color) in enumerate(colors.items()):
    x=40+i*190
    svg.append(f"<rect x='{x}' y='698' width='22' height='22' fill='{color}'/><text x='{x+32}' y='715' font-size='16'>{floors} {'piano' if floors==1 else 'piani'}</text>")
svg.append("<text x='610' y='715' font-size='12'>Muretti grigi: 2,4 m · bande spawn schematiche · scale indipendenti</text></g></svg>")
target=out/'Village-Variety.svg'
target.write_text(''.join(svg))
print(target)
