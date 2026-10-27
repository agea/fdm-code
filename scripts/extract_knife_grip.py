"""Extract the supplied closed knife grip with Blender; retain original inputs.

Blender --background --python THIS -- /path/to/SketchfabWeapons
Output positions use the imported Unreal weapon's component frame, in cm.
"""
from pathlib import Path
import json
import math
import sys
import bpy
from mathutils import Matrix, Vector

root = Path(sys.argv[sys.argv.index('--')+1]).resolve()
source = next(root.glob('f3140*'))
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(next(source.glob('original/**/*.fbx'))))
scene = bpy.context.scene
scene.frame_set(1)
bpy.context.view_layer.update()
knife = bpy.data.objects['knife']
normalize = Matrix.Rotation(math.pi/2,4,'Z') @ Matrix.Translation(-knife.matrix_world.translation)
def rigid(m):
    p,q,_ = m.decompose()
    return Matrix.LocRotScale(p,q,Vector((1,1,1)))
idle = rigid(normalize @ knife.matrix_world)
scene.frame_set(107)
bpy.context.view_layer.update()
motion = rigid(normalize @ knife.matrix_world) @ idle.inverted()
canonical = motion.inverted() @ normalize
rig = next(o for o in bpy.data.objects if o.type=='ARMATURE')
def point(p):
    p = canonical @ rig.matrix_world @ p
    # Blender FBX -> the imported component frame: X, -Y, Z. This also
    # reproduces the imported mesh bounds, including the blade along -Y.
    return [p.x*100,-p.y*100,p.z*100]
mapping = dict(index=('point','Joint_3_7'),middle=('middle','Joint_3_8'),
               ring=('ring','Joint_3_9'),pinky=('pink','Joint_3_10'),thumb=('thumb','Joint_3_6'))
fingers = {}
for name,(token,end) in mapping.items():
    fingers[name] = [point(rig.pose.bones['R_'+token+str(i)].head) for i in (1,2,3)]
    fingers[name].append(point(rig.pose.bones[end].head))
receipt = dict(source_frame=107,wrist=point(rig.pose.bones['R_wrist'].head),fingers=fingers)
vertices = [canonical @ knife.matrix_world @ v.co for v in knife.data.vertices]
converted = [(v.x*100,-v.y*100,v.z*100) for v in vertices]
receipt['bounds'] = [[min(v[i] for v in converted),max(v[i] for v in converted)] for i in range(3)]
receipt['handle_sections'] = []
for station in range(-7,6):
    section = [v for v in converted if abs(v[1]-station)<.6]
    if section:
        receipt['handle_sections'].append(dict(y=station,x=[min(v[0] for v in section),max(v[0] for v in section)],
                                               z=[min(v[2] for v in section),max(v[2] for v in section)]))
(source/'derived/knife-source-grip.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('ER_KNIFE_SOURCE_GRIP '+json.dumps(receipt))
