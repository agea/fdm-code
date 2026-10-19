"""Save a PCGEx route-analysis graph from the validated industrial recipe."""
import unreal as ue
assets=ue.EditorAssetLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
folder='/Game/EclipseRecoil/Art/MapTemplates/IndustrialPCG'
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
if not template:raise RuntimeError('Create the industrial recipe first')
data_path=folder+('/DA_CentralBuilding_42_DenseCover' if template.get_editor_property('revision')>=6 else '/DA_CentralBuilding_42_FactoryFinishes' if template.get_editor_property('revision')>=5 else '/DA_CentralBuilding_42_PreferredLadders' if template.get_editor_property('revision')>=4 else '/DA_CentralBuilding_42_Ladders' if template.get_editor_property('revision')>=3 else '/DA_CentralBuilding_42_Evolution')
data=assets.load_asset(data_path) if assets.does_asset_exist(data_path) else None
if not data:
    data=template.save_pcg_preview(data_path,42)
    if not data:raise RuntimeError('Cannot save PCG preview')
graph_path=folder+'/PCG_CentralBuildingRoutes'
if not assets.does_asset_exist(graph_path):
    factory=ue.PCGGraphFactory()
    graph=tools.create_asset('PCG_CentralBuildingRoutes',folder,ue.PCGGraph,factory)
    loader,settings=graph.add_node_of_type(ue.PCGLoadDataAssetSettings)
    settings.set_editor_property('asset',data)
    # Updating an asset reference rebuilds the node's cached dynamic pins.
    settings.post_edit_change() if hasattr(settings,'post_edit_change') else None
    node,properties=graph.add_node_of_type(ue.PCGExWritePathPropertiesSettings)
    properties.set_editor_property('write_path_length',True)
    print('ER_PCG_PREVIEW_PINS load='+str([x.properties.label for x in loader.get_editor_property('output_pins')]))
    if not graph.add_edge(loader,'Crossings',node,'Paths'):raise RuntimeError('Cannot connect crossing paths')
    if not graph.add_edge(node,'Paths',graph.get_output_node(),'Out'):raise RuntimeError('Cannot connect analyzed paths')
    if not assets.save_loaded_asset(graph):raise RuntimeError('Cannot save PCGEx graph')
else:
    graph=assets.load_asset(graph_path)
    for node in graph.get_editor_property('nodes'):
        settings=node.get_settings()
        if isinstance(settings,ue.PCGLoadDataAssetSettings):settings.set_editor_property('asset',data)
    if not assets.save_loaded_asset(graph,False):raise RuntimeError('Cannot refresh PCGEx source')
print('ER_INDUSTRIAL_PCG_PREVIEW_COMPLETE '+graph_path)
