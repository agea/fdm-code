"""Match Belica/Drongo's dyed surfaces to the muted Wraith/Gadget palette.
Only existing owned team-colour materials change. No meshes, catalogs or source
art are saved. Natural/neutral surfaces and the new characters stay untouched.
"""
from pathlib import Path
import hashlib,json,zipfile
import unreal as u
A=u.EditorAssetLibrary;E=u.MaterialEditingLibrary;p=Path(u.Paths.project_dir()).resolve()
out=p/'Saved/Testing/MutedLegacyPalette';out.mkdir(parents=True,exist_ok=True)
root='/Game/EclipseRecoil/Characters/Teams/'
c=u.get_default_object(A.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
def graph(m):
 while isinstance(m,u.MaterialInstanceConstant):m=m.get_editor_property('parent')
 return m
def nodes(m):return {str(x.get_editor_property('desc')):x for x in E.get_material_expressions(m)}
def rel(m):return 'Assets/characters/Content/'+m.get_path_name().split('.')[0].removeprefix('/Game/')+'.uasset'
def digest(f):return hashlib.sha256(f.read_bytes()).hexdigest()
receipt=out/'changed.json'
if receipt.exists():
 rows=json.loads(receipt.read_text())
 assert all(digest(p/x['file'])==x['after_hash'] for x in rows),'Palette changed since the saved migration; inspect before rerunning'
 u.log('ER_MUTED_LEGACY_PALETTE_ALREADY_APPLIED')
else:
 targets={};protected_graphs=set()
 for team in ['alpha','omega']:
  for i,a in enumerate(c.get_editor_property(team+'_skins')):
   selected=(team=='alpha' and i>=5) or (team=='omega' and i<5)
   for m in a.materials:
    if not m:continue
    g=graph(m)
    if not selected:protected_graphs.add(g.get_path_name());continue
    n=nodes(g);t=n.get('ER_GroundedTint')
    if not t:continue
    color=E.get_material_instance_vector_parameter_value(m,'ER_GroundedTint') if isinstance(m,u.MaterialInstanceConstant) else t.get_editor_property('default_value')
    r,green,b=color.r,color.g,color.b
    dyed=(b>.3 and b>r*2 and b>green*1.35) if team=='alpha' else (r>.5 and r>green*1.7 and r>b*3)
    if not dyed:continue
    assert g.get_path_name().startswith(root+team.title()+'/Skins/'),g.get_path_name()
    original_strength=float(A.get_metadata_tag(m,'ERTeamReadabilityStrength') or A.get_metadata_tag(g,'ERTeamReadabilityStrength') or r)
    strength=1.0 if team=='alpha' else max(.9,min(1.1,original_strength/.77))
    tint=[.12,.28,.52] if team=='alpha' else [.62*strength,.29*strength,.115*strength]
    for material in {m,g}:targets[material.get_path_name()]={'asset':material,'graph':g,'team':team,'tint':tint,'old_tint':[r,green,b]}
 assert targets and not (set(x['graph'].get_path_name() for x in targets.values())&protected_graphs),'A target graph is shared with a protected new character'
 filenames={rel(x['asset']) for x in targets.values()}
 protected={str(f.relative_to(p)):digest(f) for f in (p/'Assets/characters/Content').rglob('*.uasset') if str(f.relative_to(p)) not in filenames}
 (out/'protected-hashes.json').write_text(json.dumps(protected))
 with zipfile.ZipFile(out/'materials-before.zip','w',zipfile.ZIP_DEFLATED) as z:
  for file in sorted(filenames):z.write(p/file,file)
 rows=[];done=set()
 for key,x in sorted(targets.items()):
  m=x['asset'];g=x['graph'];before=digest(p/rel(m));n=nodes(g)
  if g.get_path_name() not in done:
   n['ER_GroundedTint'].set_editor_property('default_value',u.LinearColor(*x['tint'],1))
   if 'ER_DesaturationAmount' in n:n['ER_DesaturationAmount'].set_editor_property('r',.85)
   lift=n.get('ER_TeamReadabilityLift')
   if lift:
    lift.set_editor_property('const_alpha',.08 if x['team']=='alpha' else .10);lift.set_editor_property('const_b',.5)
   g.set_editor_property('used_with_skeletal_mesh',True);A.set_metadata_tag(g,'ERMutedLegacyPalette','1');E.recompile_material(g)
   # Save each material below so its before_hash always refers to pre-migration data.
   done.add(g.get_path_name())
  if isinstance(m,u.MaterialInstanceConstant):
   E.set_material_instance_vector_parameter_value(m,'ER_GroundedTint',u.LinearColor(*x['tint'],1));E.update_material_instance(m)
  A.set_metadata_tag(m,'ERMutedLegacyPalette','1');assert A.save_loaded_asset(m,False)
  rows.append({'file':rel(m),'asset':m.get_path_name(),'team':x['team'],'before_hash':before,'after_hash':digest(p/rel(m)),'old_tint':x['old_tint'],'tint':x['tint'],'desaturation':.85,'lift_alpha':.08 if x['team']=='alpha' else .10,'lift_white':.5})
 for file,h in protected.items():assert digest(p/file)==h,file
 receipt.write_text(json.dumps(rows,indent=2))
 (out/'setup.json').write_text(json.dumps({'changed_owned_materials':len(rows),'protected_packages':len(protected),'new_characters_unchanged':True,'geometry_catalogs_fps_rigs_unchanged':True},indent=2))
 u.log('ER_MUTED_LEGACY_PALETTE_READY materials='+str(len(rows)))
