"""Append Murdock Alpha/Gadget Omega at indices10..14; retain the original20 skins.

Only create owned derivatives and save the three appearance catalogs. Raw Paragon
packages stay immutable. Completion metadata protects subsequent manual art edits.
"""
from pathlib import Path
import json
import re
import unreal as ue

# Reuse proven rig/material helpers without running earlier migrations.
exec(Path(__file__).with_name('setup_phase_belica.py').read_text().split('\ndef build():')[0])
MARKER = 'ERMurdockGadgetVersion'
VERSION = '1'
murdock = '/Game/ParagonMurdock/Characters/Heroes/Murdock/'
gadget = '/Game/ParagonGadget/Characters/Heroes/Gadget/'
SPECS = {
 'Alpha': ('Murdock', [
  ('Skull', murdock+'Skins/Tier2/Merc/Meshes/Murdock_Merc', (.20,.32,.48), (.24,.26,.25), False),
  ('Infantry', murdock+'Meshes/Murdock', (.18,.30,.46), (.28,.30,.29), False),
  ('Recon', murdock+'Skins/Tier2/Merc/Meshes/Murdock_Merc', (.25,.36,.49), (.36,.34,.29), True),
  ('Patrol', murdock+'Meshes/Murdock', (.30,.40,.53), (.20,.23,.28), False),
  ('Veteran', murdock+'Skins/Tier2/Merc/Meshes/Murdock_Merc', (.14,.25,.41), (.42,.39,.31), False),
 ]),
 'Omega': ('Gadget', [
  ('Mechanic', gadget+'Skins/Tier_1/Gadget_Guerilla/Meshes/GadgetGuerilla', (.67,.29,.10), (.28,.28,.24), False),
  ('Patrol', gadget+'Skins/Tier_1/Gadget_Winter/Meshes/GadgetWInter', (.66,.33,.15), (.24,.25,.24), False),
  ('Recon', gadget+'Meshes/Gadget', (.57,.25,.10), (.20,.22,.21), False),
  ('Field Engineer', gadget+'Skins/Tier_2/ShellShock/Meshes/Gadget_Shellshock', (.62,.29,.12), (.31,.30,.25), False),
  ('Veteran', gadget+'Skins/Tier_1/Gadget_Guerilla/Meshes/GadgetGuerilla', (.76,.37,.14), (.42,.38,.30), False),
 ])
}


def finish(source, folder, team, slot, tint, natural=False, preserve_skin=False):
    # The shared recursive helper retains all texture/instance overrides.
    result = surface(source, folder, team, slot, tint, natural)
    graph = result
    while isinstance(graph, ue.MaterialInstanceConstant):
        graph = graph.parent
    expressions = list(edit.get_material_expressions(graph))
    if preserve_skin:
        for sample in expressions:
            match = re.search(r'(red|green|blue|alpha)\s*=\s*skin', str(sample.get_editor_property('desc')), re.I)
            if not match or not isinstance(sample, ue.MaterialExpressionTextureSample):
                continue
            channel = dict(red='R',green='G',blue='B',alpha='A')[match.group(1).lower()]
            tinted = next(x for x in expressions if str(x.get_editor_property('desc'))=='ER_GroundedColor')
            desat = next(x for x in expressions if str(x.get_editor_property('desc'))=='ER_GroundedDesaturation')
            original = edit.get_inputs_for_material_expression(graph, desat)[0]
            blend = edit.create_material_expression(graph, ue.MaterialExpressionLinearInterpolate)
            blend.set_editor_property('desc', 'ER_PreserveAuthoredSkin')
            assert edit.connect_material_expressions(tinted, '', blend, 'A')
            assert edit.connect_material_expressions(original, 'BaseColor' if isinstance(original,ue.MaterialExpressionBreakMaterialAttributes) else '', blend, 'B')
            assert edit.connect_material_expressions(sample, channel, blend, 'Alpha')
            assert edit.connect_material_property(blend, '', ue.MaterialProperty.MP_BASE_COLOR)
            break
        else:
            # Shellshock is fully covered; its atlas has no exposed-skin mask.
            assert 'Shellshock' in source.get_name(), (slot,source.get_path_name(),'missing authored skin mask')
    graph.set_editor_property('used_with_skeletal_mesh', True)
    edit.recompile_material(graph)
    save(graph)
    return result


