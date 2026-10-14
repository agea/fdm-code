"""Restore the previous mannequin FPS view for a user screenshot comparison.

Preserves the mercenary world meshes, generated FPS alternatives and gameplay.
"""
import unreal as ue

assets = ue.EditorAssetLibrary
mesh = ue.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
idle = ue.load_asset('/Game/EclipseRecoil/Characters/A_ER_Rifle_HipPose')
fire = ue.load_asset('/Game/EclipseRecoil/Characters/A_ER_Rifle_Fire')
if not all([mesh, idle, fire]):
    raise RuntimeError('Missing original mannequin FPS assets')
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget']:
    bp = ue.load_asset(path)
    if not bp:
        raise RuntimeError('Missing ' + path)
    defaults = ue.get_default_object(bp.generated_class())
    body = defaults.get_editor_property('first_person_body')
    body.set_skeletal_mesh_asset(mesh)
    body.set_editor_property('override_materials', [])
    body.set_editor_property('relative_scale3d', ue.Vector(1, 1, 1))
    body.set_editor_property('relative_location', ue.Vector(-10, 0, -152))
    body.set_editor_property('relative_rotation', ue.Rotator(pitch=0, yaw=-90, roll=0))
    body.set_editor_property('animation_mode', ue.AnimationMode.ANIMATION_SINGLE_NODE)
    anim = body.get_editor_property('animation_data')
    anim.set_editor_property('anim_to_play', idle)
    anim.set_editor_property('saved_looping', True)
    body.set_editor_property('animation_data', anim)
    defaults.set_editor_property('first_person_fire_animation', fire)
    defaults.set_editor_property('first_person_visible_material_slots', [])
    defaults.set_editor_property('alpha_first_person_materials', [])
    defaults.set_editor_property('omega_first_person_materials', [])
    rifle = defaults.get_editor_property('first_person_rifle')
    rifle.set_editor_property('absolute_scale', False)
    rifle.set_editor_property('relative_scale3d', ue.Vector(1, 1, 1))
    rifle.set_editor_property('relative_location', ue.Vector(0, 0, 0))
    rifle.set_editor_property('relative_rotation', ue.Rotator(pitch=0, yaw=0, roll=0))
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    if not assets.save_loaded_asset(bp, only_if_is_dirty=False):
        raise RuntimeError('Cannot save ' + path)
    ue.log('ER_FPS_MANNEQUIN_RESTORED class=%s mesh=%s' % (bp.generated_class().get_path_name(), mesh.get_path_name()))
