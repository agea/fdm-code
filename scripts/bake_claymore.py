"""Blender 3.6: preserve source geometry/UVs, normalize cm and bake rigid FBX."""
from pathlib import Path
import bpy,json
from mathutils import Vector
root=Path(__file__).resolve().parents[1]/'Assets/weapons/SourceAssets/ClaymoreM18'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.wm.collada_import(filepath=str(root/'original/source/model/model/model.dae'))
objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
points=[o.matrix_world@Vector(p) for o in objects for p in o.bound_box]
lo=Vector([min(p[i] for p in points) for i in range(3)]);hi=Vector([max(p[i] for p in points) for i in range(3)])
print('ER_CLAYMORE_SOURCE',[(o.name,list(o.dimensions)) for o in objects],list(lo),list(hi),flush=True)
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=objects[0]
bpy.ops.object.join();mesh=bpy.context.object
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
# Source dimensions inferred from original bounds; largest dimension becomes22cm.
points=[mesh.matrix_world@Vector(p) for p in mesh.bound_box]
lo=Vector([min(p[i] for p in points) for i in range(3)]);hi=Vector([max(p[i] for p in points) for i in range(3)])
scale=.22/max(hi-lo)
# Keep authored front/lettering; record bounds for visual orientation review.
for v in mesh.data.vertices:
    world=mesh.matrix_world@v.co
    local=(world-Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z)))*scale
    # Authored convex front faces -Y; gameplay detects along +X.
    v.co=Vector((-local.y,local.x,local.z))
mesh.matrix_world.identity();mesh.name='Claymore_M18'
if not mesh.data.materials:mesh.data.materials.append(bpy.data.materials.new('Claymore_Surface'))
root.joinpath('derived').mkdir(exist_ok=True)
opts=dict(use_selection=True,apply_unit_scale=True,axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
bpy.ops.export_scene.fbx(filepath=str(root/'derived/SM_Claymore.fbx'),object_types={'MESH'},**opts)
armature=bpy.data.armatures.new('Claymore');rig=bpy.data.objects.new('Claymore',armature);bpy.context.collection.objects.link(rig)
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.object.mode_set(mode='EDIT');bone=armature.edit_bones.new('root');bone.head=(0,0,0);bone.tail=(0,0,.1);bpy.ops.object.mode_set(mode='OBJECT')
group=mesh.vertex_groups.new(name='root');group.add(list(range(len(mesh.data.vertices))),1,'REPLACE')
mod=mesh.modifiers.new('Rigid','ARMATURE');mod.object=rig;mesh.parent=rig
mesh.select_set(True)
bpy.ops.export_scene.fbx(filepath=str(root/'derived/SK_Claymore.fbx'),object_types={'MESH','ARMATURE'},**opts)
(root/'derived/bake-receipt.json').write_text(json.dumps(dict(source_bounds=[list(lo),list(hi)],scale=scale,vertices=len(mesh.data.vertices),dimensions_m=list(mesh.dimensions)),indent=2))
print('ER_CLAYMORE_BAKED',list(mesh.dimensions),flush=True)
