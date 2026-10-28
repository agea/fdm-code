"""Original log mesh and panoramic sky sphere; Poly Haven source stays intact.

Run with Blender --background --factory-startup --python.
"""
from pathlib import Path
import bpy,json,math
from mathutils import Vector

root=Path(__file__).resolve().parents[1]/'Assets/maps/SourceAssets/Village'
out=root/'topology-derived';out.mkdir(exist_ok=True)
def export(obj,name):
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    obj.name=name
    bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,global_scale=1,
        apply_unit_scale=True,axis_forward='-Y',axis_up='Z',object_types={'MESH'},
        mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False)
    obj.data.calc_loop_triangles();return len(obj.data.loop_triangles)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.mesh.primitive_cone_add(vertices=10,radius1=.14,radius2=.115,depth=4,location=(0,0,2))
pole=export(bpy.context.object,'SM_Village_PalisadeLog')
bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16,radius=1)
sky=export(bpy.context.object,'SM_Village_SkyDome')
src=root/'a5778d3c90e644e2be1062895ccd44aa/je_gray_park-original.jpg'
im=bpy.data.images.load(str(src));size=list(im.size);im.scale(4096,2048)
im.filepath_raw=str(out/'T_Village_GrayPark.png');im.file_format='PNG';im.save()
(out/'manifest.json').write_text(json.dumps(dict(pole_triangles=pole,sky_triangles=sky,panorama_source=str(src.relative_to(root)),source_size=size,runtime_texture_size=[4096,2048],geometry='Original generated meshes, backdrop panorama Greg Zaal / Poly Haven CC0'),indent=2)+'\n')
print('VILLAGE_BACKDROP_BAKED',pole,sky)
