"""Create five owned silhouettes per team, using existing rigs and collision-free field gear.
Run in Unreal after compiling. Originals and existing retargeted animations are preserved.
"""
from pathlib import Path
import unreal as ue

# Share idempotent asset/material utilities without running the original retarget job.
exec(Path(__file__).with_name('setup_team_characters.py').read_text().split('manny = load(')[0])
manny = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
character = ue.get_default_object(assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
gear_folder = root + '/Gear'
assets.make_directory(gear_folder)
gear_meshes = {}
for name, helmet in [('SM_ER_FieldHelmet', True), ('SM_ER_FieldPack', False)]:
    mesh = copy(load('/Engine/BasicShapes/Cube'), gear_folder+'/'+name)
    assert ue.ERCorpse.build_field_gear(mesh, helmet), name
    save(mesh)
    gear_meshes['helmet' if helmet else 'pack'] = mesh

tb = '/Game/ParagonTwinblast/Characters/Heroes/TwinBlast/'
dr = '/Game/ParagonDrongo/Characters/Heroes/Drongo/'
specs = {
 'Alpha': [
  ('Ufficiale · cappotto', 'Officer', tb+'Skins/Tier2/ActionMovie/Meshes/TwinBlast_ActionHero', [], (.22,.40,.72)),
  ('Assalto · uniforme tattica', 'Assault', tb+'Skins/Tier2/ShadowOps/Meshes/TwinBlast_ShadowOps', [], (.16,.31,.55)),
  ('Pattuglia · giacca tattica', 'Patrol', tb+'Meshes/TwinBlast', [], (.27,.46,.70)),
  ('Ricognitore · giacca usurata e zaino', 'Scout', tb+'Skins/Tier1/CS_Twinblast_Apocalyptic/TwinBlast_Wasteland', ['pack'], (.24,.40,.53)),
  ('Supporto · zaino tattico', 'SupportNatural', tb+'Meshes/TwinBlast', ['pack'], (.18,.34,.65))],
 'Omega': [
  ('Predone · gilet', 'Raider', dr+'Meshes/Drongo_GDC', [], (.90,.40,.12)),
  ('Sciacallo · giacca pesante', 'Scavenger', dr+'Skins/Scavenger/Meshes/Drongo_Scavenger', [], (.77,.31,.08)),
  ('Incursore · casco', 'Breacher', dr+'Meshes/Drongo_GDC', ['helmet'], (.78,.39,.18)),
  ('Contrabbandiere · giacca e zaino', 'Smuggler', dr+'Skins/Scavenger/Meshes/Drongo_Scavenger', ['pack'], (.65,.27,.09)),
  ('Miliziano · casco e zaino', 'Militia', dr+'Meshes/Drongo_GDC', ['helmet','pack'], (.92,.45,.16))]
}
skins = {}
def surface_tint(team, index, original, dominant):
    name = original.get_name().lower()
    if team == 'Alpha':
        if any(part in name for part in ('coat', 'torso', 'upper', 'cloth', 'jacket', 'shirt')):
            return dominant
        if 'low' in name or 'pants' in name:
            return [(.48,.50,.54),(.55,.53,.47),(.40,.44,.49),(.43,.45,.43),(.35,.40,.46)][index]
        if 'arms' in name: return (.40,.46,.52)
        # Keep every remaining visible cloth surface in the team's cool range;
        # trousers, arms, straps and field gear stay neutral for contrast.
        return tuple(channel * .82 for channel in dominant)
    if 'upper' in name and 'cloth' in name: return dominant
    if 'lower' in name and 'cloth' in name:
        return [(.59,.55,.48),(.65,.43,.24),(.50,.48,.43),(.57,.42,.29),(.61,.50,.35)][index]
    return (.43,.32,.23)

def skin_material(original, folder, team, slot_name):
    if isinstance(original, ue.MaterialInstanceConstant):
        parent = original.parent
        while isinstance(parent, ue.MaterialInstanceConstant): parent = parent.parent
        grounded = grounded_material(parent, folder, team, slot_name)
        instance = copy(original, folder+'/MI_ER_'+team+'_'+slot_name)
        edit.set_material_instance_parent(instance, grounded)
        edit.update_material_instance(instance)
        save(instance)
        return instance
    return grounded_material(original, folder, team, slot_name)

for team, choices in specs.items():
    base = character.get_editor_property(team.lower()+'_appearance')
    skins[team] = []
    for index, (label, key, source_path, parts, tint) in enumerate(choices):
        folder = root+'/'+team+'/Skins/'+key
        assets.make_directory(folder)
        source = load(source_path)
        mesh_name = 'SK_ER_'+('Twinblast_Alpha' if team=='Alpha' else 'Drongo_Omega')+'_'+key
        mesh = copy(source, folder+'/'+mesh_name)
        mesh.skeleton = base.mesh.skeleton
        physics = copy(base.mesh.physics_asset, folder+'/PA_ER_'+key+'_Ragdoll')
        assert ue.ERCorpse.build_team_ragdoll_physics(physics, mesh), key
        assert ue.ERCharacter.copy_training_grip_sockets(mesh, manny), key
        save(physics); save(mesh)
        reference = source.skeleton.get_reference_pose()
        names = ue.AnimPoseExtensions.get_bone_names(reference)
        probe = ue.SkeletalMeshComponent(); probe.set_skeletal_mesh_asset(mesh)
        bones = {str(b) for b in names if probe.get_bone_index(b)>=0}
        hide_bones = list(base.hidden_bones)
        if team=='Alpha': hide_bones += ['q_weapon_l','q_weapon_r','grenade','ult_root','weapon_l','weapon_r']
        hide_words = ['pistol','grenade','sim'] if team=='Alpha' else ['weapons','bones','teddybear']
        materials, hidden = [], []
        for slot_index, slot in enumerate(mesh.materials):
            slot_name, original = str(slot.material_slot_name), slot.material_interface
            if any(w in slot_name.lower() for w in hide_words): hidden.append(slot_index)
            natural = original is None or any(w in original.get_name().lower() for w in ['skin','hair','eye','teeth','mouth','tear'])
            # Mixed face/shirt graphs retain their natural colour. Other visible cloth
            # carries the team palette; remove hero glow in all generated surfaces.
            natural = natural or 'head_sh' in original.get_name().lower() if original else True
            mat = original if natural else skin_material(original, folder, team, slot_name)
            if mat and mat.get_path_name().startswith(folder):
                surface_color = surface_tint(team,index,original,tint)
                graph = mat.parent if isinstance(mat,ue.MaterialInstanceConstant) else mat
                for expression in edit.get_material_expressions(graph):
                    desc = str(expression.get_editor_property('desc'))
                    if desc=='ER_GroundedTint': expression.set_editor_property('default_value',ue.LinearColor(*surface_color,1))
                    if desc=='ER_DesaturationAmount': expression.set_editor_property('r',.62)
                edit.recompile_material(graph); save(graph)
                if isinstance(mat,ue.MaterialInstanceConstant):
                    edit.set_material_instance_vector_parameter_value(mat,'ER_GroundedTint',ue.LinearColor(*surface_color,1))
                    edit.update_material_instance(mat);save(mat)
            materials.append(mat)
        pose = lambda bone: ue.AnimPoseExtensions.get_bone_pose(reference,bone,ue.AnimPoseSpaces.WORLD).translation
        foot_z = min(pose('foot_l').z,pose('foot_r').z)-6
        scale = 185/(pose('head').z+12-foot_z)
        gear_mat_name = 'M_ER_'+key+'_Gear'
        gear_mat = ue.load_asset(folder+'/'+gear_mat_name) or tools.create_asset(gear_mat_name,folder,ue.Material,ue.MaterialFactoryNew())
        edit.delete_all_material_expressions(gear_mat)
        color=edit.create_material_expression(gear_mat,ue.MaterialExpressionConstant3Vector)
        color.set_editor_property('constant',ue.LinearColor(.055,.070,.095,1) if team=='Alpha' else ue.LinearColor(.09,.068,.045,1))
        edit.connect_material_property(color,'',ue.MaterialProperty.MP_BASE_COLOR)
        rough=edit.create_material_expression(gear_mat,ue.MaterialExpressionConstant);rough.set_editor_property('r',.88)
        edit.connect_material_property(rough,'',ue.MaterialProperty.MP_ROUGHNESS)
        edit.recompile_material(gear_mat);save(gear_mat)
        attachments=[]
        for part in parts:
            bone = 'head' if part=='helmet' else 'spine_03'
            # Head-bone pivots vary between the TwinBlast meshes. Patrol needs a
            # higher but flatter shell to clear its hair; Support sits lower.
            if part=='helmet' and team=='Alpha':
                offset = ue.Vector(0,0,9 if key=='Patrol' else 4)
            else:
                offset = ue.Vector(0,0,4) if part=='helmet' else ue.Vector(0,-21,-13)
            # Drongo already has a bulky mask and head gear; use a lower-profile
            # shell so the extra helmet does not swallow the face silhouette.
            size = ue.Vector(.90,.90,.62) if team=='Omega' and part=='helmet' else ue.Vector(1.02,1.02,.72) if part=='helmet' else ue.Vector(1,1,1)
            attachments.append(ue.ERCorpse.make_field_attachment(gear_meshes[part],gear_mat,mesh,bone,offset,size))
        appearance=ue.ERTeamAppearance()
        first_person = ue.load_asset(root+'/'+team+'/FirstPerson/'+mesh_name+'_Forearms')
        for attr,value in dict(display_name=label, mesh=mesh, animation_class=base.animation_class,corpse_pose=base.corpse_pose,
            first_person_mesh=first_person,
            materials=materials,hidden_bones=sorted(set(str(b) for b in hide_bones if str(b) in bones)),hidden_material_slots=hidden,
            attachments=attachments,scale=scale,feet_offset=-92.5-foot_z*scale).items(): appearance.set_editor_property(attr,value)
        skins[team].append(appearance)
        ue.log('ER_SKIN_READY team=%s index=%d mesh=%s gear=%s'%(team,index,mesh.get_name(),parts))

for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']:
    bp=load(path);cdo=ue.get_default_object(bp.generated_class())
    for team, choices in skins.items():cdo.set_editor_property(team.lower()+'_skins',choices)
    ue.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
bp=load('/Game/EclipseRecoil/Core/BP_ERPlayerController')
ue.get_default_object(bp.generated_class()).set_editor_property('skin_catalog', assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
ue.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
ue.log('ER_SKINS_SETUP_COMPLETE')
