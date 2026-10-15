"""Explicit one-time outdoor lighting fix; preserve later map/artist edits."""
import unreal as ue

levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
path = '/Game/EclipseRecoil/Maps/L_DustMirror'
if not levels.load_level(path): raise RuntimeError('Missing Dust map')
world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
assets = ue.EditorAssetLibrary
if assets.get_metadata_tag(world, 'ERDustLightingRevision') == '1':
    ue.log('ER_DUST_LIGHTING_PRESERVED')
else:
    for actor in ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors():
        if isinstance(actor, ue.SkyLight):
            light = actor.light_component
            light.set_mobility(ue.ComponentMobility.MOVABLE)
            light.set_editor_property('real_time_capture', True)
            light.set_editor_property('lower_hemisphere_is_black', False)
            light.set_editor_property('intensity', 1.5)
        elif isinstance(actor, ue.DirectionalLight):
            actor.light_component.set_mobility(ue.ComponentMobility.MOVABLE)
            actor.light_component.set_editor_property('atmosphere_sun_light', True)
    assets.set_metadata_tag(world, 'ERDustLightingRevision', '1')
    if not levels.save_current_level(): raise RuntimeError('Cannot save lighting')
    ue.log('ER_DUST_LIGHTING_COMPLETE')
