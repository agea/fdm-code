"""Append five Belica Alpha and five Phase Omega appearances via Unreal.

Run after building RespawnAndRageEditor. The original ten stable indices and vendor assets
are preserved. A completion marker prevents overwriting subsequent artist edits.
"""
from pathlib import Path
import unreal as ue

exec(Path(__file__).with_name('setup_team_characters.py').read_text().split('manny = load(')[0])
VERSION = '1'
MARKER = 'ERPhaseBelicaVersion'
BP_PATHS = ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget', '/Game/EclipseRecoil/Core/BP_ERCorpse']
belica = '/Game/ParagonLtBelica/Characters/Heroes/Belica/'
phase = '/Game/ParagonPhase/Characters/Heroes/Phase/'
# Five distinct silhouettes per character: different supplied clothing/armor,
# with project field gear where appropriate. No fantasy Witch skin is used.
SPECS = {
 'Alpha': ('Belica', belica+'Meshes/Belica', belica+'Meshes/Belica_Extents', [
  ('Belica · Infantry', 'BelicaInfantry', belica+'Meshes/Belica', [], (.22,.39,.65), (.31,.34,.37)),
  ('Belica · Arctic Patrol', 'BelicaPolar', belica+'Skins/PolarStrike/Meshes/Belica_PolarStrike', [], (.31,.46,.65), (.48,.49,.48)),
  ('Belica · NBC Defense', 'BelicaCBRN', belica+'Skins/Biohazard/Meshes/Belica_Biohazard', [], (.17,.30,.50), (.33,.35,.32)),
  ('Belica · Armored', 'BelicaArmor', belica+'Skins/HeavyArmor/Meshes/Belica_HeavyArmor', [], (.23,.37,.57), (.28,.31,.35)),
  ('Belica · Support', 'BelicaSupport', belica+'Meshes/Belica', ['pack'], (.18,.32,.57), (.42,.41,.36)),
 ]),
 'Omega': ('Phase', phase+'Meshes/Phase_GDC', phase+'Meshes/Phase_Extents', [
  ('Phase · Recon', 'PhaseRecon', phase+'Meshes/Phase_GDC', [], (.76,.34,.13), (.31,.29,.25)),
  ('Phase · Assault', 'PhaseAssault', phase+'Skins/Tier_1_5/Kitty/Meshes/Phase_Kitty_GDC', [], (.66,.27,.10), (.28,.29,.26)),
  ('Phase · Patrol', 'PhasePatrol', phase+'Skins/Tier_1_5/Pandapack/Meshes/Phase_Pandapack_GDC', [], (.82,.40,.17), (.38,.35,.29)),
  ('Phase · Engineer', 'PhaseEngineer', phase+'Meshes/Phase_GDC', ['helmet'], (.62,.29,.12), (.33,.32,.29)),
  ('Phase · Support', 'PhaseSupport', phase+'Skins/Tier_1_5/Kitty/Meshes/Phase_Kitty_GDC', ['pack'], (.74,.35,.15), (.39,.36,.29)),
 ])
}


