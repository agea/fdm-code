"""Read saved team assets in a fresh Unreal process; fail on missing bindings."""
import unreal as u

root = '/Game/EclipseRecoil/Characters/Teams/'
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget', '/Game/EclipseRecoil/Core/BP_ERCorpse']:
    bp = u.load_asset(path)
    defaults = u.get_default_object(bp.generated_class())
    for team, name in [('Alpha','SK_ER_Twinblast_Alpha_Soldier'),('Omega','SK_ER_Drongo_Omega')]:
        appearance = defaults.get_editor_property(team.lower() + '_appearance')
        assert appearance.mesh.get_path_name().startswith(root + team + '/' + name), (path,team,'mesh')
        assert appearance.animation_class and appearance.corpse_pose, (path,team,'animation')
        assert appearance.mesh.get_editor_property('physics_asset').get_path_name().startswith(root+team+'/PA_ER_'+team+'_Ragdoll'), (path,team,'physics')
        assert appearance.mesh.find_socket('HandGrip_R'), (path,team,'grip')
        assert len(appearance.materials) == len(appearance.mesh.get_editor_property('materials')), (path,team,'slots')
        assert len(appearance.hidden_material_slots) == 2, (path,team,'hidden weapons')
        for material in appearance.materials:
            if not material or not material.get_path_name().startswith(root): continue
            assert not material.get_editor_property('use_material_attributes'), material.get_path_name()
            emissive = u.MaterialEditingLibrary.get_material_property_input_node(material, u.MaterialProperty.MP_EMISSIVE_COLOR)
            assert emissive and emissive.get_editor_property('r') == 0, material.get_path_name()
        u.log('ER_TEAM_SAVED_PASS class=%s team=%s mesh=%s anim=%s' % (path,team,appearance.mesh.get_name(),appearance.animation_class.get_name()))
u.log('ER_TEAM_SAVED_COMPLETE')
