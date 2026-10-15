"""Read-only Unreal audit of saved catalog, animation and source dependencies."""
from pathlib import Path
from collections import Counter
import csv
import hashlib
import json
import unreal as ue

project=Path(ue.Paths.project_dir()).resolve()
root=project/'Assets/weapons/SourceAssets/SketchfabWeapons'
catalog=json.loads((root/'import-report.json').read_text())
source=json.loads((root/'source-manifest.json').read_text())
failures=[];results=[]
def check(value,label):
    ue.log(('ER_REFERENCE_ASSET_PASS ' if value else 'ER_REFERENCE_ASSET_FAIL ')+label)
    if not value:failures.append(label)

check(len(catalog)==10,'complete_ten_slot_catalog')
check(len(source)==9,'nine_original_archives_with_attribution')
for item in source:
    path=root/item['uid']/'original.zip';digest=hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
    check(digest.hexdigest()==item['archive_sha256'],'original_archive_sha256_'+item['uid'])

fitted=ue.load_asset('/Game/EclipseRecoil/Characters/FirstPerson/SKEL_ER_FirstPerson')
for index,item in enumerate(catalog):
    key=item['key'];data=ue.load_asset(item['data']);mesh=ue.load_asset(item['model'])
    check(data and mesh and data.get_editor_property('mesh')==mesh,'saved_model_binding_'+key)
    check(all(s.material_interface and '/Sketchfab/' in s.material_interface.get_path_name() for s in mesh.get_editor_property('materials')),'private_material_bindings_'+key)
    for name,expected in item['stats'].items():
        check(abs(data.get_editor_property(name)-expected)<1e-5,'source_stat_'+key+'_'+name)
    idle=data.get_editor_property('idle_animation')
    check(idle and idle.get_editor_property('skeleton')==fitted,'existing_fps_skeleton_'+key)
    for label,entry in item['clips'].items():
        animation=ue.load_asset(entry['path'])
        check(animation and animation.get_editor_property('skeleton')==mesh.get_editor_property('skeleton'),'weapon_clip_skeleton_'+key+'_'+label)
    if 1<=index<=7:
        reload=data.get_editor_property('weapon_reload_animation');hands=data.get_editor_property('reload_animation')
        check(reload and hands and hands.get_editor_property('skeleton')==fitted,'authored_reload_and_skin_hands_'+key)
        options=ue.AnimPoseEvaluationOptions();options.set_editor_property('should_retarget',False);options.set_editor_property('optional_skeletal_mesh',mesh)
        start=ue.AnimPoseExtensions.get_anim_pose_at_time(reload,0,options)
        names=ue.AnimPoseExtensions.get_bone_names(start)
        moved=set()
        for ratio in [.25,.5,.75,1]:
            pose=ue.AnimPoseExtensions.get_anim_pose_at_time(reload,reload.get_play_length()*ratio,options)
            for bone in names:
                first=ue.AnimPoseExtensions.get_bone_pose(start,bone,ue.AnimPoseSpaces.WORLD)
                current=ue.AnimPoseExtensions.get_bone_pose(pose,bone,ue.AnimPoseSpaces.WORLD)
                if str(first)!=str(current):moved.add(str(bone))
        check(bool(moved),'saved_reload_contains_mechanical_motion_'+key)
    results.append(dict(key=key,mesh=mesh.get_path_name(),material_slots=len(mesh.get_editor_property('materials')),clips=list(item['clips'])))

launcher=ue.load_asset(catalog[7]['data'])
check(launcher.get_editor_property('projectile_mesh') is not None,'source_model_projectile_binding')
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget']:
    bp=ue.load_asset(path);defaults=ue.get_default_object(bp.generated_class())
    references=defaults.get_editor_property('reference_arsenal')
    check([v.get_path_name() for v in references]==[v['data'] for v in catalog],'blueprint_catalog_'+path.rsplit('/',1)[1])
controller=ue.get_default_object(ue.load_asset('/Game/EclipseRecoil/Core/BP_ERPlayerController').generated_class())
check(controller.get_editor_property('reference_arsenal_menu'),'default_menu_uses_ten_slot_controls')
check(len(controller.get_editor_property('primary_options'))==7,'legacy_menu_and_server_loadout_choices_retained')

counts=Counter()
for path in ue.EditorAssetLibrary.list_assets('/Game/EclipseRecoil/Weapons/Sketchfab',recursive=True,include_folder=False):
    asset=ue.load_asset(path)
    if asset:counts[asset.get_class().get_name()]+=1
receipt=dict(passed=not failures,failures=failures,original_archives=len(source),catalog_slots=len(catalog),asset_counts=dict(counts),weapons=results)
(project/'docs/REFERENCE_WEAPON_RESULTS.json').write_text(json.dumps(receipt,indent=2)+'\n')
check(not failures,'complete_saved_asset_audit')
if failures:raise RuntimeError('Reference weapon asset verification failed: '+str(failures))
