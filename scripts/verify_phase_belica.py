"""Verify saved Phase/Belica catalog, rig, material and presentation bindings."""
import runpy
from pathlib import Path
import unreal as ue

scripts = Path(__file__).resolve().parent
runpy.run_path(str(scripts/'verify_team_skins.py'))
runpy.run_path(str(scripts/'verify_skin_first_person_forearms.py'))
catalog = ue.get_default_object(ue.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
edit = ue.MaterialEditingLibrary
for team,hero in [('alpha','Belica'),('omega','Phase')]:
    choices = list(catalog.get_editor_property(team+'_skins'))
    assert len(choices)>=10
    for index,appearance in enumerate(choices[5:10],5):
        assert appearance.display_name.startswith(hero+' · ')
        assert appearance.mesh.get_name().startswith('SK_ER_'+hero+'_'+team.title())
        assert appearance.mesh.skeleton==appearance.corpse_pose.get_editor_property('skeleton')
        assert appearance.first_person_mesh and appearance.first_person_mesh.skeleton!=appearance.mesh.skeleton
        assert appearance.mesh.physics_asset and .5 < appearance.scale < 1.5
        assert appearance.mesh.find_socket('HandGrip_L') and appearance.mesh.find_socket('HandGrip_R')
        assert ('weapon' if hero=='Belica' else 'teddy_bear_root') in [str(b) for b in appearance.hidden_bones]
        if hero=='Belica':
            options = ue.AnimPoseEvaluationOptions()
            options.optional_skeletal_mesh = appearance.mesh
            options.should_retarget = False
            reference = appearance.mesh.skeleton.get_reference_pose()
            expected = ue.AnimPoseExtensions.get_bone_pose(reference, 'faceAttach', ue.AnimPoseSpaces.LOCAL).scale3d
            for time in (0, appearance.corpse_pose.get_play_length()*.5):
                pose = ue.AnimPoseExtensions.get_anim_pose_at_time(appearance.corpse_pose, time, options)
                actual = ue.AnimPoseExtensions.get_bone_pose(pose, 'faceAttach', ue.AnimPoseSpaces.LOCAL).scale3d
                assert max(abs(actual.x-expected.x), abs(actual.y-expected.y), abs(actual.z-expected.z)) < .0001, (index, 'facial scale')
        if hero=='Phase':
            assert appearance.copy_physics_pose_for_rendering
            if index==8:
                parts=list(appearance.attachments)
                assert len(parts)==1 and parts[0].mesh.get_name()=='SM_ER_PhaseEngineer_ArmoredHood', 'Approved Phase Engineer armored hood'
                assert str(parts[0].bone)=='head'
                for slot, section in enumerate(appearance.mesh.materials):
                    if 'hair_short' in str(section.material_slot_name).lower():
                        assert slot in appearance.hidden_material_slots, 'Keep stray hair cards hidden'
            for slot,section in enumerate(appearance.mesh.materials):
                if any(part in str(section.material_slot_name).lower() for part in ('hair_long','hair_short')):
                    assert slot in appearance.hidden_material_slots
        dominant = neutral = 0
        for slot,material in zip(appearance.mesh.materials,appearance.materials):
            assert material and material.get_path_name().startswith('/Game/EclipseRecoil/Characters/Teams/')
            while isinstance(material,ue.MaterialInstanceConstant): material=material.parent
            emissive=edit.get_material_property_input_node(material,ue.MaterialProperty.MP_EMISSIVE_COLOR)
            assert emissive and emissive.get_editor_property('r')==0,(hero,index,slot.material_slot_name,'glow')
            if any(w in str(slot.material_slot_name).lower() for w in ['upperbody','jacket']): dominant+=1
            else: neutral+=1
        assert dominant and neutral
        ue.log('ER_PHASE_BELICA_SAVED_PASS team=%s skin=%d mesh=%s'%(team,index,appearance.mesh.get_name()))
ue.log('ER_PHASE_BELICA_SAVED_COMPLETE new_skins=10 catalog=20')
