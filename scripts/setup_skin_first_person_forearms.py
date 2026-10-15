"""Extract skin forearms/hands and rebind them to the existing FPS rig in Unreal.

Keep the old camera frame, grip sockets, weapon idle/fire animations and UVs.
The world meshes and vendor packages are never edited.
"""
import unreal as ue

assets = ue.EditorAssetLibrary
rig = ue.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/FirstPerson/SK_ER_Twinblast_Alpha_Officer_Forearms')
if not rig or rig.skeleton.get_path_name() != '/Game/EclipseRecoil/Characters/FirstPerson/SKEL_ER_FirstPerson.SKEL_ER_FirstPerson':
    raise RuntimeError('Run detach_mercenary.py once to establish the independent FPS rig')
player = ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter')
catalog = ue.get_default_object(player.generated_class())
choices = {}

for team in ['alpha', 'omega']:
    appearances = list(catalog.get_editor_property(team + '_skins'))
    for index, appearance in enumerate(appearances):
        source = appearance.mesh
        folder = '/Game/EclipseRecoil/Characters/Teams/' + team.title() + '/FirstPerson'
        assets.make_directory(folder)
        path = folder + '/' + source.get_name() + '_Forearms'
        mesh = ue.load_asset(path) or assets.duplicate_asset(source.get_path_name(), path)
        if not ue.ERCharacter.build_first_person_forearms(mesh, source, rig,
                list(appearance.hidden_material_slots), list(appearance.hidden_bones)):
            raise RuntimeError('Forearm extraction failed: ' + path)
        if mesh.skeleton != rig.skeleton:
            raise RuntimeError('FPS skeleton differs from the old viewmodel: ' + path)
        if not assets.save_loaded_asset(mesh, only_if_is_dirty=False):
            raise RuntimeError('Cannot save forearms: ' + path)
        appearance.set_editor_property('first_person_mesh', mesh)
        ue.log('ER_FPS_SKIN_READY team=%s index=%d mesh=%s source=%s' %
               (team, index, mesh.get_path_name(), source.get_path_name()))
    choices[team] = appearances

for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget']:
    bp = ue.load_asset(path)
    defaults = ue.get_default_object(bp.generated_class())
    for team, appearances in choices.items():
        defaults.set_editor_property(team + '_skins', appearances)
        fallback = defaults.get_editor_property(team + '_appearance')
        fallback.set_editor_property('first_person_mesh', appearances[0].first_person_mesh)
        defaults.set_editor_property(team + '_appearance', fallback)
    body = defaults.first_person_body
    # Keep the fitted FPS frame; only the rendered geometry changes at runtime.
    body.set_skeletal_mesh_asset(rig)
    body.set_editor_property('relative_scale3d', ue.Vector(1, 1, 1))
    body.set_editor_property('relative_location', ue.Vector(-11.2263, 1.4908, -159.9167))
    body.set_editor_property('relative_rotation', ue.Rotator(pitch=0, yaw=-90, roll=0))
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    if not assets.save_loaded_asset(bp, only_if_is_dirty=False):
        raise RuntimeError('Cannot save FPS skin references: ' + path)
    ue.log('ER_FPS_SKIN_DEFAULTS class=%s frame=%s' % (path, body.relative_location))

ue.log('ER_FPS_FOREARMS_READY skins=%d camera_frame_preserved=1 weapon_poses_preserved=1' % sum(len(x) for x in choices.values()))
