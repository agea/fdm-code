"""Blender: extract only the supplied single straight aluminium fence module."""
import hashlib,json
from pathlib import Path
import bpy
from mathutils import Vector
root=Path(__file__).resolve().parents[1]/'Assets/maps/SourceAssets/AluminiumFences'
out=root/'derived';out.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(root/'original/source/AluminiumFences.fbx'))
obj=bpy.data.objects['Fence_1_Straight_01_Horizontal']
points=[obj.matrix_world@v.co for v in obj.data.vertices]
lo=Vector([min(v[i] for v in points) for i in range(3)])
hi=Vector([max(v[i] for v in points) for i in range(3)])
for v,p in zip(obj.data.vertices,points):v.co=p-(lo+hi)*.5
obj.matrix_world.identity();obj.name='SM_ER_AluminiumFenceModule'
# Geometry, UVs and normals retained; the runtime uses the existing steel finish.
obj.data.materials.clear();obj.data.materials.append(bpy.data.materials.new('ExistingSteel'))
for p in obj.data.polygons:p.material_index=0
bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
path=out/(obj.name+'.fbx')
bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
report=dict(source_object='Fence_1_Straight_01_Horizontal',bounds_m=list(hi-lo),vertices=len(obj.data.vertices),triangles=sum(len(p.vertices)-2 for p in obj.data.polygons),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),modifications='Single original module, centered; existing project steel replaces source material. No decimation.')
(out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
print('ER_ALUMINIUM_FENCE_BAKED',json.dumps(report))
