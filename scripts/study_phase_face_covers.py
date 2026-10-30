"""Preview two source-derived Phase covers, with an explicit catalog restore mode.

Run with GeometryScripting enabled. -ERPhaseCoverRestore restores only the two
attachment arrays changed by the study. Existing mesh/material assets are never
edited. The private backup and active receipt live under Saved/PhaseFaceCoverStudy.
"""
from pathlib import Path
from datetime import datetime
import json, zipfile
import unreal as u

project=Path(u.Paths.project_dir()).resolve();assets=u.EditorAssetLibrary
root='/Game/EclipseRecoil/Characters/Teams/Gear'
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
folder=project/'Saved/PhaseFaceCoverStudy';folder.mkdir(exist_ok=True)
receipt=folder/'active.json'
if 'ERPhaseCoverRestore' in u.SystemLibrary.get_command_line():
    state=json.loads(receipt.read_text())
    if state.get('restored',False):
        print('ER_PHASE_COVERS_ALREADY_RESTORED')
    else:
        for path in paths:
            bp=assets.load_asset(path);c=u.get_default_object(bp.generated_class());choices=list(c.omega_skins)
            for idx in (5,6):
                assert not state['original_attachments'][path][str(idx)]
                a=choices[idx];a.set_editor_property('attachments',[])
                if 'original_hidden_material_slots' in state:
                    a.set_editor_property('hidden_material_slots',state['original_hidden_material_slots'][path][str(idx)])
                choices[idx]=a
            c.set_editor_property('omega_skins',choices);u.BlueprintEditorLibrary.compile_blueprint(bp)
            assert assets.save_loaded_asset(bp,False)
        state['restored']=True;state['catalogs_modified']=False;receipt.write_text(json.dumps(state,indent=2))
        print('ER_PHASE_COVERS_RESTORED catalogs=3')
