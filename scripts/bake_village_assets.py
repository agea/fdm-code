"""Prepare owner-supplied Sketchfab sources; originals remain byte-identical.

Blender --background --factory-startup --python scripts/bake_village_assets.py
"""
from pathlib import Path
import json
import bpy
import bmesh
from mathutils import Vector

root=Path(__file__).resolve().parents[1]/'Assets/maps/SourceAssets/Village'
out=root/'imported-derived';out.mkdir(exist_ok=True)
survey=json.loads((root/'mesh-survey.json').read_text())
manifest=[]
def export(obj,name,source):
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    # Decimation can leave zero-area faces/unused vertices. Remove them before
    # Unreal tangent generation; FBX carries UVs, material binding is explicit.
    obj.data.validate(clean_customdata=True)
    if obj.data.has_custom_normals:bpy.ops.mesh.customdata_custom_splitnormals_clear()
    bm=bmesh.new();bm.from_mesh(obj.data)
    bmesh.ops.delete(bm,geom=[f for f in bm.faces if f.calc_area()<1.e-8],context='FACES')
    bmesh.ops.delete(bm,geom=[v for v in bm.verts if not v.link_faces],context='VERTS')
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(obj.data);bm.free()
    for slot in obj.material_slots:slot.material=bpy.data.materials.new(name+'_Slot_'+str(list(obj.material_slots).index(slot)))
    obj.name=name;obj.data.calc_loop_triangles()
    bounds=[obj.matrix_world@Vector(v) for v in obj.bound_box]
    lo=Vector([min(v[i] for v in bounds) for i in range(3)])
    hi=Vector([max(v[i] for v in bounds) for i in range(3)])
    obj.location=-Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,global_scale=1,
        apply_unit_scale=True,axis_forward='-Y',axis_up='Z',object_types={'MESH'},
        mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False)
    manifest.append(dict(name=name,source=source['uid'],triangles=len(obj.data.loop_triangles),
        size=list(hi-lo),materials=[m.name for m in obj.data.materials]))

