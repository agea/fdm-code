"""Add a safe patrol loop to the existing depot without rebuilding its geometry."""
import unreal as ue
levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
if not levels.load_level('/Game/EclipseRecoil/Maps/L_Depot'):
    raise RuntimeError('Missing depot level')
points = [(-2400,0),(-2400,-1300),(-1250,-1600),(0,-1600),(1250,-1600),(2400,-1300),
          (2400,0),(2400,1300),(1250,1600),(0,1600),(-1250,1600),(-2400,1300)]
existing = {a.get_editor_property('route_index'): a for a in actors.get_all_level_actors() if isinstance(a, ue.ERBotPatrolPoint)}
for index, (x,y) in enumerate(points):
    if index in existing:
        continue
    point = actors.spawn_actor_from_class(ue.ERBotPatrolPoint, ue.Vector(x,y,100))
    if not point:
        raise RuntimeError('Cannot create patrol point')
    point.set_editor_property('route_index',index)
    point.set_actor_label('BotPatrol_%02d' % index)
if not levels.save_current_level():
    raise RuntimeError('Cannot save patrol route')
character = ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter')
rifle_anim = ue.EditorAssetLibrary.load_blueprint_class('/Game/Variant_Shooter/Anims/ABP_TP_Rifle')
if not character or not rifle_anim:
    raise RuntimeError('Missing character or template rifle locomotion')
ue.get_default_object(character.generated_class()).get_editor_property('mesh').set_anim_instance_class(rifle_anim)
if not ue.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False):
    raise RuntimeError('Cannot save world rifle animation')
ue.log('ER_BOT_ROUTE_SAVED: 12 patrol points in existing depot; world rifle locomotion assigned')
