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
    assert len(choices)==10
    for index,appearance in enumerate(choices[5:],5):
        assert appearance.display_name.startswith(hero+' · ')
        assert appearance.mesh.get_name().startswith('SK_ER_'+hero+'_'+team.title())
        assert appearance.mesh.skeleton==appearance.corpse_pose.get_editor_property('skeleton')
        assert appearance.first_person_mesh and appearance.first_person_mesh.skeleton!=appearance.mesh.skeleton
        assert appearance.mesh.physics_asset and .5 < appearance.scale < 1.5
        assert appearance.mesh.find_socket('HandGrip_L') and appearance.mesh.find_socket('HandGrip_R')
        assert ('weapon' if hero=='Belica' else 'teddy_bear_root') in [str(b) for b in appearance.hidden_bones]
        if hero=='Phase':
            assert appearance.copy_physics_pose_for_rendering
            if index==8:
                helmet = list(appearance.attachments)[0]
                assert helmet.mesh.get_path_name().split('.')[0].endswith('/PhaseEngineer/SM_ER_PhaseEngineer_Helmet')
                assert helmet.material.get_path_name().split('.')[0].endswith('/PhaseEngineer/M_ER_PhaseEngineer_Helmet')
                assert helmet.material.get_editor_property('two_sided'), 'Omega 9 helmet shell culling'
            for slot,section in enumerate(appearance.mesh.materials):
                if 'hair_long' in str(section.material_slot_name).lower(): assert slot in appearance.hidden_material_slots
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
