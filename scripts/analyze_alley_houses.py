"""Blender read-only geometry survey; normalized roof/door samples, no mesh edits."""
import json
from pathlib import Path
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

root=Path(__file__).resolve().parents[1]/'Assets/maps/SourceAssets/MiddleEasternAlleys'
report=[]
for entry in json.loads((root/'prepared_houses/manifest.json').read_text()):
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.fbx(filepath=str(root/'prepared_houses'/(entry['name']+'.fbx')))
    obj=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    vs=[obj.matrix_world@v.co for v in obj.data.vertices]
    lo=Vector([min(v[i] for v in vs) for i in range(3)]);size=Vector([max(v[i] for v in vs)-lo[i] for i in range(3)])
    vertices=[Vector([(v[i]-lo[i])/size[i] for i in range(3)]) for v in vs]
    tree=BVHTree.FromPolygons(vertices,[list(p.vertices) for p in obj.data.polygons])
    roofs=[]
    for x in [.10,.15,.2,.3,.5,.7,.85,.9]:
        for y in [.1,.15,.2,.5,.8]:
            hit=tree.ray_cast(Vector((x,y,1.2)),Vector((0,0,-1)),1.3)
            if hit[0] is not None and hit[0].z>.55:roofs.append([x,y,round(hit[0].z,4)])
    doors=[]
    # Survey for an 80cm-wide, 185cm-high opening when fitted to 8x8.5x6m.
    for axis in [0,1]:
        for side in [0,1]:
            for along in [.1,.2,.3,.4,.5,.6,.7,.8,.9]:
                clear=True
                for offset in [-.05,0,.05]:
                    for z in [.05,.15,.25,.31]:
                        p=Vector((along+offset,along+offset,z));p[axis]=-.08 if side==0 else 1.08
                        direction=Vector((0,0,0));direction[axis]=1 if side==0 else -1
                        if tree.ray_cast(p,direction,.30)[0] is not None:clear=False
                if clear:doors.append(dict(axis=axis,side=side,along=along))
    report.append(dict(name=entry['name'],roofs=roofs,doors=doors))
    print('HOUSE_TRAVERSAL',entry['name'],'roof-left',[(x,y,z) for x,y,z in roofs if x==.15 and y==.15],'doors',doors)
(root/'house-traversal.json').write_text(json.dumps(report,indent=2)+'\n')
