"""Read-only: rerun PCG_RoomsMacroGraph and compare its output with the saved table.

Run through UnrealEditor -run=pythonscript. PCG finishes in the commandlet; inside the
automation tick it did not, so this check lives here and not in a native test.
"""
import unreal as ue
assets=ue.EditorAssetLibrary
folder='/Game/EclipseRecoil/Art/MapTemplates/RoomsPCG/'
# Loaded here: PCG would stream it over frames this pump does not run.
data=assets.load_asset(folder+'DA_RoomsMacroInput')
graph=assets.load_asset(folder+'PCG_RoomsMacroGraph');saved=assets.load_asset(folder+'DA_RoomsMacroTable')
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_Rooms')
if not data or not graph or not saved or not template:raise RuntimeError('Missing Rooms macro assets')
if template.get_editor_property('rooms').get_editor_property('macro_table')!=saved:raise RuntimeError('Recipe does not reference the table')
plain=lambda table:[(v.lane_rooms,v.bake_seed,[(p.x,p.y) for p in v.links]) for v in table.get_editor_property('variants')]
expected=plain(saved)
for run in range(2):
    fresh=ue.new_object(ue.ERRoomsMacroTable)
    count=ue.ERRoomsMacroBake.bake(graph,fresh)
    print(f'ER_ROOMS_MACRO_VERIFY run={run} variants={count}')
    if count!=len(expected) or plain(fresh)!=expected:
        print('ER_ROOMS_MACRO_DIFF expected='+str(expected)+' actual='+str(plain(fresh)))
        raise RuntimeError('Graph output differs from the saved table')
print('ER_ROOMS_MACRO_VERIFIED '+str(len(expected)))
