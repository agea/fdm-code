"""Build owned Twinblast/Drongo appearances; never edit the Paragon packages.

Run in Unreal Editor after compiling RespawnAndRageEditor. Retarget the complete template
rifle animation blueprint with its referenced locomotion assets for each rig.
"""
import unreal as ue

assets = ue.EditorAssetLibrary
tools = ue.AssetToolsHelpers.get_asset_tools()
edit = ue.MaterialEditingLibrary
root = '/Game/EclipseRecoil/Characters/Teams'


def load(path):
    value = ue.load_asset(path)
    if not value:
        raise RuntimeError('Missing asset: ' + path)
    return value


def save(value):
    if not assets.save_loaded_asset(value, only_if_is_dirty=False):
        raise RuntimeError('Cannot save: ' + value.get_path_name())


def copy(source, path):
    value = ue.load_asset(path) or assets.duplicate_asset(source.get_path_name(), path)
    if not value:
        raise RuntimeError('Cannot duplicate: ' + path)
    return value


def make_rig(mesh, name):
    path = root + '/' + name
    rig = ue.load_asset(path) or tools.create_asset(name, root, ue.IKRigDefinition, ue.IKRigDefinitionFactory())
    controller = ue.IKRigController.get_controller(rig)
    controller.set_skeletal_mesh(mesh)
    if not controller.apply_auto_generated_retarget_definition():
        raise RuntimeError('Auto retarget definition failed: ' + mesh.get_path_name())
    if not controller.apply_auto_fbik():
        raise RuntimeError('Auto FBIK failed: ' + mesh.get_path_name())
    save(rig)
    return rig


