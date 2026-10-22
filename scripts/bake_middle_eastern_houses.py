"""Blender: preserve each of the twelve supplied house variants as a mesh."""
import hashlib
import json
from pathlib import Path
import bpy
from mathutils import Vector

root = Path(__file__).resolve().parents[1] / 'Assets/maps/SourceAssets/MiddleEasternAlleys'
source = root / 'bd4917fa72c84e41aa1e5299871f1b2e'
out = root / 'prepared_houses'
out.mkdir(exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(source / 'original/source/House Lowpoly.fbx'))
meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
if len(meshes) != 12: raise RuntimeError('Expected twelve supplied houses')
receipt = []
for obj in sorted(meshes, key=lambda o: o.name):
    original_name = obj.name
    vertices = [obj.matrix_world @ v.co for v in obj.data.vertices]
    lo = Vector([min(v[i] for v in vertices) for i in range(3)])
    hi = Vector([max(v[i] for v in vertices) for i in range(3)])
    offset = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z))
    for vertex, position in zip(obj.data.vertices, vertices): vertex.co = position - offset
    obj.matrix_world.identity()
    name = 'SM_ER_' + original_name.replace(' ', '')
    obj.name = name
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    target = out / (name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(target), use_selection=True, object_types={'MESH'},
                             add_leaf_bones=False, bake_anim=False, axis_forward='-Y', axis_up='Z')
    receipt.append(dict(name=name, original=original_name, material=obj.data.materials[0].name,
                        bounds_m=list(hi - lo), sha256=hashlib.sha256(target.read_bytes()).hexdigest()))
(out / 'manifest.json').write_text(json.dumps(receipt, indent=2) + '\n')
print('ER_ALLEY_HOUSES_BAKED', len(receipt))
