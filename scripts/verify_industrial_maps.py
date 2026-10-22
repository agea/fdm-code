"""Read-only validation of active maps, private art, PCG and preserved assets."""
import hashlib,json,runpy
from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir());assets=ue.EditorAssetLibrary
registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
maps={str(x.package_name).rsplit('/',1)[-1] for x in registry.get_assets_by_path('/Game/EclipseRecoil/Maps') if str(x.asset_class_path.asset_name)=='World'}
expected={'L_MainMenu','L_Depot','L_CentralBuilding','L_MiddleEasternAlleys'}
if maps!=expected:raise RuntimeError('Unexpected active maps '+str(maps))
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
if template.get_editor_property('archetype')!=ue.ERMapArchetype.CENTRAL_BUILDING or template.get_editor_property('symmetry')!=ue.ERTemplateSymmetry.HALF_TURN:raise RuntimeError('Incorrect industrial archetype')
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
    if not template.validate_layout(seed):raise RuntimeError('Invalid saved recipe '+str(seed))
if template.get_editor_property('central_building').get_editor_property('fence_mesh'):raise RuntimeError('Retired wire reference remains')
for field in ['container_mesh','tank_mesh','barrel_mesh']:
    mesh=template.get_editor_property('central_building').get_editor_property(field)
    for slot in range(len(mesh.get_editor_property('static_materials'))):
        surface=mesh.get_material(slot)
        if surface and not surface.get_base_material().get_editor_property('used_with_instanced_static_meshes'):
            raise RuntimeError('Industrial prop material cannot render instances: '+surface.get_path_name())
module=template.get_editor_property('central_building').get_editor_property('fence_module')
if not module or module.get_name()!='SM_ER_AluminiumFenceModule':raise RuntimeError('Missing single modular steel barrier')
if module.get_material(0).get_base_material().get_editor_property('blend_mode')!=ue.BlendMode.BLEND_OPAQUE:raise RuntimeError('Barrier steel must be opaque')
if module.get_editor_property('body_setup').get_editor_property('collision_trace_flag')!=ue.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:raise RuntimeError('Barrier must use original triangle collision')
mask_input='none (opaque steel geometry)'
snapshot='DA_CentralBuilding_42_SketchfabTextures' if template.get_editor_property('revision')>=7 else 'DA_CentralBuilding_42_DenseCover'
preview=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/IndustrialPCG/'+snapshot)
graph=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/IndustrialPCG/PCG_CentralBuildingRoutes')
if not preview or not graph:raise RuntimeError('Missing PCGEx authoring assets')
pins={str(item.get_editor_property('pin')) for item in preview.get_editor_property('data').get_editor_property('tagged_data')}
if not {'CellVolumes','CoverPockets'}.issubset(pins):raise RuntimeError('PCG snapshot lacks destructible cells')
if template.get_editor_property('revision')<6:raise RuntimeError('Stale recipe revision')
recipe=template.get_editor_property('central_building')
steel=recipe.get_editor_property('steel_surface');palette=recipe.get_editor_property('container_surfaces')
if not steel:raise RuntimeError('Missing steel finish')
if assets.get_metadata_tag(template,'ERSketchfabTextureRevision')=='2':
    if 'M_ER_SF_steel_irregular' not in steel.get_path_name() or 'M_ER_SF_iron_plates' not in recipe.get_editor_property('deck_surface').get_path_name():raise RuntimeError('Wrong Sketchfab steel/deck finish')
    for field,count in [('concrete_surfaces',2),('brick_surfaces',4),('wood_surfaces',1),('ground_surfaces',3)]:
        values=recipe.get_editor_property(field)
        if len(values)!=count or any(not value for value in values):raise RuntimeError('Incomplete Sketchfab palette '+field)
    selection=runpy.run_path(str(root/'scripts/sketchfab_texture_selection.py'))
    namespace='/Game/EclipseRecoil/Art/Industrial/SketchfabTextures'
    installed=registry.get_assets_by_path(namespace,recursive=True)
    names={str(item.asset_name) for item in installed if str(item.asset_class_path.asset_name)=='Material'}
    if names!={'M_ER_SF_'+key for key in selection['MATERIAL_IDS']}:raise RuntimeError('Sketchfab library differs from current selection')
    options=ue.AssetRegistryDependencyOptions(True,True,True,True,True)
    for item in installed:
        if not registry.get_referencers(item.package_name,options):raise RuntimeError('Unused Sketchfab asset remains '+str(item.package_name))
    source=root/'Assets/maps/SourceAssets/SketchfabTextures'
    manifest=json.loads((source/'materials-manifest.json').read_text())
    if {entry['id'] for entry in manifest}!=selection['MATERIAL_IDS']:raise RuntimeError('Stale Sketchfab source manifest')
    credits=(root/'Branding/Credits.txt').read_text()
    for entry in manifest:
        if entry['source'] not in credits:raise RuntimeError('Missing retained texture credit '+entry['id'])
        if any(not (source/relative).is_file() for relative in entry['maps'].values()):raise RuntimeError('Missing prepared map '+entry['id'])
    legacy_names={str(item.asset_name) for item in registry.get_assets_by_path('/Game/EclipseRecoil/Art/ProceduralTextures') if str(item.asset_class_path.asset_name)=='Material'}
    expected_legacy={'M_ER_PBR_'+key for key in ['Plaster001','red_brick_03','concrete_wall_006','wood_planks','stone_wall_02','rusty_metal_04','asphalt_02']}|{'M_ER_Glass'}
    if legacy_names!=expected_legacy:raise RuntimeError('Unexpected legacy texture library')
elif 'M_ER_FactoryGraySteel' not in steel.get_path_name() or recipe.get_editor_property('deck_surface')!=steel:raise RuntimeError('Wrong Factory steel finish')
if len(palette)!=4 or len({m.get_path_name() for m in palette})!=4:raise RuntimeError('Missing container paint colors')

nodes=graph.get_editor_property('nodes')
if len(nodes)!=2 or not any(isinstance(n.get_settings(),ue.PCGExWritePathPropertiesSettings) for n in nodes):raise RuntimeError('Invalid PCGEx route-analysis graph')
loaders=[n.get_settings() for n in nodes if isinstance(n.get_settings(),ue.PCGLoadDataAssetSettings)]
if len(loaders)!=1 or snapshot not in str(loaders[0].get_editor_property('asset')):raise RuntimeError('PCG graph uses a stale snapshot')
receipt=json.loads((root/'Saved/MapRefactorBackup/2026-10-19/receipt.json').read_text())
for name,sha in receipt['preserved_sha256'].items():
    if hashlib.sha256((root/name).read_bytes()).hexdigest()!=sha:raise RuntimeError('Preserved asset changed '+name)
if registry.get_assets_by_path('/Game/EclipseRecoil/Art/ArabianEnvironment',recursive=True):raise RuntimeError('Retired Arabian packages remain')
result=dict(industrial_steel=steel.get_path_name(),industrial_container_palette=[m.get_path_name() for m in palette],container_colors_independent=True,result='PASS',maps=sorted(maps),validated_seeds=516,pcg_graph_nodes=len(nodes),wire_mask_channel=mask_input,prop_material_instancing=True,preserved_sha256=receipt['preserved_sha256'])
catalog=assets.load_asset('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials')
result['material_surfaces']={str(row.get_editor_property('type')):str(row.get_editor_property('surface').get_path_name()) for row in catalog.get_editor_property('materials')}
(root/'Saved/Testing/industrial-assets-report.json').write_text(json.dumps(result,indent=2)+'\n')
print('ER_INDUSTRIAL_ASSETS_PASS '+json.dumps(result))
