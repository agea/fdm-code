"""Audit/delete character packages outside the complete retained game closure.
Keeps all twenty offline-selectable skins, external project references, native
asset loads and the owned animation libraries used by derived menu walk paths.
Run -ERDeleteUnusedCharacters only after the audit has been reviewed.
"""
from pathlib import Path
import json,re,hashlib
import unreal as u
p=Path(u.Paths.project_dir()).resolve();out=Path('/private/tmp/rr-character-cleanup');out.mkdir(parents=True,exist_ok=True)
A=u.EditorAssetLibrary;R=u.AssetRegistryHelpers.get_asset_registry();R.search_all_assets(True);R.wait_for_completion()
opts=u.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True,include_searchable_names=False,include_soft_management_references=True,include_hard_management_references=True)
files={ '/Game/'+str(f.relative_to(p/'Assets/characters/Content')).removesuffix(f.suffix):f for f in (p/'Assets/characters/Content').rglob('*') if f.suffix in ['.uasset','.umap'] }
all_packages={str(x.package_name) for x in R.get_assets_by_path('/Game',recursive=True)}
roots=all_packages-set(files)
for folder in [p/'Source',p/'Config']:
 for f in folder.rglob('*'):
  if f.suffix not in ['.cpp','.h','.ini']:continue
  for value in re.findall(r'/Game/[A-Za-z0-9_/]+',f.read_text(errors='replace')):
   if value in files:roots.add(value)
# Dynamic menu walking paths are derived from each hero's pose library.
roots.update(x for x in files if '/EclipseRecoil/Characters/' in x and '/Animations/' in x)
c=u.get_default_object(A.load_blueprint_class('/Game/EclipseRecoil/Core/BP_ERCharacter'))
assert len(c.alpha_skins)==len(c.omega_skins)==10
for a in list(c.alpha_skins)+list(c.omega_skins):
 for asset in [a.mesh,a.first_person_mesh,a.animation_class,a.corpse_pose]+list(a.materials)+[x.mesh for x in a.attachments]+[x.material for x in a.attachments]:
  if asset:roots.add(asset.get_path_name().split('.')[0])
seen=set();pending=list(roots)
while pending:
 name=pending.pop()
 if name in seen:continue
 seen.add(name);pending.extend(str(x) for x in R.get_dependencies(name,opts) if str(x).startswith('/Game/') and str(x) not in seen)
dead=set(files)-seen
outside={name:sorted(str(x) for x in R.get_referencers(name,opts) if str(x) not in dead) for name in dead}
assert not any(outside.values()),{k:v for k,v in outside.items() if v}
kept=set(files)-dead
report={'candidate_packages':len(files),'retained_packages':len(kept),'delete_packages':len(dead),'delete_bytes':sum(files[x].stat().st_size for x in dead),'retained_bytes':sum(files[x].stat().st_size for x in kept),'roots':sorted(roots&set(files)),'delete':{x:str(files[x].relative_to(p)) for x in sorted(dead)},'retained':{x:str(files[x].relative_to(p)) for x in sorted(kept)},'protected_hashes':{str(files[x].relative_to(p)):hashlib.sha256(files[x].read_bytes()).hexdigest() for x in kept},'outside_referencers':outside}
(out/'character-plan.json').write_text(json.dumps(report,indent=2))
u.log('ER_CHARACTER_PRUNE_PLAN delete=%d bytes=%d retained=%d'%(len(dead),report['delete_bytes'],len(kept)))
if '-ERDeleteUnusedCharacters' in u.SystemLibrary.get_command_line().split():
 assert u.ERCharacter.delete_unloaded_paragon_packages(sorted(dead))
 assert all(not files[x].exists() for x in dead)
 for rel,h in report['protected_hashes'].items():assert hashlib.sha256((p/rel).read_bytes()).hexdigest()==h,rel
 report['deleted']=True;(out/'character-plan.json').write_text(json.dumps(report,indent=2));u.log('ER_CHARACTER_PRUNE_COMPLETE')
