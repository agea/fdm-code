"""Create FDM-owned presentation assets for training targets and patrol bots.

Preserves Fab originals, player appearance, template animation and edited maps.
Run through Unreal's Python commandlet after compiling the Editor module.
"""
import unreal as ue

assets = ue.EditorAssetLibrary
tools = ue.AssetToolsHelpers.get_asset_tools()
folder = '/Game/EclipseRecoil/Characters/Mercenary'
assets.make_directory(folder)


def load(path):
    result = ue.load_asset(path)
    if not result:
        raise RuntimeError('Missing asset: ' + path)
    return result


def save(obj):
    if not assets.save_loaded_asset(obj, only_if_is_dirty=False):
        raise RuntimeError('Cannot save: ' + obj.get_path_name())


def duplicate(source, destination):
    return ue.load_asset(destination) or assets.duplicate_asset(source.get_path_name(), destination)


original = load('/Game/Mercenary_Male_WaveforceStudio/Mesh/SK_MER_ME_Outfit01')
manny = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
original_skeleton = original.get_editor_property('skeleton')
manny_skeleton = manny.get_editor_property('skeleton')
source_pose = manny_skeleton.get_reference_pose()
target_pose = original_skeleton.get_reference_pose()
source_names = set(str(n) for n in ue.AnimPoseExtensions.get_bone_names(source_pose))
target_names = set(str(n) for n in ue.AnimPoseExtensions.get_bone_names(target_pose))
# Explicitly validate the shared animation hierarchy before declaring compatibility.
for bone in sorted(source_names & target_names):
    source_parent = str(manny.get_bone_parent(bone))
    target_parent = str(original.get_bone_parent(bone))
    if source_parent != target_parent:
        raise RuntimeError('Different bone parent for %s: %s / %s' % (bone, source_parent, target_parent))
for bone in ['root','pelvis','spine_01','spine_02','spine_03','spine_04','spine_05','upperarm_l','lowerarm_l','hand_l','upperarm_r','lowerarm_r','hand_r','thigh_l','calf_l','foot_l','thigh_r','calf_r','foot_r']:
    if bone not in target_names:
        raise RuntimeError('Missing required rifle bone: ' + bone)
ue.log('ER_MERC_HIERARCHY shared=%d source=%d target=%d' % (len(source_names & target_names), len(source_names), len(target_names)))

skeleton = duplicate(original_skeleton, folder + '/SKEL_ER_Mercenary_Target')
skeleton.add_compatible_skeleton(manny_skeleton)
skeleton.add_compatible_skeleton(original_skeleton)
save(skeleton)
mesh = duplicate(original, folder + '/SK_ER_Mercenary_Target')
mesh.skeleton = skeleton  # Invoke Unreal's BlueprintSetter (editor property is read-only).
# The existing rifle attaches to HandGrip_R. Add the template grips only to our
# mesh copy, using their authored transforms under the matching hand bones.
if not ue.ERCharacter.copy_training_grip_sockets(mesh, manny):
    raise RuntimeError('Cannot copy the rifle grip sockets')
save(mesh)
sub = ue.get_editor_subsystem(ue.SkeletalMeshEditorSubsystem)
ue.log('ER_MERC_MESH lods=%d vertices=%s material_slots=%d' % (sub.get_lod_count(mesh), [sub.get_num_verts(mesh, i) for i in range(sub.get_lod_count(mesh))], len(mesh.get_editor_property('materials'))))

path = '/Game/EclipseRecoil/Core/BP_ERTrainingTarget'
bp = ue.load_asset(path)
if not bp:
    factory = ue.BlueprintFactory()
    factory.set_editor_property('parent_class', assets.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
    bp = tools.create_asset('BP_ERTrainingTarget', '/Game/EclipseRecoil/Core', ue.Blueprint, factory)
ue.BlueprintEditorLibrary.compile_blueprint(bp)
defaults = ue.get_default_object(bp.generated_class())
component = defaults.get_editor_property('mesh')
component.set_skeletal_mesh_asset(mesh)
component.set_anim_instance_class(assets.load_blueprint_class('/Game/Variant_Shooter/Anims/ABP_TP_Rifle'))
# Fit the ~2m imported outfit to the unchanged 185cm capsule.
bounds = mesh.get_imported_bounds()
scale = 185.0 / (bounds.box_extent.z * 2.0)
component.set_editor_property('relative_scale3d', ue.Vector(scale, scale, scale))
component.set_editor_property('relative_location', ue.Vector(0, 0, -92.5))
component.set_editor_property('relative_rotation', ue.Rotator(pitch=0, yaw=-90, roll=0))
# Own material copies: preserve skin/face, colour the clothing and hair.
palettes = {'Alpha': ue.LinearColor(0.025, 0.24, 0.70, 1),
            'Omega': ue.LinearColor(0.75, 0.085, 0.012, 1)}
for team, colour in palettes.items():
    team_materials = []
    for slot in mesh.get_editor_property('materials'):
        source = slot.material_interface
        name = source.get_name()
        if any(part in name for part in ['Outer01', 'Pant01', 'Gloves01', 'Shoes01', 'Hair01']):
            material = duplicate(source, folder + '/MI_ER_' + team + '_' + name.removeprefix('MI_MER_ME_'))
            tint = colour if any(part in name for part in ['Outer01', 'Pant01', 'Hair01']) else ue.LinearColor(0.025, 0.03, 0.04, 1)
            ue.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material, 'TintColor_01', tint)
            ue.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material, 'TintColor_02', ue.LinearColor(0.025, 0.03, 0.04, 1))
            ue.MaterialEditingLibrary.update_material_instance(material)
            save(material)
            team_materials.append(material)
        else:
            team_materials.append(source)
    defaults.set_editor_property(team.lower() + '_body_materials', team_materials)
ue.BlueprintEditorLibrary.compile_blueprint(bp)
save(bp)
mode = load('/Game/EclipseRecoil/Core/BP_ERGameMode')
ue.get_default_object(mode.generated_class()).set_editor_property('training_target_pawn_class', bp.generated_class())
save(mode)
ue.log('ER_MERC_TARGET_READY class=%s mesh=%s scale=%.4f socket=%s' % (bp.generated_class().get_path_name(), mesh.get_path_name(), scale, str(mesh.find_socket('HandGrip_R'))))
