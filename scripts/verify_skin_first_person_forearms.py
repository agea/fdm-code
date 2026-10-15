"""Read saved skin FPS assets and compare both grips with the old weapon poses."""
import math
import unreal as ue

rig = ue.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/FirstPerson/SK_ER_Twinblast_Alpha_Officer_Forearms')
assert rig.skeleton.get_path_name() == '/Game/EclipseRecoil/Characters/FirstPerson/SKEL_ER_FirstPerson.SKEL_ER_FirstPerson'
catalog = ue.get_default_object(ue.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
reference = ue.AnimPoseEvaluationOptions()
reference.set_editor_property('optional_skeletal_mesh', rig)
samples = 0

def point(vector):
    return vector.x, vector.y, vector.z

for team in ['alpha', 'omega']:
    for index, appearance in enumerate(catalog.get_editor_property(team + '_skins')):
        mesh = appearance.first_person_mesh
        assert mesh and mesh.skeleton == rig.skeleton, (team, index, 'FPS rig')
        assert len(mesh.materials) == len(appearance.materials), (team, index, 'skin materials')
        expected_bounds, actual_bounds = rig.get_imported_bounds(), mesh.get_imported_bounds()
        assert math.dist(point(expected_bounds.origin),point(actual_bounds.origin)) < .01, (team,index,'culling origin')
        assert math.dist(point(expected_bounds.box_extent),point(actual_bounds.box_extent)) < .01, (team,index,'culling extent')
        evaluation = ue.AnimPoseEvaluationOptions()
        evaluation.set_editor_property('optional_skeletal_mesh', mesh)
        for data in catalog.arsenal:
            assert data and data.mesh and data.idle_animation and data.fire_animation
            for animation, times in [(data.idle_animation, [0]), (data.fire_animation, [0, 1/60, .05, .1])]:
                for time in times:
                    old = ue.AnimPoseExtensions.get_anim_pose_at_time(animation, time, reference)
                    new = ue.AnimPoseExtensions.get_anim_pose_at_time(animation, time, evaluation)
                    for socket in ['HandGrip_R', 'HandGrip_L']:
                        expected = ue.AnimPoseExtensions.get_socket_pose(old, socket, ue.AnimPoseSpaces.WORLD)
                        actual = ue.AnimPoseExtensions.get_socket_pose(new, socket, ue.AnimPoseSpaces.WORLD)
                        assert math.dist(point(expected.translation), point(actual.translation)) < .01, (team,index,data.display_name,socket,'position')
                        a, b = expected.rotation, actual.rotation
                        assert abs(1-abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)) < .0001, (team,index,data.display_name,socket,'rotation')
                        assert math.dist(point(expected.scale3d), point(actual.scale3d)) < .001, (team,index,data.display_name,socket,'scale')
                    samples += 1
        ue.log('ER_FPS_FOREARMS_SAVED_PASS team=%s skin=%d weapons=%d mesh=%s' % (team,index,len(catalog.arsenal),mesh.get_name()))

for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget']:
    defaults = ue.get_default_object(ue.EditorAssetLibrary.load_blueprint_class(path))
    body = defaults.first_person_body
    assert math.dist(point(body.relative_location),(-11.2263,1.4908,-159.9167)) < .001
    assert point(body.relative_scale3d) == (1,1,1)
    assert defaults.first_person_rifle.get_editor_property('absolute_scale')
    for team in ['alpha','omega']:
        assert [x.first_person_mesh for x in defaults.get_editor_property(team+'_skins')] == [x.first_person_mesh for x in catalog.get_editor_property(team+'_skins')]

ue.log('ER_FPS_FOREARMS_SAVED_COMPLETE skins=20 weapons=11 samples=%d grips_per_sample=2 frame_preserved=1' % samples)
