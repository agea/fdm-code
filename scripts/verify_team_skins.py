"""Fresh-process verification of all twenty selectable appearances and their corpse copies."""
import unreal as ue
import runpy
from pathlib import Path
names=runpy.run_path(str(Path(__file__).with_name('character_catalog.py')))['NAMES']
root='/Game/EclipseRecoil/Characters/Teams/'
catalog=None
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']:
    bp=ue.load_asset(path);cdo=ue.get_default_object(bp.generated_class())
    for team in ['Alpha','Omega']:
        choices=cdo.get_editor_property(team.lower()+'_skins')
        assert len(choices)==10,(path,team,'count')
        assert len({x.display_name for x in choices})==10
        assert len({(x.mesh.get_path_name(),tuple(str(a.mesh.get_path_name()) for a in x.attachments)) for x in choices})==10
        for index,x in enumerate(choices):
            assert x.display_name==names[team][index],(path,team,index,x.display_name)
            assert x.mesh.get_path_name().startswith(root+team+'/Skins/')
            assert x.animation_class and x.corpse_pose
            assert x.mesh.find_socket('HandGrip_R')
            assert x.mesh.physics_asset.get_path_name().startswith(root+team+'/Skins/')
            assert len(x.materials)==len(x.mesh.materials)
            for part in x.attachments:
                skin_folder = x.mesh.get_path_name().rsplit('/',1)[0]+'/'
                assert part.mesh and part.material and part.mesh.get_path_name().startswith((root+'Gear/',skin_folder))
            if catalog:
                original=catalog.get_editor_property(team.lower()+'_skins')[index]
                assert x.mesh==original.mesh and x.materials==original.materials and x.attachments==original.attachments
            ue.log('ER_SKIN_SAVED_PASS class=%s team=%s index=%d'%(path,team,index))
    if catalog is None:catalog=cdo
controller=ue.get_default_object(ue.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERPlayerController'))
assert controller.skin_catalog==ue.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter')
ue.log('ER_SKINS_SAVED_COMPLETE')