def surface(source, folder, team, slot, tint, natural=False, hair=False):
    """Preserve every inherited instance override, texture, normal and opacity."""
    if isinstance(source, ue.MaterialInstanceConstant):
        parent = surface(source.parent, folder, team, slot+'_Parent', tint, natural, hair)
        result = copy(source, folder+'/MI_ER_'+slot)
        edit.set_material_instance_parent(result, parent)
        edit.set_material_instance_vector_parameter_value(result, 'ER_GroundedTint', ue.LinearColor(*tint,1))
        edit.update_material_instance(result)
    else:
        result = grounded_material(source, folder, team, slot)
        if result == source:
            raise RuntimeError('Unsupported material type: '+source.get_path_name())
        for expr in edit.get_material_expressions(result):
            desc = str(expr.get_editor_property('desc'))
            if desc == 'ER_GroundedTint': expr.set_editor_property('default_value',ue.LinearColor(*tint,1))
            if desc == 'ER_DesaturationAmount': expr.set_editor_property('r', .85 if hair else 0 if natural else .85)
            if desc == 'ER_MatteRoughness' and natural: expr.set_editor_property('r',.55)
        zero=edit.create_material_expression(result,ue.MaterialExpressionConstant)
        zero.set_editor_property('desc','ER_NoHeroVertexOffset');zero.set_editor_property('r',0)
        edit.connect_material_property(zero,'',ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
        edit.recompile_material(result)
    save(result)
    return result


def retarget(team, hero, original):
    folder = root+'/'+team+'/'+hero
    assets.make_directory(folder)
    skeleton = copy(original.skeleton, folder+'/SKEL_ER_'+hero)
    skeleton.add_compatible_skeleton(original.skeleton)
    save(skeleton)
    mesh = copy(original, folder+'/SK_ER_'+hero+'_Retarget')
    mesh.skeleton = skeleton
    save(mesh)
    output = folder+'/Animations'
    anim_path = output+'/ER_'+hero+'_ABP_TP_Rifle'
    idle_path = output+'/ER_'+hero+'_MF_Rifle_Idle_ADS'
    if not assets.does_asset_exist(anim_path) or not assets.does_asset_exist(idle_path):
        manny = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
        source_rig = load(root+'/IK_ER_Manny')
        target_rig = make_rig(mesh,'IK_ER_'+hero)
        name = 'RTG_ER_Manny_'+hero
        rt = ue.load_asset(root+'/'+name) or tools.create_asset(name,root,ue.IKRetargeter,ue.IKRetargetFactory())
        ctrl = ue.IKRetargeterController.get_controller(rt)
        ctrl.set_ik_rig(ue.RetargetSourceOrTarget.SOURCE,source_rig)
        ctrl.set_ik_rig(ue.RetargetSourceOrTarget.TARGET,target_rig)
        if not ctrl.get_num_retarget_ops(): ctrl.add_default_ops()
        ctrl.auto_map_chains(ue.AutoMapChainType.FUZZY,True)
        ctrl.auto_align_all_bones(ue.RetargetSourceOrTarget.TARGET)
        save(rt)
        inputs = ue.IKRetargetBatchOperationInputs()
        for key,value in dict(assets_to_retarget=[assets.find_asset_data('/Game/Variant_Shooter/Anims/ABP_TP_Rifle'),assets.find_asset_data('/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS')],source_mesh=manny,target_mesh=mesh,ik_retarget_asset=rt,prefix='ER_'+hero+'_',target_path=output,include_referenced_assets=True,overwrite_existing_files=False).items():
            inputs.set_editor_property(key,value)
        ue.IKRetargetBatchOperation.run_batch_retarget(inputs)
        for path in assets.list_assets(output,recursive=True,include_folder=False):
            value=load(path)
            if isinstance(value,ue.AnimBlueprint): ue.BlueprintEditorLibrary.compile_blueprint(value)
            save(value)
    anim=assets.load_blueprint_class(anim_path)
    if not anim: raise RuntimeError('Missing retargeted blueprint: '+hero)
    return skeleton,anim,load(idle_path)


def build():
    bp=load(BP_PATHS[0]); catalog=ue.get_default_object(bp.generated_class())
    if assets.get_metadata_tag(bp,MARKER)==VERSION:
        ue.log('ER_PHASE_BELICA_ALREADY_APPLIED artist_edits_preserved=1')
        return
    manny=load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
    fps_rig=load(root+'/Alpha/FirstPerson/SK_ER_Twinblast_Alpha_Officer_Forearms')
    choices={}
    for team,(hero,source_path,physics_path,specs) in SPECS.items():
        existing=list(catalog.get_editor_property(team.lower()+'_skins'))
        if len(existing)!=5: raise RuntimeError('Expected five original skins before migration: '+team)
        skeleton,anim,idle=retarget(team,hero,load(source_path))
        for label,key,source_path,parts,dominant,neutral in specs:
            folder=root+'/'+team+'/Skins/'+key
            assets.make_directory(folder)
            source=load(source_path)
            mesh=copy(source,folder+'/SK_ER_'+hero+'_'+team+'_'+key)
            mesh.skeleton=skeleton
            physics=copy(load(physics_path),folder+'/PA_ER_'+key+'_Ragdoll')
            assert ue.ERCorpse.build_team_ragdoll_physics(physics,mesh,limb_swing=45 if hero=='Phase' else 65,leg_swing=35 if hero=='Phase' else 65),key
            assert ue.ERCharacter.copy_training_grip_sockets(mesh,manny),key
            save(physics);save(mesh)
            probe=ue.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(mesh)
            names=[str(b) for b in ue.AnimPoseExtensions.get_bone_names(skeleton.get_reference_pose()) if probe.get_bone_index(b)>=0]
            hidden_bones=([b for b in ['weapon','weapon_l','pistol','hologram','datapad','binoculars'] if b in names] if hero=='Belica' else
                [b for b in ['weapon_l','weapon_r','teddy_bear_root','hip_bag','earring_l','pendant','bracelet1','bracelet2','hip_chain_l_01','hip_chain_r_01','skirt_chain_01'] if b in names])
            materials=[];hidden=[]
            for index,slot in enumerate(mesh.materials):
                slot_name=str(slot.material_slot_name); name=slot_name.lower(); original=slot.material_interface
                if any(w in name for w in (['guns','hologram','datapad'] if hero=='Belica' else ['backpack','pandapack','mask'])): hidden.append(index)
                hair='hair' in name and 'holder' not in name
                natural=any(w in name for w in ['body_head','head','mouth','eye','tear']) and 'upper' not in name
                tint=(.30,.23,.17) if hair else (1,1,1) if natural else dominant if any(w in name for w in ['upperbody','jacket']) else neutral
                if original: materials.append(surface(original,folder,team,slot_name,tint,natural,hair))
                else: materials.append(original)
            ref=skeleton.get_reference_pose()
            def pos(b): return ue.AnimPoseExtensions.get_bone_pose(ref,b,ue.AnimPoseSpaces.WORLD).translation
            feet=min(pos('foot_l').z,pos('foot_r').z)-6
            scale=185/(pos('head').z+12-feet)
            attachments=[]
            for part in parts:
                gear=load(root+'/Gear/SM_ER_Field'+('Helmet' if part=='helmet' else 'Pack'))
                gear_mat=load(root+('/Alpha/Skins/SupportNatural/M_ER_SupportNatural_Gear' if team=='Alpha' else '/Omega/Skins/Militia/M_ER_Militia_Gear'))
                attachments.append(ue.ERCorpse.make_field_attachment(gear,gear_mat,mesh,'head' if part=='helmet' else 'spine_03',ue.Vector(0,0,5) if part=='helmet' else ue.Vector(0,-18,-10),ue.Vector(.86,.86,.63) if part=='helmet' else ue.Vector(.85,.85,.85)))
            fps_folder=root+'/'+team+'/FirstPerson';assets.make_directory(fps_folder)
            fps=copy(source,fps_folder+'/'+mesh.get_name()+'_Forearms')
            assert ue.ERCharacter.build_first_person_forearms(fps,mesh,fps_rig,hidden,hidden_bones),key
            save(fps)
            appearance=ue.ERTeamAppearance()
            appearance.set_editor_property('copy_physics_pose_for_rendering',hero=='Phase')
            for attr,value in dict(display_name=label,mesh=mesh,animation_class=anim,corpse_pose=idle,first_person_mesh=fps,materials=materials,hidden_bones=hidden_bones,hidden_material_slots=hidden,attachments=attachments,scale=scale,feet_offset=-92.5-feet*scale).items(): appearance.set_editor_property(attr,value)
            existing.append(appearance)
            ue.log('ER_PHASE_BELICA_READY team=%s index=%d key=%s hidden=%s'%(team,len(existing)-1,key,hidden))
        choices[team]=existing
    # Publish only after all ten assets, poses, materials and FPS meshes exist.
    for path in BP_PATHS:
        bp=load(path);defaults=ue.get_default_object(bp.generated_class())
        for team,values in choices.items(): defaults.set_editor_property(team.lower()+'_skins',values)
        ue.BlueprintEditorLibrary.compile_blueprint(bp)
        assets.set_metadata_tag(bp,MARKER,VERSION)
        save(bp)
    ue.log('ER_PHASE_BELICA_COMPLETE alpha=10 omega=10 added=10')

build()
# Explicit one-time field finish; its own marker protects later artist edits.
import runpy
runpy.run_path(str(Path(__file__).with_name('refine_phase_field_appearance.py')))

runpy.run_path(str(Path(__file__).with_name('setup_phase_corpse_pose.py')))
runpy.run_path(str(Path(__file__).with_name('fit_phase_ragdolls.py')))
runpy.run_path(str(Path(__file__).with_name('fix_phase_engineer_helmet.py')))
runpy.run_path(str(Path(__file__).with_name('normalize_character_names.py')))
runpy.run_path(str(Path(__file__).with_name('setup_phase_recon_long_hair.py')))
