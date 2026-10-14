"""Repair the prototype's existing spawn/sun rotations without rebuilding the map."""
import unreal as ue

levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
if not levels.load_level("/Game/EclipseRecoil/Maps/L_Depot"):
    raise RuntimeError("Cannot load L_Depot")
for actor in actors.get_all_level_actors():
    if isinstance(actor, ue.ERTeamPlayerStart):
        yaw = 0 if actor.get_editor_property("team") == ue.ERTeam.ALPHA else 180
        actor.set_actor_rotation(ue.Rotator(pitch=0, yaw=yaw, roll=0), False)
    elif actor.get_actor_label() == "DepotSun":
        actor.set_actor_rotation(ue.Rotator(pitch=-50, yaw=-25, roll=0), False)
if not levels.save_current_level():
    raise RuntimeError("Cannot save L_Depot")
ue.log("ER_ROTATIONS_REPAIRED: Alpha/Omega face the arena; sun uses explicit axes.")
