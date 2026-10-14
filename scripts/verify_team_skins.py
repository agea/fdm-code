"""Fresh-process verification of all ten selectable appearances and their corpse copies."""
import unreal as ue
root='/Game/EclipseRecoil/Characters/Teams/'
catalog=None
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']:
    bp=ue.load_asset(path);cdo=ue.get_default_object(bp.generated_class())
    for team in ['Alpha','Omega']:
        choices=cdo.get_editor_property(team.lower()+'_skins')
        assert len(choices)==5,(path,team,'count')
        assert len({x.display_name for x in choices})==5
        assert len({(x.mesh.get_path_name(),tuple(str(a.mesh.get_path_name()) for a in x.attachments)) for x in choices})==5
        for index,x in enumerate(choices):
            assert x.mesh.get_path_name().startswith(root+team+'/Skins/')
            assert x.animation_class and x.corpse_pose
            assert x.mesh.find_socket('HandGrip_R')
            assert x.mesh.physics_asset.get_path_name().startswith(root+team+'/Skins/')
            assert len(x.materials)==len(x.mesh.materials)
            for part in x.attachments:
                assert part.mesh and part.material and part.mesh.get_path_name().startswith(root+'Gear/')
            if catalog:
                original=catalog.get_editor_property(team.lower()+'_skins')[index]
                assert x.mesh==original.mesh and x.materials==original.materials and x.attachments==original.attachments
            ue.log('ER_SKIN_SAVED_PASS class=%s team=%s index=%d'%(path,team,index))
    if catalog is None:catalog=cdo
controller=ue.get_default_object(ue.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERPlayerController'))
assert controller.skin_catalog==ue.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter')
ue.log('ER_SKINS_SAVED_COMPLETE')
