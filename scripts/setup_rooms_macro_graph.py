"""Create PCG_RoomsMacroGraph and bake its PCGEx connections into DA_RoomsMacroTable.

Run through UnrealEditor -run=pythonscript after compiling. The graph is authoring
data: matches read only the baked table. Rerunning refreshes input and table.
"""
import hashlib,json,zipfile
from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir()).resolve()
assets=ue.EditorAssetLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
folder='/Game/EclipseRecoil/Art/MapTemplates/RoomsPCG'
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_Rooms')
if not template:raise RuntimeError('Rooms recipe missing')
if not assets.does_directory_exist(folder):assets.make_directory(folder)
def owned(name,cls):
    path=folder+'/'+name
    if assets.does_asset_exist(path):return assets.load_asset(path)
    return tools.create_asset(name,folder,cls,ue.PCGGraphFactory() if cls is ue.PCGGraph else ue.DataAssetFactory())
data=ue.ERRoomsMacroBake.find_or_create_input(folder+'/DA_RoomsMacroInput')
if not data or not ue.ERRoomsMacroBake.fill_input(template,data,8):raise RuntimeError('Cannot sample Rooms plans')
graph_exists=assets.does_asset_exist(folder+'/PCG_RoomsMacroGraph')
graph=owned('PCG_RoomsMacroGraph',ue.PCGGraph)
if not graph_exists:
    loader,settings=graph.add_node_of_type(ue.PCGLoadDataAssetSettings)
    settings.set_editor_property('asset',data)
    settings.post_edit_change() if hasattr(settings,'post_edit_change') else None
    print('ER_ROOMS_MACRO_PINS '+str([x.properties.label for x in loader.get_editor_property('output_pins')]))
    connect,_=graph.add_node_of_type(ue.PCGExConnectPointsSettings)
    probe,_=graph.add_node_of_type(ue.PCGExProbeKNNProviderSettings)
    refine,refine_settings=graph.add_node_of_type(ue.PCGExRefineEdgesSettings)
    refine_settings.set_editor_property('refinement',ue.new_object(ue.PCGExEdgeRefineGabriel,refine_settings))
    output=graph.get_output_node()
    for link in ((loader,'Slots',connect,'In'),(probe,'Probe',connect,'Probes'),(connect,'Vtx',refine,'Vtx'),(connect,'Edges',refine,'Edges'),(refine,'Vtx',output,'Out'),(refine,'Edges',output,'Out')):
        if not graph.add_edge(*link):raise RuntimeError('Cannot connect '+str(link[1:]))
table=owned('DA_RoomsMacroTable',ue.ERRoomsMacroTable)
count=ue.ERRoomsMacroBake.bake(graph,table)
print('ER_ROOMS_MACRO_VARIANTS '+str(count))
if count<1:raise RuntimeError('PCGEx graph produced no usable variant')
for asset in (data,graph,table):
    if not assets.save_loaded_asset(asset):raise RuntimeError('Cannot save '+asset.get_name())
# The recipe keeps only a reference: the descriptor still decides whether it is read.
recipe=root/'Assets/maps/Content/EclipseRecoil/Art/MapTemplates/DA_Rooms.uasset'
backup=root/'Saved/RoomsMacroBackup/before-rooms-macro.zip';backup.parent.mkdir(parents=True,exist_ok=True)
if not backup.exists():
    with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
        z.write(recipe,recipe.name);z.writestr('hashes.json',json.dumps({recipe.name:hashlib.sha256(recipe.read_bytes()).hexdigest()}))
with zipfile.ZipFile(backup) as z:
    for name,digest in json.loads(z.read('hashes.json')).items():
        if hashlib.sha256(z.read(name)).hexdigest()!=digest:raise RuntimeError('Invalid Rooms rollback')
rooms=template.get_editor_property('rooms');rooms.set_editor_property('macro_table',table);template.set_editor_property('rooms',rooms)
if not assets.save_loaded_asset(template,False):raise RuntimeError('Cannot save Rooms recipe')
variants=[dict(lane_rooms=v.lane_rooms,bake_seed=v.bake_seed,links=[[p.x,p.y] for p in v.links]) for v in table.get_editor_property('variants')]
out=Path(ue.Paths.project_saved_dir())/'Testing/rooms-macro-table.json';out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(variants,indent=1))
print('ER_ROOMS_MACRO_COMPLETE '+str(count))
