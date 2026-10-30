"""Read saved character catalog and check animated boot contact for all 20 skins."""
from pathlib import Path
import json
import unreal as u
c=u.get_default_object(u.EditorAssetLibrary.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
rows=[]
for team in ['alpha','omega']:
 for i,a in enumerate(c.get_editor_property(team+'_skins')):
  mesh=a.mesh
  points={b:list(u.ERCharacter.get_presentation_vertices(mesh,b)) for b in ['foot_l','foot_r','ball_l','ball_r']}
  opts=u.AnimPoseEvaluationOptions();opts.optional_skeletal_mesh=mesh
  samples=[]
  for t in [a.corpse_pose.get_play_length()*j/16 for j in range(17)]:
   pose=u.AnimPoseExtensions.get_anim_pose_at_time(a.corpse_pose,t,opts)
   zs=[u.MathLibrary.transform_location(u.AnimPoseExtensions.get_bone_pose(pose,b,u.AnimPoseSpaces.WORLD),v).z*a.scale+92.5+a.feet_offset for b,vs in points.items() for v in vs]
   samples.append(min(zs))
  rows.append(dict(team=team,index=i,name=a.display_name,offset=a.feet_offset,bottoms=samples))
assert all(abs(min(r['bottoms'])) < .001 for r in rows)
out=Path(u.Paths.project_dir()).resolve()/'Saved/Testing/CharacterGrounding'
out.mkdir(parents=True,exist_ok=True)
(out/'verified.json').write_text(json.dumps(rows,indent=2))
u.log('ER_CHARACTER_GROUNDING_VERIFY_PASS skins=20')
