"""Blender background: retain flying fragments and bake a small rigid skeletal effect.
Source archive/FBX remain untouched; no .uasset is written by this script.
"""
import bpy, json, math
from pathlib import Path
from mathutils import Matrix, Vector

project = Path(__file__).resolve().parents[1]
source = project / 'SourceAssets/BombCrackGround'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(source / 'source/Bomb Crack_Ground_00.fbx'))
scene = bpy.context.scene
scene.render.fps = 30
objects = [o for o in scene.objects if o.type == 'MESH']
rows = []
for o in objects:
    action = o.animation_data.action if o.animation_data else None
    if not action: continue
    zcurve = next((fc for fc in action.fcurves if fc.data_path == 'location' and fc.array_index == 2), None)
    if not zcurve: continue
    heights = [k.co.y for k in zcurve.keyframe_points]
    local = [v.co for v in o.data.vertices]
    size = max(max(v[a] for v in local) - min(v[a] for v in local) for a in range(3)) * .01
    # Reject the stationary floor and low, flat slabs, and microscopic specks.
    if max(heights) - min(heights) < .35 or size < .018: continue
    angle = math.atan2(o.location.y, o.location.x)
    sector = int((angle + math.pi) / (2 * math.pi) * 8) % 8
    rows.append((o, size, sector))
selected = []
for sector in range(8):
    group = sorted((r for r in rows if r[2] == sector), key=lambda r: (-r[1], r[0].name))
    # Eight large pieces plus eight evenly distributed smaller pieces per sector.
    selected.extend(group[:8])
    rest = group[8:]
    selected.extend(rest[int(i * len(rest) / 8)] for i in range(min(8, len(rest))))
selected.sort(key=lambda r: r[0].name)
assert len(selected) == 128

# Reference each rigid piece at its first visible frame. This keeps skinning
# well conditioned despite the source's tiny scales before particle birth.
refs = {}
for frame in range(10, 20):
    scene.frame_set(frame)
    for o, _, _ in selected:
        if o.name not in refs and max(o.scale) > .005:
            refs[o.name] = o.matrix_world.copy()
assert len(refs) == len(selected)

arm_data = bpy.data.armatures.new('BombDebrisSkeleton')
arm = bpy.data.objects.new('BombDebris', arm_data)
scene.collection.objects.link(arm)
bpy.context.view_layer.objects.active = arm
arm.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
root = arm_data.edit_bones.new('Root'); root.head = (0,0,0); root.tail = (0,.1,0)
for index, (o, _, _) in enumerate(selected):
    bone = arm_data.edit_bones.new('Shard_%03d' % index)
    bone.parent = root
    ref = refs[o.name] @ Matrix.Scale(100, 4)
    bone.head = ref.translation
    bone.tail = ref @ Vector((0,.04,0))
    bone.matrix = ref
bpy.ops.object.mode_set(mode='OBJECT')

vertices, faces, face_materials, groups, uvs = [], [], [], [], []
materials = [bpy.data.materials['Surface'], bpy.data.materials['Side']]
for index, (o, _, _) in enumerate(selected):
    start = len(vertices)
    vertices.extend(refs[o.name] @ v.co for v in o.data.vertices)
    faces.extend([start + i for i in p.vertices] for p in o.data.polygons)
    face_materials.extend(p.material_index for p in o.data.polygons)
    uv_layer = o.data.uv_layers.active
    uvs.extend(tuple(uv_layer.data[i].uv) if uv_layer else (0,0)
               for p in o.data.polygons for i in p.loop_indices)
    groups.append(list(range(start, len(vertices))))
mesh_data = bpy.data.meshes.new('BombDebrisGeometry')
mesh_data.from_pydata(vertices, [], faces); mesh_data.update()
uv_layer = mesh_data.uv_layers.new(name='UVMap')
for loop, uv in zip(uv_layer.data, uvs): loop.uv = uv
mesh = bpy.data.objects.new('SK_ER_BombDebris', mesh_data)
scene.collection.objects.link(mesh)
for m in materials: mesh_data.materials.append(m)
for polygon, material in zip(mesh_data.polygons, face_materials): polygon.material_index = material
for index, indices in enumerate(groups):
    mesh.vertex_groups.new(name='Shard_%03d' % index).add(indices, 1.0, 'REPLACE')
modifier = mesh.modifiers.new('RigidFragments', 'ARMATURE'); modifier.object = arm
mesh.parent = arm
for pose in arm.pose.bones: pose.rotation_mode = 'QUATERNION'
for frame in range(10, 102):
    scene.frame_set(frame)
    for index, (o, _, _) in enumerate(selected):
        pose = arm.pose.bones['Shard_%03d' % index]
        pose.matrix = o.matrix_world @ Matrix.Scale(100, 4)
        for prop in ('location', 'rotation_quaternion', 'scale'):
            pose.keyframe_insert(data_path=prop, frame=frame-9, group=pose.name)
for fc in arm.animation_data.action.fcurves:
    for k in fc.keyframe_points: k.interpolation = 'LINEAR'
# Remove originals only from the derivative scene, not the archived source.
fragment_names = [r[0].name for r in selected]
for o in objects: bpy.data.objects.remove(o, do_unlink=True)
scene.frame_start = 1; scene.frame_end = 92; scene.frame_set(1)
bpy.ops.object.select_all(action='DESELECT'); arm.select_set(True); mesh.select_set(True)
bpy.context.view_layer.objects.active = arm
output = source / 'SK_ER_BombDebris.fbx'
bpy.ops.export_scene.fbx(filepath=str(output), use_selection=True, object_types={'MESH','ARMATURE'},
    add_leaf_bones=False, bake_anim=True, bake_anim_use_all_bones=True,
    bake_anim_use_nla_strips=False, bake_anim_use_all_actions=False,
    bake_anim_simplify_factor=0, axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE')
report = {'source_objects':len(objects), 'eligible_flying_fragments':len(rows),
          'selected_fragments':len(selected), 'vertices':len(vertices), 'triangles':len(faces),
          'fps':30, 'frames':92, 'removed':'stationary floor/low slabs; downsampled airborne fragments',
          'fragment_names':fragment_names, 'output':output.name}
(source / 'bake-report.json').write_text(json.dumps(report, indent=2)+'\n')
print('ER_BOMB_BAKE_COMPLETE',json.dumps(report))