def authored_scales(hero, mesh):
    """Manny IK export normalizes auxiliary scales; retain authored facial sizes."""
    ref = mesh.skeleton.get_reference_pose()
    probe = ue.SkeletalMeshComponent()
    probe.set_skeletal_mesh_asset(mesh)
    bones = [b for b in ue.AnimPoseExtensions.get_bone_names(ref) if probe.get_bone_index(b) >= 0]
    scales = {}
    for bone in bones:
        scale = ue.AnimPoseExtensions.get_bone_pose(ref,bone,ue.AnimPoseSpaces.LOCAL).scale3d
        if any(abs(v-1)>.0001 for v in (scale.x,scale.y,scale.z)):
            scales[str(bone)] = scale
    if not scales:
        return
    for path in assets.list_assets(root+'/'+('Alpha' if hero=='Murdock' else 'Omega')+'/'+hero+'/Animations',recursive=True):
        clip=ue.load_asset(path)
        if not isinstance(clip,ue.AnimSequence):continue
        controller=clip.get_editor_property('controller');model=controller.get_model_interface()
        count=model.get_number_of_frames()+1;rate=model.get_frame_rate()
        options=ue.AnimPoseEvaluationOptions();options.optional_skeletal_mesh=mesh;options.should_retarget=False
        poses=[ue.AnimPoseExtensions.get_anim_pose_at_time(clip,frame*rate.denominator/rate.numerator,options) for frame in range(count)]
        controller.open_bracket('Preserve authored auxiliary bone scales',False)
        try:
            for bone,scale in scales.items():
                keys=[ue.AnimPoseExtensions.get_bone_pose(pose,bone,ue.AnimPoseSpaces.LOCAL) for pose in poses]
                if bone.lower() not in [str(n).lower() for n in model.get_bone_track_names()]:
                    assert controller.add_bone_curve(bone,False)
                assert controller.set_bone_track_keys(bone,[k.translation for k in keys],[k.rotation for k in keys],[scale]*count,False)
        finally:controller.close_bracket(False)
        save(clip)


