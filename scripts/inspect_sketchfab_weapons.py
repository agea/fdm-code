"""Read supplied FBX geometry/rig/actions with Blender, without source scripts.

Run: Blender --background --python scripts/inspect_sketchfab_weapons.py -- ROOT
The report stays in the private source asset mount.
"""
from pathlib import Path
import json
import sys
import bpy

root = Path(sys.argv[sys.argv.index('--') + 1]).resolve()
report = []
for source in sorted(root.glob('*/original/**/*.fbx')):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    try:
        bpy.ops.import_scene.fbx(filepath=str(source))
        bpy.context.view_layer.update()
        objects = []
        for obj in bpy.data.objects:
            item = dict(name=obj.name, type=obj.type, location=list(obj.location),
                        rotation=list(obj.rotation_euler), scale=list(obj.scale),
                        dimensions=list(obj.dimensions),
                        parent=obj.parent.name if obj.parent else None)
            if obj.type == 'MESH':
                item.update(vertices=len(obj.data.vertices),
                            materials=[m.name if m else None for m in obj.data.materials],
                            groups=[g.name for g in obj.vertex_groups])
            if obj.type == 'ARMATURE':
                item['bones'] = [dict(name=b.name, head=list(b.head_local),
                                     tail=list(b.tail_local), parent=b.parent.name if b.parent else None)
                                 for b in obj.data.bones]
            if obj.animation_data and obj.animation_data.action:
                item['action'] = obj.animation_data.action.name
            objects.append(item)
        actions = [dict(name=a.name, frames=list(a.frame_range)) for a in bpy.data.actions]
        materials = {}
        for material in bpy.data.materials:
            materials[material.name] = [dict(name=n.name, image=n.image.filepath)
                                       for n in material.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image] if material.use_nodes else []
        item = dict(source=str(source.relative_to(root)), fps=bpy.context.scene.render.fps,
                    objects=objects, actions=actions, materials=materials)
        report.append(item)
        print('ER_SOURCE_INSPECT', item['source'], [(o['name'], o['type'], o.get('vertices')) for o in objects], actions, flush=True)
    except Exception as exc:
        report.append(dict(source=str(source.relative_to(root)), error=str(exc)))
        print('ER_SOURCE_ERROR', str(source), str(exc), flush=True)
(root / 'source-inspection.json').write_text(json.dumps(report, indent=2) + '\n')
