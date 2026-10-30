"""Measure actual authored head/boot vertices in reference, idle and preview walk poses."""
from pathlib import Path
import json
import unreal as u
p=Path(u.Paths.project_dir()).resolve()
c=u.get_default_object(u.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
rows=[]
for index in [0,1,2,5,6,7,8,9]:
 a=c.alpha_skins[index];mesh=a.mesh
 probe=u.SkeletalMeshComponent();probe.set_skeletal_mesh_asset(mesh)
 points={bone:list(u.ERCharacter.get_presentation_vertices(mesh,bone)) for bone in ['head','foot_l','foot_r','ball_l','ball_r'] if probe.get_bone_index(bone)>=0}
 opts=u.AnimPoseEvaluationOptions();opts.optional_skeletal_mesh=mesh
 walk=u.load_asset(a.corpse_pose.get_path_name().replace('MF_Rifle_Idle_ADS','MF_Unarmed_Walk_Fwd'))
 row={'index':index,'name':a.display_name,'scale':a.scale,'feet_offset':a.feet_offset,'samples':[]}
 for label,clip,t in [('reference',None,0),('idle',a.corpse_pose,0)]+[('walk',walk,t) for t in [0,.2,.4,.6,.8]]:
  pose=u.AnimPoseExtensions.get_anim_pose_at_time(clip,t,opts) if clip else mesh.skeleton.get_reference_pose()
  z={}
  for bone,vs in points.items():
   transform=u.AnimPoseExtensions.get_bone_pose(pose,bone,u.AnimPoseSpaces.WORLD)
   z[bone]=[u.MathLibrary.transform_location(transform,v).z*a.scale+92.5+a.feet_offset for v in vs]
  top=max(z['head']);bottom=min(x for b,vs in z.items() if b!='head' for x in vs)
  head=u.AnimPoseExtensions.get_bone_pose(pose,'head',u.AnimPoseSpaces.WORLD).translation
  row['samples'].append({'pose':label,'time':t,'head_top_cm':top,'boot_bottom_cm':bottom,'height_cm':top-bottom,'head_bone_z_cm':head.z*a.scale+92.5+a.feet_offset})
 rows.append(row)
(p/'Saved/Testing/WraithReplacement/heights.json').write_text(json.dumps(rows,indent=2))
