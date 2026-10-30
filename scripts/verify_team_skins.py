"""Fresh-process verification of all selectable appearances and their corpse copies."""
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
        assert len(choices)==len(names[team]),(path,team,'count')
        assert len({x.display_name for x in choices})==len(names[team])
        assert len({(x.mesh.get_path_name(),tuple(str(a.mesh.get_path_name()) for a in x.attachments)) for x in choices})==len(names[team])
        for index,x in enumerate(choices):
            assert x.display_name==names[team][index],(path,team,index,x.display_name)
            assert x.mesh.get_path_name().startswith(root+team+'/Skins/')
            assert x.animation_class and x.corpse_pose
            assert x.mesh.find_socket('HandGrip_R')
            assert x.mesh.physics_asset.get_path_name().startswith(root+team+'/Skins/')
            assert len(x.materials)==len(x.mesh.materials)
            for part in x.attachments:
                skin_folder = x.mesh.get_path_name().rsplit('/',1)[0]+'/'
                assert part.mesh and part.mesh.get_path_name().startswith((root+'Gear/',skin_folder))
                # These accessories retain their separate authored material slots.
                assert part.material or (part.mesh.get_name() in ('SM_ER_DrongoMask','SM_ER_PhaseEngineer_ArmoredHood','SM_ER_MurdockRecon_HumanHead','SM_ER_WraithAssault_TwinblastHead','SM_ER_WraithOfficer_TwinblastHead') and all(s.material_interface for s in part.mesh.get_editor_property('static_materials')))
            if catalog:
                original=catalog.get_editor_property(team.lower()+'_skins')[index]
                assert x.mesh==original.mesh and x.materials==original.materials and x.attachments==original.attachments
            ue.log('ER_SKIN_SAVED_PASS class=%s team=%s index=%d'%(path,team,index))
    if catalog is None:catalog=cdo
controller=ue.get_default_object(ue.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERPlayerController'))
assert controller.skin_catalog==ue.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter')
recon=catalog.alpha_skins[3]
palette={'M01_cloth':('M_ER_Recon_NeutralJacket',(.52,.50,.47)),
         'M04_LowerBody':('M_ER_Recon_BlueTrousers',(.10,.30,.72))}
for slot,section in enumerate(recon.mesh.materials):
    key=str(section.material_slot_name)
    if key not in palette:continue
    name,expected=palette[key];material=recon.materials[slot]
    assert material.get_name()==name,(key,material.get_path_name())
    assert ue.EditorAssetLibrary.get_metadata_tag(material,'ERPalettePolicy')=='ReconNeutralJacketBlueTrousers'
    tint=next(n.get_editor_property('default_value') for n in ue.MaterialEditingLibrary.get_material_expressions(material)
              if str(n.get_editor_property('desc'))=='ER_GroundedTint')
    assert all(abs(a-b)<1e-5 for a,b in zip((tint.r,tint.g,tint.b),expected)),(key,tint)
ue.log('ER_RECON_PALETTE_SAVED_PASS skin=3 gallery_variant=4 catalogs=3')
ue.log('ER_SKINS_SAVED_COMPLETE')
