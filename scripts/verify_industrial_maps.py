"""Read-only validation of active maps, private art, PCG and preserved assets."""
import hashlib,json
from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir());assets=ue.EditorAssetLibrary
registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
maps={str(x.package_name).rsplit('/',1)[-1] for x in registry.get_assets_by_path('/Game/EclipseRecoil/Maps') if str(x.asset_class_path.asset_name)=='World'}
expected={'L_MainMenu','L_Depot','L_CentralBuilding'}
if maps!=expected:raise RuntimeError('Unexpected active maps '+str(maps))
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
if template.get_editor_property('archetype')!=ue.ERMapArchetype.CENTRAL_BUILDING or template.get_editor_property('symmetry')!=ue.ERTemplateSymmetry.HALF_TURN:raise RuntimeError('Incorrect industrial archetype')
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
    if not template.validate_layout(seed):raise RuntimeError('Invalid saved recipe '+str(seed))
wire=template.get_editor_property('central_building').get_editor_property('fence_mesh')
for field in ['container_mesh','tank_mesh','barrel_mesh']:
    mesh=template.get_editor_property('central_building').get_editor_property(field)
    for slot in range(len(mesh.get_editor_property('static_materials'))):
        surface=mesh.get_material(slot)
        if surface and not surface.get_base_material().get_editor_property('used_with_instanced_static_meshes'):
            raise RuntimeError('Industrial prop material cannot render instances: '+surface.get_path_name())
material=wire.get_material(1)
if material.get_editor_property('blend_mode')!=ue.BlendMode.BLEND_MASKED or not material.get_editor_property('two_sided'):raise RuntimeError('Wire material is not masked/two-sided')
mask_input=ue.MaterialEditingLibrary.get_material_property_input_node_output_name(material,ue.MaterialProperty.MP_OPACITY_MASK)
if mask_input!='R':raise RuntimeError('Incorrect wire mask channel '+str(mask_input))
preview=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/IndustrialPCG/DA_CentralBuilding_42_DenseCover')
graph=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/IndustrialPCG/PCG_CentralBuildingRoutes')
if not preview or not graph:raise RuntimeError('Missing PCGEx authoring assets')
pins={str(item.get_editor_property('pin')) for item in preview.get_editor_property('data').get_editor_property('tagged_data')}
if not {'CellVolumes','CoverPockets'}.issubset(pins):raise RuntimeError('PCG snapshot lacks destructible cells')
if template.get_editor_property('revision')<6:raise RuntimeError('Stale recipe revision')
recipe=template.get_editor_property('central_building')
steel=recipe.get_editor_property('steel_surface');palette=recipe.get_editor_property('container_surfaces')
if not steel or 'M_ER_FactoryGraySteel' not in steel.get_path_name() or recipe.get_editor_property('deck_surface')!=steel:raise RuntimeError('Wrong steel finish')
if len(palette)!=4 or len({m.get_path_name() for m in palette})!=4:raise RuntimeError('Missing container paint colors')

nodes=graph.get_editor_property('nodes')
if len(nodes)!=2 or not any(isinstance(n.get_settings(),ue.PCGExWritePathPropertiesSettings) for n in nodes):raise RuntimeError('Invalid PCGEx route-analysis graph')
loaders=[n.get_settings() for n in nodes if isinstance(n.get_settings(),ue.PCGLoadDataAssetSettings)]
if len(loaders)!=1 or 'DA_CentralBuilding_42_DenseCover' not in str(loaders[0].get_editor_property('asset')):raise RuntimeError('PCG graph uses a stale snapshot')
receipt=json.loads((root/'Saved/MapRefactorBackup/2026-10-19/receipt.json').read_text())
for name,sha in receipt['preserved_sha256'].items():
    if hashlib.sha256((root/name).read_bytes()).hexdigest()!=sha:raise RuntimeError('Preserved asset changed '+name)
if registry.get_assets_by_path('/Game/EclipseRecoil/Art/ArabianEnvironment',recursive=True):raise RuntimeError('Retired Arabian packages remain')
result=dict(industrial_steel=steel.get_path_name(),industrial_container_palette=[m.get_path_name() for m in palette],container_colors_independent=True,result='PASS',maps=sorted(maps),validated_seeds=516,pcg_graph_nodes=len(nodes),wire_mask_channel=mask_input,prop_material_instancing=True,preserved_sha256=receipt['preserved_sha256'])
catalog=assets.load_asset('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials')
result['material_surfaces']={str(row.get_editor_property('type')):str(row.get_editor_property('surface').get_path_name()) for row in catalog.get_editor_property('materials')}
(root/'Saved/Testing/industrial-assets-report.json').write_text(json.dumps(result,indent=2)+'\n')
print('ER_INDUSTRIAL_ASSETS_PASS '+json.dumps(result))