for idx,e in enumerate(survey):
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    p=root/e['uid']/'original/source'/e['source'].split('/original/source/',1)[1]
    if p.suffix=='.fbx':bpy.ops.import_scene.fbx(filepath=str(p))
    elif p.suffix=='.glb':bpy.ops.import_scene.gltf(filepath=str(p))
    else:bpy.ops.wm.collada_import(filepath=str(p))
    obs=[o for o in bpy.context.scene.objects if o.type=='MESH']
    if idx<2:
        high=obs[0];obj=high.copy();obj.data=high.data.copy();bpy.context.collection.objects.link(obj)
        bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
        mod=obj.modifiers.new('Backdrop budget','DECIMATE');mod.ratio=6000/e['objects'][0]['triangles']
        bpy.ops.object.modifier_apply(modifier=mod.name)
        # Reproject source colour to a new low-poly atlas. Keeping the old UVs
        # after collapsing tiny chart boundaries creates visibly scrambled art.
        bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.012);bpy.ops.object.mode_set(mode='OBJECT')
        mat=bpy.data.materials.new('BakeTarget');mat.use_nodes=True;obj.data.materials.clear();obj.data.materials.append(mat)
        image=bpy.data.images.new('VillageBake',width=2048,height=2048,alpha=False)
        node=mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=image;mat.node_tree.nodes.active=node
        scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=1
        scene.render.bake.use_selected_to_active=True;scene.render.bake.cage_extrusion=.015;scene.render.bake.max_ray_distance=.06
        scene.render.bake.use_pass_direct=False;scene.render.bake.use_pass_indirect=False;scene.render.bake.use_pass_color=True;scene.render.bake.margin=12
        high.select_set(True);bpy.context.view_layer.objects.active=obj;bpy.ops.object.bake(type='DIFFUSE')
        image.filepath_raw=str(out/(['JungleColor','RustyColor'][idx]+'.png'));image.file_format='PNG';image.save()
        export(obj,['SM_Village_JungleHouse','SM_Village_RustyHouse'][idx],e)
    elif idx==2:
        obj=obs[0];bpy.context.view_layer.objects.active=obj
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        # Retain the original UV-mapped, hollow wooden room and its doorway.
        # Replace the upper sheet/rafters with calibrated gameplay metal/wood.
        bm=bmesh.new();bm.from_mesh(obj.data)
        bmesh.ops.delete(bm,geom=[f for f in bm.faces if max(v.co.z for v in f.verts)>3.1],context='FACES')
        bm.to_mesh(obj.data);bm.free();obj.data.update()
        # The source door contains a raised step/pallet which leaves too little
        # standing clearance after fitting. Cut a 1.6m-wide through doorway,
        # including the interior obstruction, while interpolating original UVs.
        obj.data.calc_loop_triangles();uv=obj.data.uv_layers.active
        def clip(poly,axis,at,less):
            result=[]
            def dist(v):return (v[0][axis]-at)*(1 if less else -1)
            prev=poly[-1];da=dist(prev)
            for cur in poly:
                db=dist(cur)
                if (da<=0)!=(db<=0):
                    t=da/(da-db);result.append((prev[0].lerp(cur[0],t),prev[1].lerp(cur[1],t)))
                if db<=0:result.append(cur)
                prev=cur;da=db
            return result
        polygons=[]
        for tri in obj.data.loop_triangles:
            inside=[(obj.data.vertices[obj.data.loops[i].vertex_index].co.copy(),uv.data[i].uv.copy()) for i in tri.loops]
            for axis,low,high in [(0,1.3,2.9),(2,-.1,2.45)]:
                if len(inside)<3:break
                outside=clip(inside,axis,low,True)
                if len(outside)>=3:polygons.append(outside)
                inside=clip(inside,axis,low,False)
                if len(inside)<3:break
                outside=clip(inside,axis,high,False)
                if len(outside)>=3:polygons.append(outside)
                inside=clip(inside,axis,high,True)
        vertices=[];faces=[];coords=[]
        for poly in polygons:
            faces.append(list(range(len(vertices),len(vertices)+len(poly))))
            vertices.extend(v[0] for v in poly);coords.extend(v[1] for v in poly)
        mesh=bpy.data.meshes.new('PlayableStorage');mesh.from_pydata(vertices,[],faces)
        layer=mesh.uv_layers.new(name='UVMap')
        for i,co in enumerate(coords):layer.data[i].uv=co
        mesh.materials.append(obj.data.materials[0]);obj.data=mesh
        export(obj,'SM_Village_StorageBody',e)
    elif idx==3:
        export(next(o for o in obs if o.name=='Wooden bridge pack asset 20'),'SM_Village_Bridge',e)
    else:
        # Source leaves and trunk have different UV-layer names. Merge their
        # actual maps into channel0 before joining the two material sections.
        for o in obs:
            chosen=o.data.uv_layers.get('leafUV' if o.name=='leaves' else 'Orco') or o.data.uv_layers.active
            for layer in list(o.data.uv_layers):
                if layer!=chosen:o.data.uv_layers.remove(layer)
            chosen.name='UVMap'
        bpy.ops.object.select_all(action='DESELECT')
        for o in obs:o.select_set(True)
        bpy.context.view_layer.objects.active=next(o for o in obs if o.name=='tree')
        bpy.ops.object.join();export(bpy.context.object,'SM_Village_Tree',e)

# Source texture reduction bounds the imported residency without altering ZIPs.
textures={
 'StorageColor':(2,'*pbr_diffuse*'), 'BridgeColor':(3,'Wooden_bridge_diffuse.jpg'),
 'BarkColor':(4,'TexturesCom_BarkDecidious0194_1_seamless_S.jpeg'),
 'LeavesColor':(4,'TexturesCom_Branches0018_1_masked_S.png')}
for name,(idx,pattern) in textures.items():
    src=next((root/survey[idx]['uid']/'original/textures').glob(pattern))
    image=bpy.data.images.load(str(src),check_existing=False)
    size=list(image.size);scale=min(1,2048/max(size))
    if scale<1:image.scale(round(size[0]*scale),round(size[1]*scale))
    image.filepath_raw=str(out/(name+'.png'));image.file_format='PNG';image.save()
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('VILLAGE_ASSETS_BAKED',json.dumps(manifest))
