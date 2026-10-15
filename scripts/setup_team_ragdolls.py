"""Generate anatomical ragdolls only on project-owned character meshes/assets."""
import unreal as u

assets=u.EditorAssetLibrary
bp=u.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter')
source=u.get_default_object(bp.generated_class())
for team in ['Alpha','Omega']:
    appearance=source.get_editor_property(team.lower()+'_appearance')
    mesh=appearance.mesh
    folder='/Game/EclipseRecoil/Characters/Teams/'+team
    path=folder+'/PA_ER_'+team+'_Ragdoll'
    physics=u.load_asset(path) or assets.duplicate_asset(mesh.get_editor_property('physics_asset').get_path_name(),path)
    if not physics or not u.ERCorpse.build_team_ragdoll_physics(physics,mesh):
        raise RuntimeError('Cannot generate ragdoll '+team)
    for value in [physics,mesh]:
        if not assets.save_loaded_asset(value,only_if_is_dirty=False): raise RuntimeError('Cannot save '+value.get_path_name())
    u.log('ER_RAGDOLL_ASSET_READY team='+team+' physics='+physics.get_path_name())
for path in ['/Game/EclipseRecoil/Core/BP_ERCorpse','/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget']:
    blueprint=u.load_asset(path)
    u.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if path.endswith('BP_ERCorpse'):
        body=u.get_default_object(blueprint.generated_class()).get_editor_property('body')
        assert body.get_class().get_name()=='ERCorpseMeshComponent',body.get_class().get_name()
    if not assets.save_loaded_asset(blueprint,only_if_is_dirty=False): raise RuntimeError('Cannot save '+path)
u.log('ER_RAGDOLL_ASSETS_COMPLETE')
