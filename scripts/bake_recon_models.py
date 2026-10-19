"""Blender --background --gpu-backend opengl --python THIS -- private/ReconDrones.

Preserve originals/UVs/materials. Bake the supplied drone at 85cm, and the case
at 70cm with its animated lid/lock frozen and aligned closed. Export static FBX.
"""
from pathlib import Path
import json,sys
import bpy
from mathutils import Vector,Matrix

root=Path(sys.argv[sys.argv.index('--')+1]).resolve()
out=root/'derived';out.mkdir(exist_ok=True)
report={}
for key in ['Drone','Case']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    if key=='Drone':bpy.ops.wm.open_mainfile(filepath=str(root/'drone/Sketchfab_2019_06_04_16_16_22.blend'))
    else:bpy.ops.import_scene.fbx(filepath=str(root/'pelican-case-game-ready/source/model.fbx'))
    # The supplied blend hides its authoring objects/collection in the viewport.
    # Hidden objects otherwise return unevaluated meshes (losing mirrored arms).
    for collection in bpy.data.collections:
        collection.hide_viewport=False;collection.hide_render=False
    for obj in bpy.context.scene.objects:
        obj.hide_viewport=False;obj.hide_render=False;obj.hide_set(False)
    bpy.context.scene.frame_set(1);bpy.context.view_layer.update()
    source=[o for o in bpy.context.scene.objects if o.type=='MESH']
    baked=[]
    for obj in source:
        mesh=bpy.data.meshes.new_from_object(obj.evaluated_get(bpy.context.evaluated_depsgraph_get()))
        mesh.transform(obj.matrix_world)
        clone=bpy.data.objects.new(obj.name+'_Baked',mesh);bpy.context.collection.objects.link(clone);baked.append(clone)
    for obj in source:bpy.data.objects.remove(obj,do_unlink=True)
    def bounds(obj):
        return [Vector([min(v.co[i] for v in obj.data.vertices) for i in range(3)]),Vector([max(v.co[i] for v in obj.data.vertices) for i in range(3)])]
    if key=='Case':
        lower=next(o for o in baked if o.name.startswith('Case_Lower'))
        upper=next(o for o in baked if o.name.startswith('Case_Upper'))
        lock=next(o for o in baked if o.name.startswith('Case_Lock'))
        lo,hi=bounds(lower);a,b=bounds(upper)
        # Close the exploded source lid against the lower rim; retain mesh/UVs.
        upper.data.transform(Matrix.Translation(Vector(((lo.x+hi.x-a.x-b.x)/2,(lo.y+hi.y-a.y-b.y)/2,hi.z-a.z-.025))))
        a,b=bounds(lock)
        lock.data.transform(Matrix.Translation(Vector(((lo.x+hi.x-a.x-b.x)/2,lo.y-(a.y+b.y)/2,hi.z-(a.z+b.z)/2))))
    points=[v.co.copy() for o in baked for v in o.data.vertices]
    low=Vector([min(p[i] for p in points) for i in range(3)]);high=Vector([max(p[i] for p in points) for i in range(3)])
    width=.85 if key=='Drone' else .70
    scale=width/max((high-low).x,(high-low).y)
    pivot=(low+high)/2 if key=='Drone' else Vector(((low.x+high.x)/2,(low.y+high.y)/2,low.z))
    matrix=Matrix.Scale(scale,4)@Matrix.Translation(-pivot)
    for obj in baked:obj.data.transform(matrix)
    if key=='Drone':
        # Retain actual source geometry/materials for a hull and four arm/rotor
        # fragments. The intact mesh stays unchanged; fragments are centered.
        materials=[]
        for obj in baked:
            for mat in obj.data.materials:
                if mat not in materials:materials.append(mat)
        shards=[]
        for part in range(5):
            verts=[];faces=[];slots=[]
            for obj in baked:
                arm=obj.name.startswith('Circle')
                if (part==0)==arm:continue
                for poly in obj.data.polygons:
                    center=sum((obj.data.vertices[v].co for v in poly.vertices),Vector())/len(poly.vertices)
                    quadrant=1+(0 if center.x>=0 else 1)+(0 if center.y>=0 else 2)
                    if part and quadrant!=part:continue
                    start=len(verts);verts.extend(tuple(obj.data.vertices[v].co) for v in poly.vertices)
                    faces.append(tuple(range(start,len(verts))))
                    slots.append(materials.index(obj.data.materials[poly.material_index]) if poly.material_index<len(obj.data.materials) else 0)
            midpoint=Vector([(min(p[i] for p in verts)+max(p[i] for p in verts))/2 for i in range(3)])
            mesh=bpy.data.meshes.new('ReconShard');mesh.from_pydata([Vector(p)-midpoint for p in verts],[],faces);mesh.update()
            for mat in materials:mesh.materials.append(mat)
            for poly,slot in zip(mesh.polygons,slots):poly.material_index=slot
            name='SM_ReconShard'+str(part);obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
            bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
            bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,mesh_smooth_type='FACE')
            shards.append({'mesh':name,'vertices':len(mesh.vertices)})
        report['Debris']=shards
    bpy.ops.object.select_all(action='DESELECT')
    for obj in baked:obj.select_set(True)
    bpy.context.view_layer.objects.active=baked[0]
    bpy.ops.object.join()
    joined=bpy.context.object;joined.name='SM_Recon'+key
    bpy.ops.export_scene.fbx(filepath=str(out/(joined.name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,mesh_smooth_type='FACE')
    report[key]={'mesh':joined.name,'width_m':width,'vertices':len(joined.data.vertices),'materials':[m.name for m in joined.data.materials],
        'diffuse':{m.name:list(m.diffuse_color) for m in joined.data.materials},'modifications':'Static evaluated frame 1, centered/rescaled; case lid and lock aligned closed' if key=='Case' else 'Static evaluated source, centered/rescaled'}
(out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
print('ER_RECON_BAKE_PASS '+json.dumps(report))
