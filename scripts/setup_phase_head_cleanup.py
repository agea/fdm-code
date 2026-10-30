"""Phase revision2: hairless Sniper and only helmet component0 on Grenadier.

The jacket hood is untouched. Preserve the numbered source component0 exactly,
all body/FPS meshes, cloth, materials and gameplay. Other helmet pieces are removed.
Run in Unreal with GeometryScripting enabled.
"""
from collections import defaultdict
from datetime import datetime
from pathlib import Path
import hashlib
import json
import zipfile
import unreal as ue

assets=ue.EditorAssetLibrary
project=Path(ue.Paths.project_dir()).resolve()
catalogs=['/Game/EclipseRecoil/Core/BP_ERCharacter',
          '/Game/EclipseRecoil/Core/BP_ERTrainingTarget',
          '/Game/EclipseRecoil/Core/BP_ERCorpse']
folder='/Game/EclipseRecoil/Characters/Teams/Omega/Skins/PhaseEngineer'
target_path=folder+'/SM_ER_PhaseEngineer_ArmoredHood'
source_path='/Game/EclipseRecoil/Characters/Teams/Gear/SM_ER_PhaseHood_Study'
target=assets.load_asset(target_path)
source=assets.load_asset(source_path)
assert target and source
backup=project/'Saved/AssetBackups'/('PhaseHeadCleanup-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
backup.parent.mkdir(parents=True,exist_ok=True)
def file(path):return project/'Content'/(path.removeprefix('/Game/').split('.')[0]+'.uasset')
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as archive:
    for path in catalogs+[target_path]:
        filename=file(path)
        archive.write(filename,str(filename.relative_to(project)))
with zipfile.ZipFile(backup) as archive:assert archive.testzip() is None
protected={p:digest(p) for p in (project/'Assets/characters/Content').rglob('*.uasset')
           if p.resolve()!=file(target_path).resolve()}

# Rebuild deterministically from the retained study, never from a previously
# trimmed mesh. Component numbering matches the supplied diagram: connected
# islands sorted by descending triangle count. Component0 is the red head shell.
mesh=ue.DynamicMesh()
result=ue.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(source,mesh,
    ue.GeometryScriptCopyMeshFromAssetOptions(),
    ue.GeometryScriptMeshReadLOD(lod_type=ue.GeometryScriptLODType.SOURCE_MODEL,lod_index=0))
assert result[1]==ue.GeometryScriptOutcomePins.SUCCESS
vertices={}
for i in range(mesh.get_num_vertex_i_ds()):
    position,valid=mesh.get_vertex_position(i)
    if valid:vertices[i]=position
triangles={};materials={};adjacency=defaultdict(list)
for i in range(mesh.get_num_triangle_i_ds()):
    triangle,valid=mesh.get_triangle_indices(i)
    if not valid:continue
    triangles[i]=(triangle.x,triangle.y,triangle.z)
    material,valid=mesh.get_triangle_material_id(i)
    assert valid
    materials[i]=material
    for vertex in triangles[i]:adjacency[vertex].append(i)
remaining=set(triangles);components=[]
while remaining:
    todo=[remaining.pop()];component=[]
    while todo:
        triangle=todo.pop();component.append(triangle)
        for vertex in triangles[triangle]:
            for other in adjacency[vertex]:
                if other in remaining:
                    remaining.remove(other);todo.append(other)
    components.append(component)
remove=[];records=[]
components.sort(key=len,reverse=True)
assert len(components[0])==3778, 'Unexpected source component0; inspect before changing the helmet'
for index,component in enumerate(components):
    highest=max(vertices[v].x for t in component for v in triangles[t])
    material_ids=sorted({materials[t] for t in component})
    keep=index==0
    if not keep:remove.extend(component)
    records.append(dict(index=index,triangles=len(component),max_head_x=highest,materials=material_ids,kept=keep))
assert len(triangles)-len(remove)==3778,records
selection=ue.GeometryScript_MeshSelection.convert_index_array_to_mesh_selection(
    mesh,remove,ue.GeometryScriptMeshSelectionType.TRIANGLES)[1]
ue.GeometryScript_MeshEdits.delete_selected_triangles_from_mesh(mesh,selection)
assert mesh.get_triangle_count()==len(triangles)-len(remove)
slots=list(target.get_editor_property('static_materials'))
options=ue.GeometryScriptCopyMeshToAssetOptions(replace_materials=True,
    new_materials=[s.material_interface for s in slots],
    new_material_slot_names=[s.material_slot_name for s in slots])
result=ue.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(mesh,target,options,
    ue.GeometryScriptMeshWriteLOD(lod_index=0))
assert result[1]==ue.GeometryScriptOutcomePins.SUCCESS
assets.set_metadata_tag(target,'ERPhaseHeadCleanupVersion','2')
assert assets.save_loaded_asset(target,False)

hidden_material=assets.load_asset('/Game/EclipseRecoil/Characters/Teams/Gear/M_ER_HiddenSection')
assert hidden_material
for path in catalogs:
    bp=assets.load_asset(path);defaults=ue.get_default_object(bp.generated_class())
    choices=list(defaults.omega_skins)
    # All Phase hair cards are hidden, including the Sniper's previous long hair.
    for index in range(5,10):
        appearance=choices[index]
        hidden=set(appearance.hidden_material_slots);overrides=list(appearance.materials)
        for slot,section in enumerate(appearance.mesh.materials):
            name=str(section.material_slot_name).lower()
            if 'hair_short' in name or 'hair_long' in name:
                hidden.add(slot);overrides[slot]=hidden_material
        appearance.set_editor_property('hidden_material_slots',sorted(hidden))
        appearance.set_editor_property('materials',overrides)
        choices[index]=appearance
    defaults.set_editor_property('omega_skins',choices)
    assets.set_metadata_tag(bp,'ERPhaseHeadCleanupVersion','2')
    assets.set_metadata_tag(bp,'ERPhaseReconLongHairVersion','removed')
    assets.set_metadata_tag(bp,'ERPhaseSniperHairVersion','hairless-1')
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assert assets.save_loaded_asset(bp,False)
    ue.log('ER_PHASE_HEAD_CLEANUP_CATALOG '+path)
assert all(digest(p)==before for p,before in protected.items()),'Protected character asset changed'
receipt=dict(backup=str(backup),source=source_path,helmet=target_path,
    source_triangles=len(triangles),removed_triangles=len(remove),
    kept_triangles=mesh.get_triangle_count(),components=records,
    protected_packages=len(protected),hidden_hair_skins=[5,6,7,8,9],helmet_components=[0],version=2)
(project/'Saved/Testing/phase-head-cleanup.json').write_text(json.dumps(receipt,indent=2))
ue.log('ER_PHASE_HEAD_CLEANUP_COMPLETE removed=%d kept=%d protected=%d backup=%s'%
       (len(remove),mesh.get_triangle_count(),len(protected),backup))
