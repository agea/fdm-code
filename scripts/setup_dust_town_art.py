"""One-time owned town art revision: hard normals and natural timber grain."""
import hashlib,json
from pathlib import Path
import unreal as ue

root=Path(ue.Paths.project_dir());source=root/'Assets/maps/SourceAssets/MapTemplates/DustTown'
folder='/Game/EclipseRecoil/Art/MapTemplates/DustTown'
assets=ue.EditorAssetLibrary;edit=ue.MaterialEditingLibrary
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_DustMirror')
if not template or not template.get_editor_property('town').enabled:raise RuntimeError('Town migration required')
if assets.get_metadata_tag(template,'ERDustTownArtRevision')=='1':
    ue.log('ER_DUST_TOWN_ART_PRESERVED')
else:
    for name in ['SM_ER_DustArch','SM_ER_DustDoorLeft','SM_ER_DustDoorRight']:
        obj=source/(name+'.obj');sha=hashlib.sha256(obj.read_bytes()).hexdigest();mesh=assets.load_asset(folder+'/'+name)
        if assets.get_metadata_tag(mesh,'EROwnedOBJHash')!=sha:
            task=ue.AssetImportTask();task.filename=str(obj);task.destination_path=folder;task.destination_name=name
            task.automated=True;task.save=True;task.replace_existing=True
            options=ue.FbxImportUI();options.set_editor_property('import_as_skeletal',False)
            options.set_editor_property('import_materials',False);options.set_editor_property('import_textures',False)
            options.static_mesh_import_data.set_editor_property('normal_import_method',ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
            options.static_mesh_import_data.set_editor_property('convert_scene',False)
            options.static_mesh_import_data.set_editor_property('convert_scene_unit',False)
            task.options=options;ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            mesh=assets.load_asset(folder+'/'+name);assets.set_metadata_tag(mesh,'EROwnedOBJHash',sha)
            if not assets.save_loaded_asset(mesh):raise RuntimeError('Cannot save '+name)
    path=folder+'/M_ER_TownTimber'
    wood=assets.load_asset(path) if assets.does_asset_exist(path) else None
    if not wood:
        wood=assets.duplicate_asset('/Game/EclipseRecoil/Art/ProceduralTextures/M_ER_PBR_wood_planks',path)
        previous=edit.get_material_property_input_node(wood,ue.MaterialProperty.MP_BASE_COLOR)
        color=edit.create_material_expression(wood,ue.MaterialExpressionConstant3Vector,0,-200)
        color.set_editor_property('constant',ue.LinearColor(1.1,.65,.3,1))
        multiply=edit.create_material_expression(wood,ue.MaterialExpressionMultiply,200,0)
        edit.connect_material_expressions(previous,'RGB',multiply,'A');edit.connect_material_expressions(color,'',multiply,'B')
        edit.connect_material_property(multiply,'',ue.MaterialProperty.MP_BASE_COLOR)
        # Transpose the UV and its normal basis together: vertical board grain
        # on the gates, without altering any shared project material.
        for node in edit.get_material_expressions(wood):
            if not isinstance(node,ue.MaterialExpressionCustom):continue
            code=node.get_editor_property('code')
            if node.get_editor_property('output_type')==ue.CustomMaterialOutputType.CMOT_FLOAT2:
                code=code.replace('float2(P.y*sign(N.x),-P.z)','float2(-P.z,P.y*sign(N.x))').replace('float2(-P.x*sign(N.y),-P.z)','float2(-P.z,-P.x*sign(N.y))').replace('float2(P.x*sign(N.z),-P.y)','float2(-P.y,P.x*sign(N.z))')
            else:code=code.replace('T.x','T.__temp__').replace('T.y','T.x').replace('T.__temp__','T.y')
            node.set_editor_property('code',code)
        edit.recompile_material(wood)
        if not assets.save_loaded_asset(wood):raise RuntimeError('Cannot save town timber')
    town=template.get_editor_property('town');town.set_editor_property('wood_surface',wood);template.set_editor_property('town',town)
    assets.set_metadata_tag(template,'ERDustTownArtRevision','1')
    if not assets.save_loaded_asset(template):raise RuntimeError('Cannot save town art palette')
    (source/'art_revision.json').write_text(json.dumps({'revision':1,'door_surface':wood.get_path_name(),'mesh_sources':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in source.glob('*.obj')},'catalog_edited':False},indent=2)+'\n')
    ue.log('ER_DUST_TOWN_ART_COMPLETE')