# Keep authored normal maps and surface texture; remove animated/emissive hero
# effects and make clothing/equipment matte. Skin material graphs stay intact.
def grounded_material(source, folder, team, slot_name):
    name = 'M_ER_' + team + '_' + slot_name + '_Grounded'
    if source.get_class() != ue.Material.static_class():
        return source
    material = copy(source, folder + '/' + name)
    # Duplicating again is unnecessary; replace our named terminal nodes in place.
    nodes = {str(x.get_editor_property('desc')): x for x in edit.get_material_expressions(material)}
    def node(cls, key):
        item = nodes.get(key) or edit.create_material_expression(material, cls, 1300, 0)
        item.set_editor_property('desc', key)
        return item
    # Paragon connects material attributes; ordinary output pins are ignored
    # until that graph is split. Preserve textures, normals and opacity from
    # the authored graph while replacing its visible surface properties.
    if material.get_editor_property('use_material_attributes'):
        attributes = edit.get_material_property_input_node(material, ue.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
        attributes_output = edit.get_material_property_input_node_output_name(material, ue.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
        split = node(ue.MaterialExpressionBreakMaterialAttributes, 'ER_OriginalSurface')
        if not edit.connect_material_expressions(attributes, attributes_output, split, 'Attr'):
            raise RuntimeError('Cannot split authored material attributes')
        material.set_editor_property('use_material_attributes', False)
        for pin, prop in [('BaseColor',ue.MaterialProperty.MP_BASE_COLOR), ('Normal',ue.MaterialProperty.MP_NORMAL),
                          ('Specular',ue.MaterialProperty.MP_SPECULAR), ('AmbientOcclusion',ue.MaterialProperty.MP_AMBIENT_OCCLUSION),
                          ('Opacity',ue.MaterialProperty.MP_OPACITY), ('OpacityMask',ue.MaterialProperty.MP_OPACITY_MASK)]:
            if not edit.connect_material_property(split, pin, prop):
                raise RuntimeError('Cannot preserve material attribute: ' + pin)
    base = edit.get_material_property_input_node(material, ue.MaterialProperty.MP_BASE_COLOR)
    output = edit.get_material_property_input_node_output_name(material, ue.MaterialProperty.MP_BASE_COLOR)
    tint = node(ue.MaterialExpressionVectorParameter, 'ER_GroundedTint')
    tint.set_editor_property('parameter_name', 'ER_GroundedTint')
    tint.set_editor_property('default_value', ue.LinearColor(.55, .60, .43, 1) if team == 'Alpha' else ue.LinearColor(.58, .43, .31, 1))
    multiply = node(ue.MaterialExpressionMultiply, 'ER_GroundedColor')
    if base and base != multiply:
        desat = node(ue.MaterialExpressionDesaturation, 'ER_GroundedDesaturation')
        fraction = node(ue.MaterialExpressionConstant, 'ER_DesaturationAmount')
        fraction.set_editor_property('r', .65)
        edit.connect_material_expressions(fraction, '', desat, 'Fraction')
        if not edit.connect_material_expressions(base, output, desat, ''):
            raise RuntimeError('Cannot connect authored base colour')
        edit.connect_material_expressions(desat, '', multiply, 'A')
    edit.connect_material_expressions(tint, '', multiply, 'B')
    edit.connect_material_property(multiply, '', ue.MaterialProperty.MP_BASE_COLOR)
    for prop, key, value in [(ue.MaterialProperty.MP_EMISSIVE_COLOR, 'ER_NoGlow', 0),
                             (ue.MaterialProperty.MP_METALLIC, 'ER_MatteMetal', .05),
                             (ue.MaterialProperty.MP_ROUGHNESS, 'ER_MatteRoughness', .82)]:
        constant = node(ue.MaterialExpressionConstant, key)
        constant.set_editor_property('r', value)
        edit.connect_material_property(constant, '', prop)
    edit.recompile_material(material)
    save(material)
    return material


manny = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
for source in ['/Game/ParagonTwinblast/Characters/Heroes/TwinBlast/Skins/Tier2/ActionMovie/Meshes/TwinBlast_ActionHero',
               '/Game/ParagonDrongo/Characters/Heroes/Drongo/Meshes/Drongo_GDC']:
    if not assets.does_asset_exist(source):
        raise RuntimeError('Historical rebuild inputs were archived after character approval. Restore the private character cleanup archive before rebuilding; current saved skins need no setup.')
source_rig = None if globals().get('ER_SKIP_RETARGET') else make_rig(manny, 'IK_ER_Manny')
source_abp = load('/Game/Variant_Shooter/Anims/ABP_TP_Rifle')
source_idle = load('/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS')
config = {
    'Alpha': ('/Game/ParagonTwinblast/Characters/Heroes/TwinBlast/Skins/Tier2/ActionMovie/Meshes/TwinBlast_ActionHero',
              '/Game/ParagonTwinblast/Characters/Heroes/TwinBlast/Meshes/TwinBlast_Physics',
              ['weapon_l','weapon_r','ult_root','q_weapon_l','q_weapon_r','grenade'], ['Pistol','Grenade','SIM']),
    'Omega': ('/Game/ParagonDrongo/Characters/Heroes/Drongo/Meshes/Drongo_GDC',
              '/Game/ParagonDrongo/Characters/Heroes/Drongo/Meshes/Drongo_Extents',
              ['pistol','weapon_r','boomerang','Bazooka','canister_pos','grenade_pos'], ['Weapons']),
}
appearances = {}
for team, (source_path, physics_path, hidden_bones, hidden_slot_words) in config.items():
    folder = root + '/' + team
    assets.make_directory(folder)
    original = load(source_path)
    skeleton = copy(original.get_editor_property('skeleton'), folder + '/SKEL_ER_' + team)
    skeleton.add_compatible_skeleton(original.get_editor_property('skeleton'))
    save(skeleton)
    mesh = copy(original, folder + '/SK_ER_' + ('Twinblast_Alpha_Soldier' if team == 'Alpha' else 'Drongo_Omega'))
    mesh.skeleton = skeleton
    physics = copy(load(physics_path), folder + '/PA_ER_' + team + '_Ragdoll')
    if not ue.ERCorpse.build_team_ragdoll_physics(physics, mesh):
        raise RuntimeError('Cannot generate anatomical ragdoll: ' + team)
    save(physics)
    if not ue.ERCharacter.copy_training_grip_sockets(mesh, manny):
        raise RuntimeError('Cannot add rifle grip sockets: ' + team)
    save(mesh)
    output_folder = folder + '/Animations'
    if not globals().get('ER_SKIP_RETARGET'):
        rig = make_rig(mesh, 'IK_ER_' + team)
        rt_name = 'RTG_ER_Manny_' + team
        rt = ue.load_asset(root + '/' + rt_name) or tools.create_asset(rt_name, root, ue.IKRetargeter, ue.IKRetargetFactory())
        controller = ue.IKRetargeterController.get_controller(rt)
        controller.set_ik_rig(ue.RetargetSourceOrTarget.SOURCE, source_rig)
        controller.set_ik_rig(ue.RetargetSourceOrTarget.TARGET, rig)
        if not controller.get_num_retarget_ops():
            controller.add_default_ops()
        controller.auto_map_chains(ue.AutoMapChainType.FUZZY, True)
        controller.auto_align_all_bones(ue.RetargetSourceOrTarget.TARGET)
        save(rt)
        output_folder = folder + '/Animations'
        inputs = ue.IKRetargetBatchOperationInputs()
        for key, value in dict(assets_to_retarget=[assets.find_asset_data(source_abp.get_path_name()), assets.find_asset_data(source_idle.get_path_name())],
                               source_mesh=manny, target_mesh=mesh, ik_retarget_asset=rt, prefix='ER_' + team + '_',
                               target_path=output_folder, include_referenced_assets=True, overwrite_existing_files=True).items():
            inputs.set_editor_property(key, value)
        results = ue.IKRetargetBatchOperation.run_batch_retarget(inputs)
        # UE 5.8's batch return list omits duplicated animation blueprints. Save
        # the owned output directory explicitly, including all parent blueprints.
        for path in assets.list_assets(output_folder, recursive=True, include_folder=False):
            value = load(path)
            if isinstance(value, ue.AnimBlueprint): ue.BlueprintEditorLibrary.compile_blueprint(value)
            save(value)
        if not assets.save_directory(output_folder, only_if_is_dirty=False, recursive=True):
            raise RuntimeError('Cannot save retargeted outputs: ' + team)
    anim_class = assets.load_blueprint_class(output_folder + '/ER_' + team + '_ABP_TP_Rifle')
    idle = load(output_folder + '/ER_' + team + '_MF_Rifle_Idle_ADS')
    if not anim_class:
        raise RuntimeError('Missing retargeted animation class: ' + team)
    materials = []
    hidden_slots = []
    for index, slot in enumerate(mesh.get_editor_property('materials')):
        name = str(slot.material_slot_name)
        if any(word in name for word in hidden_slot_words): hidden_slots.append(index)
        source = slot.material_interface
        keep_skin = any(word in source.get_name().lower() for word in ['skin','hair','eye','teeth','mouth']) if source else True
        materials.append(source if keep_skin else grounded_material(source, folder, team, name))
    # Fit actual feet-to-head height rather than bounding boxes that include the
    # original bazooka/weapon attachments. Head centre plus ~12 cm is the crown.
    reference = skeleton.get_reference_pose()
    pose = lambda bone: ue.AnimPoseExtensions.get_bone_pose(reference, bone, ue.AnimPoseSpaces.WORLD).translation
    foot_z = min(pose('foot_l').z, pose('foot_r').z) - 6.0
    height = pose('head').z + 12.0 - foot_z
    scale = 185.0 / height
    # Paragon's shared skeleton also contains bones used only by other skins.
    # Filter against this mesh, so ActionHero never requests absent q weapons.
    probe = ue.SkeletalMeshComponent()
    probe.set_skeletal_mesh_asset(mesh)
    bones = {str(b) for b in ue.AnimPoseExtensions.get_bone_names(reference)
             if probe.get_bone_index(b) >= 0}
    appearance = ue.ERTeamAppearance()
    for key, value in dict(mesh=mesh, animation_class=anim_class, corpse_pose=idle, materials=materials,
                           hidden_bones=[b for b in hidden_bones if b in bones], hidden_material_slots=hidden_slots,
                           scale=scale, feet_offset=-92.5-foot_z*scale).items():
        appearance.set_editor_property(key, value)
    appearances[team] = appearance
    ue.log('ER_TEAM_ASSET_READY team=%s mesh=%s scale=%.4f hidden_slots=%s hidden_bones=%s animation=%s' %
           (team, mesh.get_path_name(), scale, hidden_slots, appearance.hidden_bones, anim_class.get_path_name()))

for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget', '/Game/EclipseRecoil/Core/BP_ERCorpse']:
    bp = load(path)
    cdo = ue.get_default_object(bp.generated_class())
    for team, appearance in appearances.items(): cdo.set_editor_property(team.lower() + '_appearance', appearance)
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    save(bp)

ue.log('ER_TEAM_CHARACTERS_COMPLETE')