def build():
    bp=load(BP_PATHS[0]);catalog=ue.get_default_object(bp.generated_class())
    if assets.get_metadata_tag(bp,MARKER)==VERSION:
        print('ER_MURDOCK_GADGET_ALREADY_APPLIED')
        return
    manny=load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
    make_rig(manny, 'IK_ER_Manny')
    fps_rig=load(root+'/Alpha/FirstPerson/SK_ER_Twinblast_Alpha_Officer_Forearms')
    neutral=load(root+'/Gear/M_ER_HiddenSection')
    choices={};report=[]
    for team,(hero,specs) in SPECS.items():
        existing=list(catalog.get_editor_property(team.lower()+'_skins'))
        assert len(existing)==10,(team,len(existing))
        skeleton,anim,idle=retarget(team,hero,load(specs[0][1]))
        first=None
        for label,source_path,dominant,muted,unmasked in specs:
            key=hero+label.replace(' ','')
            folder=root+'/'+team+'/Skins/'+key;assets.make_directory(folder)
            source=load(source_path)
            mesh=copy(source,folder+'/SK_ER_'+hero+'_'+team+'_'+key);mesh.skeleton=skeleton
            physics_source=source.physics_asset or load('/Game/Paragon'+hero+'/Characters/Heroes/'+hero+'/Meshes/'+hero+'_Extents')
            physics=copy(physics_source,folder+'/PA_ER_'+key+'_Ragdoll')
            assert ue.ERCorpse.build_team_ragdoll_physics(physics,mesh,limb_swing=55,leg_swing=45)
            assert ue.ERCharacter.copy_training_grip_sockets(mesh,manny)
            save(physics)
            probe=ue.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(mesh)
            bones={str(b) for b in ue.AnimPoseExtensions.get_bone_names(skeleton.get_reference_pose()) if probe.get_bone_index(b)>=0}
            hide_bones=([b for b in ['bot_uparm','gun','shotgun_base','helmet_sight','hip_pack_a','hip_pack_b','hip_pack_c','hip_pack_d','pack_hose_in','pack_hose_out'] if b in bones] if hero=='Murdock' else
                        [b for b in ['backpack_base','robo_arm_bottom'] if b in bones])
            hidden=[];materials=[];slots=list(mesh.materials)
            for i,slot in enumerate(slots):
                name=str(slot.material_slot_name);low=name.lower()
                hidden_slot=(any(w in low for w in ['weapon','glass']) or (unmasked and 'merc_mask' in low)) if hero=='Murdock' else any(w in low for w in ['machine','turkeybot'])
                if hidden_slot:
                    hidden.append(i);material=neutral
                else:
                    natural=any(w in low for w in ['face','eye','mouth','tear','teeth','hair','skin_head']) or 'merc_mask' in low
                    tint=(1,1,1) if natural else dominant if any(w in low for w in ['body','merc_skin','arms']) and 'lower' not in low else muted
                    material=finish(slot.material_interface,folder,team,name,tint,natural,hero=='Gadget' and 'upperbody' in low)
                materials.append(material);slot.set_editor_property('material_interface',material)
            mesh.set_editor_property('materials',slots);save(mesh)
            if first is None:first=mesh
            ref=skeleton.get_reference_pose()
            pos=lambda b:ue.AnimPoseExtensions.get_bone_pose(ref,b,ue.AnimPoseSpaces.WORLD).translation
            feet=min(pos('foot_l').z,pos('foot_r').z)-6;scale=185/(pos('head').z+12-feet)
            fps=copy(source,root+'/'+team+'/FirstPerson/'+mesh.get_name()+'_Forearms')
            assert ue.ERCharacter.build_first_person_forearms(fps,mesh,fps_rig,hidden,hide_bones)
            save(fps)
            appearance=ue.ERTeamAppearance()
            for attr,value in dict(display_name=hero+' · '+label,mesh=mesh,animation_class=anim,corpse_pose=idle,first_person_mesh=fps,materials=materials,hidden_bones=hide_bones,hidden_material_slots=hidden,attachments=[],scale=scale,feet_offset=-92.5-feet*scale,copy_physics_pose_for_rendering=True).items():appearance.set_editor_property(attr,value)
            existing.append(appearance)
            report.append(dict(team=team,index=len(existing)-1,name=appearance.display_name,source=source_path,mesh=mesh.get_path_name(),hidden_slots=hidden,hidden_bones=hide_bones))
            print('ER_NEW_SKIN_READY',team,len(existing)-1,key)
        authored_scales(hero,first)
        assert ue.ERCharacter.prepare_team_skeleton(skeleton,first);save(skeleton)
        choices[team]=existing
    # Publish atomically only once all ten appearances and FPS meshes exist.
    for path in BP_PATHS:
        value=load(path);cdo=ue.get_default_object(value.generated_class())
        for team,skins in choices.items():cdo.set_editor_property(team.lower()+'_skins',skins)
        ue.BlueprintEditorLibrary.compile_blueprint(value)
        assets.set_metadata_tag(value,MARKER,VERSION);save(value)
    p=Path(ue.Paths.project_dir())/'Saved/Testing/murdock-gadget-setup.json';p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(report,indent=2))
    print('ER_MURDOCK_GADGET_COMPLETE alpha=15 omega=15')

build()

import runpy
runpy.run_path(str(Path(__file__).with_name("setup_murdock_gadget_palms.py")))
