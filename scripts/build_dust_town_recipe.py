#!/usr/bin/env python3
"""Private plan graph and owned arch/curved timber-leaf mesh sources."""
import hashlib, json, math, shutil
from pathlib import Path

root = Path(__file__).resolve().parents[1]
out = root / 'Assets/maps/SourceAssets/MapTemplates/DustTown'
out.mkdir(parents=True, exist_ok=True)
streets = []
def street(name, points, width, elevated=False):
    streets.append(dict(name=name, points=points, width=width, elevated=elevated))
street('Mid', [(36,88),(45,88),(45,43),(55,43),(55,20),(57,20)], [440,500])
street('B_Tunnels', [(36,88),(18,88),(18,70),(25,70),(25,55),(18,55),(18,37),(15,37),(15,17)], [440,500])
street('B_CT', [(15,17),(15,30),(40,30),(40,25),(57,25),(57,20)], [300,360])
street('CT_A', [(57,20),(57,30),(92,30),(92,16),(81,16)], [340,400])
street('A_Long', [(81,16),(92,16),(92,67),(81,67)], [300,380])
street('Long_Approach', [(36,88),(65,88),(65,76),(81,76),(81,67)], [450,520])
street('Short_A', [(48,58),(62,58),(62,43),(68,43),(68,22),(90,22)], [300,360], True)
courts = [dict(name=n, center=c, size=s) for n,c,s in [
    ('T_Spawn',[36,88],[22,12]), ('CT_Spawn',[57,20],[12,10]),
    ('B_Court',[15,17],[18,18]), ('A_Court',[81,16],[16,18]),
    ('Long_Court',[81,67],[17,14]), ('Tunnel_Hall',[22,56],[12,10])]]
patrol = [(36,88),(18,88),(18,70),(25,70),(25,55),(18,55),(18,37),(15,37),(15,17),
          (15,30),(40,30),(40,25),(57,25),(57,20),(57,30),(92,30),(92,16),(81,16),
          (92,16),(92,67),(81,67),(81,76),(65,76),(65,88)]
recipe = dict(revision=4, arena_min=[6600,7300], arena_max=[7500,8400], axis_warp=2,
              building_height=[450,800], door_gap=[110,130], streets=streets,
              courts=courts, gates=[dict(street=n,position=p) for n,p in
                                  [('B_Tunnels',[18,46]),('Mid',[45,49]),('Long_Approach',[65,82])]],
              patrol=patrol, crossing=[68,30],
              invariants=['Southern T court / northern CT court', 'B west and A east courts',
                          'Mid, long route, bent B tunnels and elevated Short A',
                          'Short A crosses CT-A at +320cm over a 290cm clear passage',
                          'Three arched gateways: one leaf inward, one outward; frontal projection overlaps, diagonal 110..130cm slots'],
              reference='User supplied Dust plan and timber double-door photograph, followed by compact town and opposite-swing request, 15 October 2026. Approximate topology, not a pixel trace.')
(out/'recipe.json').write_text(json.dumps(recipe,indent=2)+'\n')

def extrude(name, polygon, thickness):
    # Watertight extrusion of a simple polygon in X/Z; Unreal imports the OBJ.
    if sum(polygon[i][0]*polygon[(i+1)%len(polygon)][1]-polygon[(i+1)%len(polygon)][0]*polygon[i][1] for i in range(len(polygon)))>0:
        polygon=list(reversed(polygon))
    vertices = [(x,y,z) for y in [-thickness/2, thickness/2] for x,z in polygon]
    count=len(polygon);faces=[]
    def cross(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    remaining=list(range(count));triangles=[]
    while len(remaining)>3:
        for i,b in enumerate(remaining):
            a,c=remaining[i-1],remaining[(i+1)%len(remaining)]
            if cross(polygon[a],polygon[b],polygon[c])>=-1e-7:continue
            if any(all(cross(polygon[u],polygon[v],polygon[q])<=1e-7 for u,v in [(a,b),(b,c),(c,a)]) for q in remaining if q not in [a,b,c]):continue
            triangles.append([a,b,c]);remaining.pop(i);break
        else:raise RuntimeError('Cannot triangulate owned mesh '+name)
    triangles.append(remaining)
    for tri in triangles:
        faces.append([i+1 for i in reversed(tri)])
        faces.append([count+i+1 for i in tri])
    for i in range(count):
        j=(i+1)%count;faces.append([i+1,j+1,count+j+1,count+i+1])
    text=['# Project-owned Dust town geometry; centimetres', 'o '+name, 's off']
    text += [f'v {x:.6f} {y:.6f} {z:.6f}' for x,y,z in vertices]
    text += [f'vt {x/500:.6f} {z/400:.6f}' for x,y,z in vertices]
    normals=[]
    for f in faces:
        a,b,c=[vertices[i-1] for i in f[:3]]
        u=[b[k]-a[k] for k in range(3)];v=[c[k]-a[k] for k in range(3)]
        n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
        length=sum(x*x for x in n)**.5
        if length<1e-8:raise RuntimeError('Degenerate source face '+name)
        normals.append([x/length for x in n])
    text += [f'vn {x:.8f} {y:.8f} {z:.8f}' for x,y,z in normals]
    text += ['f '+' '.join(str(i)+'/'+str(i)+'/'+str(j+1) for i in f) for j,f in enumerate(faces)]
    (out/(name+'.obj')).write_text('\n'.join(text)+'\n')

# A concave surround, open through its centre and down to the ground.
arch=[(-250,0),(-250,400),(250,400),(250,0),(210,0),(210,225)]
arch += [(210*math.cos(i*math.pi/32),225+95*math.sin(i*math.pi/32)) for i in range(1,33)]
arch += [(-210,0)]
extrude('SM_ER_DustArch',arch,80)
for right in [False,True]:
    points=[(-75,0),(75,0)]
    for i in range(24,-1,-1):
        u=i/24;h=225+75*math.sqrt(max(0,1-(1-u)**2))
        points.append((u*150-75,h))
    if right: points=[(-x,z) for x,z in reversed(points)]
    extrude('SM_ER_DustDoorRight' if right else 'SM_ER_DustDoorLeft',points,12)
for file,label in [('/var/folders/r4/xwb65b25699_ckq94myyghc80000gn/T/codex-clipboard-5d29b184-99ba-4ae2-9c0f-25b385d31c1b.png','reference_plan.png'),
                   ('/var/folders/r4/xwb65b25699_ckq94myyghc80000gn/T/codex-clipboard-ba700fcc-0e0e-43fc-87a4-6526982d5014.png','reference_doors.png')]:
    if Path(file).exists(): shutil.copyfile(file,out/label)
(out/'source_hashes.json').write_text(json.dumps({p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.suffix in ['.obj','.png']},indent=2)+'\n')
print('DUST_TOWN_RECIPE',len(streets),'streets;',len(courts),'courts; owned arch and two timber leaves')
