"""Adjust only mouse sensitivity; preserve mappings and non-scalar modifiers."""
import unreal as ue
action=ue.load_asset('/Game/EclipseRecoil/Input/IA_Look')
if not action: raise RuntimeError('Missing IA_Look')
modifiers=list(action.get_editor_property('modifiers'))
scalars=[m for m in modifiers if isinstance(m,ue.InputModifierScalar)]
if len(scalars)>1: raise RuntimeError('Multiple scalar modifiers: inspect before multiplying sensitivity')
if not scalars:
    scalars=[ue.InputModifierScalar(outer=action)]; modifiers.extend(scalars)
scalars[0].set_editor_property('scalar',ue.Vector(.18,.18,.18))
action.set_editor_property('modifiers',modifiers)
if not ue.EditorAssetLibrary.save_loaded_asset(action,False): raise RuntimeError('Cannot save IA_Look')
ue.log('ER_PRESENTATION_SAVED: mouse scalar 0.18 (restored); other modifiers/mappings preserved')