else:
    if receipt.exists():
        previous=json.loads(receipt.read_text());assert previous.get('restored',False) or not previous.get('catalogs_modified',False),'Restore the preceding study first'
    c=u.get_default_object(assets.load_asset(paths[0]).generated_class())
    backup=folder/('catalogs-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
    state={'restored':False,'backup':str(backup),'original_attachments':{},'original_hidden_material_slots':{},'catalogs_modified':False}
    with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
        for path in paths:
            file=project/'Content'/(path.removeprefix('/Game/')+'.uasset');z.write(file,path.removeprefix('/Game/')+'.uasset')
            cat=u.get_default_object(assets.load_asset(path).generated_class())
            state['original_attachments'][path]={str(i):[p.mesh.get_path_name() for p in cat.omega_skins[i].attachments] for i in (5,6)}
            state['original_hidden_material_slots'][path]={str(i):list(cat.omega_skins[i].hidden_material_slots) for i in (5,6)}
            assert not any(state['original_attachments'][path].values()),'Study expects the approved attachment-free Phase variants'
    with zipfile.ZipFile(backup) as z:assert z.testzip() is None
    # Keep a receipt before the first possible catalog mutation.
    receipt.write_text(json.dumps(state,indent=2))
    source=c.alpha_skins[8]
    dm=u.DynamicMesh();u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(source.mesh,dm,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD(lod_type=u.GeometryScriptLODType.RENDER_DATA,lod_index=0))
    skeleton=u.SkeletonModifier();assert skeleton.set_skeletal_mesh(source.mesh)
    head_transform=skeleton.get_bone_transform('head',True)
    assert head_transform.translation.z>100,head_transform
    slot=next(i for i,s in enumerate(source.mesh.materials) if 'facemask' in str(s.material_slot_name).lower())
    selection=u.GeometryScript_MeshSelection.select_mesh_elements_by_material_id(dm,slot)[1]
    inverse=u.GeometryScript_MeshSelection.invert_mesh_selection(dm,selection)[1]
    u.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(dm,inverse)
    u.GeometryScript_MeshTransforms.inverse_transform_mesh(dm,head_transform)
    head_box=u.Box(min=[-25,-25,-25],max=[25,25,25])
    outside=u.GeometryScript_MeshSelection.select_mesh_elements_in_box(dm,head_box,invert=True)[1]
    u.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(dm,outside)
    clear=getattr(u.GeometryScript_Materials,next(n for n in dir(u.GeometryScript_Materials) if n.startswith('clear_material_i')));clear(dm,0)
    mat_path=root+'/M_ER_PhaseVisor_Study';mat=assets.load_asset(mat_path) if assets.does_asset_exist(mat_path) else assets.duplicate_asset(source.materials[slot].get_path_name(),mat_path)
    for node in u.MaterialEditingLibrary.get_material_expressions(mat):
        if str(node.get_editor_property('desc'))=='ER_GroundedTint':node.set_editor_property('default_value',u.LinearColor(.09,.10,.11,1))
    u.MaterialEditingLibrary.recompile_material(mat);assert assets.save_loaded_asset(mat,False)
    mask_path=root+'/SM_ER_PhaseVisor_Study';mask=assets.load_asset(mask_path) if assets.does_asset_exist(mask_path) else assets.duplicate_asset('/Engine/BasicShapes/Cube',mask_path)
    opts=u.GeometryScriptCopyMeshToAssetOptions(replace_materials=True,new_materials=[mat],new_material_slot_names=['Visor'])
    result=u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(dm,mask,opts,u.GeometryScriptMeshWriteLOD(lod_index=0))
    assert result[1]==u.GeometryScriptOutcomePins.SUCCESS,result
    assert assets.save_loaded_asset(mask,False)
    print('ER_PHASE_VISOR_BUILD',head_transform,mask.get_bounding_box())
    # A second study reuses Belica's authored head covering, not the rejected
    # procedural helmet. Keep its UVs, normals and separate visor material.
    hood_dm=u.DynamicMesh();u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(source.mesh,hood_dm,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD(lod_type=u.GeometryScriptLODType.RENDER_DATA,lod_index=0))
    upper=u.GeometryScript_MeshSelection.select_mesh_elements_by_material_id(hood_dm,0)[1]
    visor=u.GeometryScript_MeshSelection.select_mesh_elements_by_material_id(hood_dm,slot)[1]
    combined=u.GeometryScript_MeshSelection.combine_mesh_selections(upper,visor)
    inverse=u.GeometryScript_MeshSelection.invert_mesh_selection(hood_dm,combined)[1]
    u.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(hood_dm,inverse)
    u.GeometryScript_MeshTransforms.inverse_transform_mesh(hood_dm,head_transform)
    outside=u.GeometryScript_MeshSelection.select_mesh_elements_in_box(hood_dm,u.Box(min=[-12,-18,-15],max=[22,18,15]),invert=True)[1]
    u.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(hood_dm,outside)
    remap=getattr(u.GeometryScript_Materials,next(n for n in dir(u.GeometryScript_Materials) if n.startswith('remap_material_i')));remap(hood_dm,slot,1)
    hood_mat_path=root+'/M_ER_PhaseHood_Study';hood_mat=assets.load_asset(hood_mat_path) if assets.does_asset_exist(hood_mat_path) else assets.duplicate_asset(source.materials[0].get_path_name(),hood_mat_path)
    for node in u.MaterialEditingLibrary.get_material_expressions(hood_mat):
        if str(node.get_editor_property('desc'))=='ER_GroundedTint':node.set_editor_property('default_value',u.LinearColor(.72,.17,.02,1))
    u.MaterialEditingLibrary.recompile_material(hood_mat);assert assets.save_loaded_asset(hood_mat,False)
    hood_path=root+'/SM_ER_PhaseHood_Study';hood=assets.load_asset(hood_path) if assets.does_asset_exist(hood_path) else assets.duplicate_asset('/Engine/BasicShapes/Cube',hood_path)
    opts=u.GeometryScriptCopyMeshToAssetOptions(replace_materials=True,new_materials=[hood_mat,mat],new_material_slot_names=['Hood','Visor'])
    result=u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(hood_dm,hood,opts,u.GeometryScriptMeshWriteLOD(lod_index=0));assert result[1]==u.GeometryScriptOutcomePins.SUCCESS
    assert assets.save_loaded_asset(hood,False)
    print('ER_PHASE_HOOD_BUILD',hood.get_bounding_box())
    state['catalogs_modified']=True;receipt.write_text(json.dumps(state,indent=2))
    for path in paths:
        bp=assets.load_asset(path);cat=u.get_default_object(bp.generated_class());choices=list(cat.omega_skins)
        for idx,mesh,scale in [(5,mask,1.0),(6,hood,1.0)]:
            a=choices[idx];part=u.ERAppearanceAttachment(mesh=mesh,bone='head',transform=u.Transform(scale=[scale]*3))
            a.set_editor_property('attachments',[part])
            if idx==6:
                hidden=set(a.hidden_material_slots)
                hidden.update(i for i,s in enumerate(a.mesh.materials) if 'hair' in str(s.material_slot_name).lower())
                a.set_editor_property('hidden_material_slots',sorted(hidden))
            choices[idx]=a
        cat.set_editor_property('omega_skins',choices);u.BlueprintEditorLibrary.compile_blueprint(bp);assert assets.save_loaded_asset(bp,False)
    print('ER_PHASE_COVERS_STUDY_READY catalogs=3 backup='+str(backup))
