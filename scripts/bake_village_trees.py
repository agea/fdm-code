"""Generate original, bounded low-poly backdrop trees; no downloaded model code.

Blender --background --factory-startup --python scripts/bake_village_trees.py.
These are temporary original art while Sketchfab archive download is unavailable.
"""
from pathlib import Path
import bpy
import random
from mathutils import Vector
root=Path(__file__).resolve().parents[1]/'Assets/maps/SourceAssets/Village/derived'
root.mkdir(parents=True,exist_ok=True)
for variant in range(2):
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    rng=random.Random(113+variant)
    trunk=bpy.data.materials.new('Trunk');trunk.diffuse_color=(.18,.105,.05,1)
    leaves=bpy.data.materials.new('Leaves');leaves.diffuse_color=(.08,.23,.09,1)
    objects=[]
    def branch(a,b,r):
        d=Vector(b)-Vector(a)
        bpy.ops.mesh.primitive_cone_add(vertices=7,radius1=r,radius2=r*.55,depth=d.length,location=(Vector(a)+Vector(b))*.5)
        o=bpy.context.object;o.rotation_euler=d.to_track_quat('Z','Y').to_euler();o.data.materials.append(trunk);objects.append(o)
    branch((0,0,0),(.12,0,6),.22)
    for k in range(9):
        x=rng.uniform(-1.3,1.3);y=rng.uniform(-1.3,1.3);z=rng.uniform(4.5,7)
        branch((0,0,3.5),(x,y,z),.1)
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=(x,y,z))
        o=bpy.context.object;o.scale=(rng.uniform(1.1,1.6),rng.uniform(1.1,1.6),rng.uniform(1,1.5));o.data.materials.append(leaves);objects.append(o)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object
    o.name='SM_ER_VillageTree_'+str(variant)
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    bpy.ops.export_scene.fbx(filepath=str(root/(o.name+'.fbx')),use_selection=True,global_scale=100,apply_unit_scale=True,axis_forward='-Y',axis_up='Z',object_types={'MESH'},mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False)
    print('VILLAGE_TREE',o.name,len(o.data.polygons))
