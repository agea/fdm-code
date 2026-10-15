"""Geometry-only palm surface measurements, in hand-bone local centimetres."""
import math

def add(a,b):return tuple(x+y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def mul(a,s):return tuple(x*s for x in a)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def length(a):return math.sqrt(dot(a,a))
def unit(a):return mul(a,1/max(1e-8,length(a)))
def percentile(values,t):return sorted(values)[round((len(values)-1)*t)]
def palm_surface(points,fingers):
    knuckles=[fingers[f] for f in ['index','middle','ring','pinky']]
    centre=mul(tuple(sum(p[i] for p in knuckles) for i in range(3)),.25)
    direction=unit(centre);across=sub(fingers['index'],fingers['pinky'])
    normal=unit(cross(direction,across))
    if dot(normal,fingers['thumb'])<0:normal=mul(normal,-1)
    lateral=unit(cross(normal,direction));span=length(across)
    selected=[p for p in points if .25*length(centre)<dot(p,direction)<.75*length(centre) and abs(dot(p,lateral))<span*.7]
    if len(selected)<8:raise RuntimeError('Insufficient palm surface geometry')
    contact=mul(centre,.55)
    contact=add(contact,mul(normal,percentile([dot(p,normal) for p in selected],.95)-dot(contact,normal)))
    return dict(position=contact,x=direction,y=lateral,z=normal,samples=len(selected))
