"""Fit a complete outward-facing helmet shell to Omega 9, once.

The shared open shell otherwise culls its front surface, leaving a visible
rear fragment/rim around Phase's head. Preserve other skins and artist edits.
"""
import unreal as ue

assets = ue.EditorAssetLibrary
paths = ['/Game/EclipseRecoil/Core/BP_ERCharacter',
         '/Game/EclipseRecoil/Core/BP_ERTrainingTarget',
         '/Game/EclipseRecoil/Core/BP_ERCorpse']
marker = 'ERPhaseEngineerHelmetVersion'
material_path = '/Game/EclipseRecoil/Characters/Teams/Omega/Skins/PhaseEngineer/M_ER_PhaseEngineer_Helmet'
mesh_path = '/Game/EclipseRecoil/Characters/Teams/Omega/Skins/PhaseEngineer/SM_ER_PhaseEngineer_Helmet'

for path in paths:
    bp = ue.load_asset(path)
    if assets.get_metadata_tag(bp, marker) == '1':
        continue
    defaults = ue.get_default_object(bp.generated_class())
    choices = list(defaults.omega_skins)
    assert len(choices) == 10
    appearance = choices[8]
    assert 'PhaseEngineer' in appearance.mesh.get_path_name()
    parts = list(appearance.attachments)
    assert len(parts) == 1 and str(parts[0].bone) == 'head'
    material = (ue.load_asset(material_path) if assets.does_asset_exist(material_path)
                else assets.duplicate_asset(parts[0].material.get_path_name().split('.')[0], material_path))
    assert isinstance(material, ue.Material)
    material.set_editor_property('two_sided', True)
    ue.MaterialEditingLibrary.recompile_material(material)
    assert assets.save_loaded_asset(material, only_if_is_dirty=False)
    mesh = (ue.load_asset(mesh_path) if assets.does_asset_exist(mesh_path)
            else assets.duplicate_asset(parts[0].mesh.get_path_name().split('.')[0], mesh_path))
    assert ue.ERCorpse.build_field_gear(mesh, True, outward_facing=True)
    assert assets.save_loaded_asset(mesh, only_if_is_dirty=False)
    parts[0] = ue.ERCorpse.make_field_attachment(mesh, material, appearance.mesh, 'head',
                                               ue.Vector(0, 0, 11), ue.Vector(.9, .9, .65))
    appearance.set_editor_property('attachments', parts)
    choices[8] = appearance
    defaults.set_editor_property('omega_skins', choices)
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assets.set_metadata_tag(bp, marker, '1')
    assert assets.save_loaded_asset(bp, only_if_is_dirty=False)
ue.log('ER_PHASE_ENGINEER_HELMET_COMPLETE')
