"""Fit conservative limb limits on the five owned Phase ragdolls once."""
import unreal as ue
assets=ue.EditorAssetLibrary
catalog=ue.get_default_object(assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
for appearance in list(catalog.omega_skins)[5:]:
    mesh=appearance.mesh;physics=mesh.physics_asset
    assert mesh.get_name().startswith('SK_ER_Phase_')
    if assets.get_metadata_tag(physics,'ERPhaseFieldRagdollVersion')=='1':continue
    assert ue.ERCorpse.build_team_ragdoll_physics(physics,mesh,limb_swing=45,leg_swing=35)
    assets.set_metadata_tag(physics,'ERPhaseFieldRagdollVersion','1')
    assert assets.save_loaded_asset(physics,only_if_is_dirty=False)
    assert assets.save_loaded_asset(mesh,only_if_is_dirty=False)
    ue.log('ER_PHASE_FIELD_RAGDOLL_READY mesh='+mesh.get_name())
ue.log('ER_PHASE_FIELD_RAGDOLL_COMPLETE')
