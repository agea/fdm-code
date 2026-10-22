"""Import one original aluminium module and bind only Central Building Industrial."""
import hashlib,json,zipfile
from pathlib import Path
import unreal as ue
root=Path(__file__).resolve().parents[1];private=root/'Assets/maps'
assets=ue.EditorAssetLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
folder='/Game/EclipseRecoil/Art/Industrial';name='SM_ER_AluminiumFenceModule'
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
source=private/'SourceAssets/AluminiumFences'
# Retain a rollback before touching the saved recipe; never overwrite this backup.
backup=source/'before-modular-fence.zip'
if not backup.exists():
 with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
  p=private/'Content/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding.uasset';z.write(p,p.relative_to(private))
protected=[root/'Assets/maps/Content/EclipseRecoil/Maps/L_Depot.umap',root/'Assets/maps/Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset']
before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
path=folder+'/'+name
if not assets.does_asset_exist(path):
 task=ue.AssetImportTask();task.set_editor_property('filename',str(source/'derived'/f'{name}.fbx'));task.set_editor_property('destination_path',folder);task.set_editor_property('destination_name',name)
 task.set_editor_property('automated',True);task.set_editor_property('save',True)
 options=ue.FbxImportUI();options.set_editor_property('import_mesh',True);options.set_editor_property('import_materials',False);options.set_editor_property('import_textures',False);options.set_editor_property('import_as_skeletal',False)
 options.set_editor_property('mesh_type_to_import',ue.FBXImportType.FBXIT_STATIC_MESH)
 data=options.get_editor_property('static_mesh_import_data');data.set_editor_property('combine_meshes',True);data.set_editor_property('auto_generate_collision',False)
 task.set_editor_property('options',options);tools.import_asset_tasks([task])
mesh=assets.load_asset(path)
if not mesh:raise RuntimeError('Missing single fence module')
recipe=template.get_editor_property('central_building');steel=recipe.get_editor_property('steel_surface')
if steel.get_base_material().get_editor_property('blend_mode')!=ue.BlendMode.BLEND_OPAQUE:raise RuntimeError('Fence requires solid opaque steel')
mesh.set_material(0,steel)
body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',ue.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
subsystem=ue.get_editor_subsystem(ue.StaticMeshEditorSubsystem)
print('ER_FENCE_IMPORTED_BOUNDS',mesh.get_bounding_box(),'section_collision_before',subsystem.is_section_collision_enabled(mesh,0,0))
subsystem.enable_section_collision(mesh,True,0,0)
assets.set_metadata_tag(mesh,'SourceURL','https://sketchfab.com/3d-models/aluminium-fences-57-modular-parts-a2d4c1a74e724b7b8a5d6873a0c3561e')
assets.set_metadata_tag(mesh,'License','CC BY 4.0; Nullified (Nullifiedit); centered single straight module, existing project steel material')
if not assets.save_loaded_asset(mesh):raise RuntimeError('Cannot save fence collision')
recipe.set_editor_property('fence_module',mesh);template.set_editor_property('central_building',recipe)
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
 if not template.validate_layout(seed):raise RuntimeError('Invalid modular fence seed '+str(seed))
assets.set_metadata_tag(template,'ERIndustrialModularFenceRevision','1')
if not assets.save_loaded_asset(template):raise RuntimeError('Cannot save modular fence binding')
for p,h in before.items():
 if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h:raise RuntimeError('Protected asset changed '+p)
report=dict(result='PASS',mesh=mesh.get_path_name(),steel=steel.get_path_name(),validated_seeds=516,collision='original LOD0 triangles, complex as simple; separate existing pawn barrier',preserved_sha256=before)
(source/'import-receipt.json').write_text(json.dumps(report,indent=2)+'\n')
print('ER_INDUSTRIAL_MODULAR_FENCE_PASS '+json.dumps(report))
