"""Recover lid/lock parts and original FBX opening rotation curves (Blender)."""
from pathlib import Path
import bpy,json,sys
from mathutils import Vector,Matrix
root=Path(sys.argv[sys.argv.index('--')+1]);out=root/'derived'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(root/'pelican-case-game-ready/source/model.fbx'))
scene=bpy.context.scene;scene.frame_set(1)
objects={k:bpy.data.objects['Case_'+k] for k in ['Lower','Upper','Lock']}
curves={k:[] for k in ['Upper','Lock']}
initial={k:objects[k].rotation_euler.x for k in curves}
for f in range(1,102):
    scene.frame_set(f)
    for k in curves:curves[k].append(objects[k].rotation_euler.x-initial[k])
scene.frame_set(1)
meshes={k:bpy.data.meshes.new_from_object(o.evaluated_get(bpy.context.evaluated_depsgraph_get())) for k,o in objects.items()}
for k,m in meshes.items():m.transform(objects[k].matrix_world)
def bounds(m):return [Vector([fn(v.co[i] for v in m.vertices) for i in range(3)]) for fn in [min,max]]
lo,hi=bounds(meshes['Lower']);a,b=bounds(meshes['Upper'])
meshes['Upper'].transform(Matrix.Translation(Vector(((lo.x+hi.x-a.x-b.x)/2,(lo.y+hi.y-a.y-b.y)/2,hi.z-a.z-.025))))
a,b=bounds(meshes['Lock']);meshes['Lock'].transform(Matrix.Translation(Vector(((lo.x+hi.x-a.x-b.x)/2,lo.y-(a.y+b.y)/2,hi.z-(a.z+b.z)/2))))
points=[v.co for m in meshes.values() for v in m.vertices];low=Vector([min(p[i] for p in points) for i in range(3)]);high=Vector([max(p[i] for p in points) for i in range(3)])
scale=.70/max((high-low).x,(high-low).y);pivot=Vector(((low.x+high.x)/2,(low.y+high.y)/2,low.z))
for k,m in meshes.items():
    m.transform(Matrix.Scale(scale,4)@Matrix.Translation(-pivot))
    a,b=bounds(m)
    hinge=Vector((0,b.y,a.z)) if k=='Upper' else Vector((0,a.y,b.z)) if k=='Lock' else Vector()
    m.transform(Matrix.Translation(-hinge))
    obj=bpy.data.objects.new('SM_ReconCase'+k,m);bpy.context.collection.objects.link(obj)
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(out/(obj.name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,mesh_smooth_type='FACE')
    # FBX axis conversion Blender -> UE: X stays X, Y flips.
    curves[k+'Pivot']=[hinge.x*100,-hinge.y*100,hinge.z*100]
(out/'case-animation.json').write_text(json.dumps(curves,indent=2)+'\n')
