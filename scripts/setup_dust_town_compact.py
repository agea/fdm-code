"""Explicit revision4 migration: smaller town, opposite-swing gate circulation.

Preserve existing town art and other maps. Candidate seeds validate before save;
the completion hash prevents repeated setup from overwriting later artist edits.
"""
import hashlib
import json
from pathlib import Path
import unreal as ue

root=Path(ue.Paths.project_dir());private=root/'Assets/maps';assets=ue.EditorAssetLibrary
source=private/'SourceAssets/MapTemplates/DustTown/recipe.json'
recipe=json.loads(source.read_text());digest=hashlib.sha256(source.read_bytes()).hexdigest()
if recipe['revision']!=4:raise RuntimeError('Expected explicit compact town revision4')
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_DustMirror')
if not template or not template.get_editor_property('town').get_editor_property('enabled'):
    raise RuntimeError('Install the base town before applying compact revision4')
protected=[private/'Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset',
           private/'Content/EclipseRecoil/Maps/L_Killhouse.umap',
           private/'Content/EclipseRecoil/Art/MapTemplates/DA_Killhouse.uasset']
hash_file=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before={str(p):hash_file(p) for p in protected}
marker=assets.get_metadata_tag(template,'ERDustTownCompactSHA256')
if marker:
    if marker!=digest:raise RuntimeError('Completed compact recipe changed; use a new explicit migration')
    ue.log('ER_DUST_COMPACT_PRESERVED')
else:
    town=template.get_editor_property('town')
    for field in ['arena_min','arena_max']:town.set_editor_property(field,ue.Vector2D(*recipe[field]))
    candidate=ue.new_object(ue.ERMapTemplate)
    for field in ['template_id','source_reference','arena_size','shell_wall_mesh','pieces','alpha_spawns','omega_spawns','patrol_route','symmetry']:
        candidate.set_editor_property(field,template.get_editor_property(field))
    candidate.set_editor_property('revision',4);candidate.set_editor_property('town',town)
    for seed in list(range(512))+[-17,7919,2147483647]:
        if not candidate.validate_layout(seed):raise RuntimeError('Invalid compact town seed '+str(seed))
    levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    if not levels.load_level('/Game/EclipseRecoil/Maps/L_DustMirror'):raise RuntimeError('Cannot open Dust town')
    actor_system=ue.get_editor_subsystem(ue.EditorActorSubsystem)
    indices={a.get_editor_property('route_index') for a in actor_system.get_all_level_actors() if isinstance(a,ue.ERBotPatrolPoint)}
    # Every gate inserts six generated waypoints into the existing patrol loop.
    count=len(recipe['patrol'])+6*len(recipe['gates'])
    for index in range(count):
        if index in indices:continue
        point=actor_system.spawn_actor_from_class(ue.ERBotPatrolPoint,ue.Vector(0,0,100),ue.Rotator())
        point.set_actor_label('Dust_TownPatrol_'+str(index));point.set_editor_property('route_index',index)
    if not levels.save_current_level():raise RuntimeError('Cannot save diagonal patrol waypoints')
    template.set_editor_property('town',town);template.set_editor_property('revision',4)
    assets.set_metadata_tag(template,'ERDustTownCompactSHA256',digest)
    assets.set_metadata_tag(template,'ERDustTownRecipeSHA256',digest)
    if not assets.save_loaded_asset(template):raise RuntimeError('Cannot save compact template')
    (source.parent/'compact_migration.json').write_text(json.dumps(dict(revision=4,recipe_sha256=digest,validated_seeds=515,patrol_points=count,arena_min=recipe['arena_min'],arena_max=recipe['arena_max'],preserved=before),indent=2)+'\n')
    ue.log('ER_DUST_COMPACT_SAVED revision=4 seeds=515 patrol_points='+str(count))
for path,expected in before.items():
    if hash_file(Path(path))!=expected:raise RuntimeError('Protected asset changed '+path)
ue.log('ER_DUST_COMPACT_COMPLETE')
