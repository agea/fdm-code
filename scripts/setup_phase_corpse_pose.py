"""Enable the explicit cosmetic physical-pose renderer on owned Phase skins."""
import unreal as ue
assets=ue.EditorAssetLibrary
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']:
    bp=ue.load_asset(path)
    if assets.get_metadata_tag(bp,'ERPhaseCorpsePoseVersion')=='1':continue
    defaults=ue.get_default_object(bp.generated_class())
    choices=list(defaults.omega_skins)
    assert len(choices)>=10
    for skin in choices[5:10]:
        assert skin.mesh.get_name().startswith('SK_ER_Phase_')
        skin.set_editor_property('copy_physics_pose_for_rendering',True)
    defaults.set_editor_property('omega_skins',choices)
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assets.set_metadata_tag(bp,'ERPhaseCorpsePoseVersion','1')
    assert assets.save_loaded_asset(bp,only_if_is_dirty=False)
ue.log('ER_PHASE_CORPSE_POSE_SETUP_COMPLETE')
