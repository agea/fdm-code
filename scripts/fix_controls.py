"""Add a non-modifier crouch alternative while preserving the current bindings."""
import unreal as ue
context = ue.load_asset('/Game/EclipseRecoil/Input/IMC_Gameplay')
action = ue.load_asset('/Game/EclipseRecoil/Input/IA_Crouch')
if not context or not action:
    raise RuntimeError('Missing input assets')
data = context.get_editor_property('default_key_mappings')
mappings = list(data.get_editor_property('mappings'))
if not any(m.get_editor_property('action') == action and str(m.get_editor_property('key').get_editor_property('key_name')) == 'C' for m in mappings):
    mapping = ue.EnhancedActionKeyMapping()
    mapping.set_editor_property('action',action)
    binding = ue.Key()
    binding.set_editor_property('key_name','C')
    mapping.set_editor_property('key',binding)
    mappings.append(mapping)
    data.set_editor_property('mappings',mappings)
    context.set_editor_property('default_key_mappings',data)
if not ue.EditorAssetLibrary.save_loaded_asset(context, only_if_is_dirty=False):
    raise RuntimeError('Cannot save crouch input alternative')
ue.log('ER_CONTROLS_UPDATED: crouch bound to Ctrl and C; other bindings preserved')
