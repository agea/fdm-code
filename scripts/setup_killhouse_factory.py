"""Unreal-only revision2 upgrade: Factory fixed art, rear roof access and balance.

Preserves all material/combat profiles and other maps. A revision marker makes
reruns read-only; later art edits are not overwritten implicitly.
"""
import hashlib,json
from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir());private=root/'Assets/maps'
source=private/'SourceAssets/MapTemplates/killhouse_template.json';data=json.loads(source.read_text())
assets=ue.EditorAssetLibrary;template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_Killhouse')
if not template:raise RuntimeError('Missing existing template')
catalog=private/'Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset'
before=hashlib.sha256(catalog.read_bytes()).hexdigest()
version=assets.get_metadata_tag(template,'ERFactoryBalancedRevision')
manifest_hash=hashlib.sha256(source.read_bytes()).hexdigest()
previous_hash=assets.get_metadata_tag(template,'ERFactoryManifestSHA256')
if previous_hash and previous_hash!=manifest_hash:raise RuntimeError('Manifest changed after completed migration; create an explicit new revision to preserve artist edits')
if version=='2' and previous_hash==manifest_hash:
    ue.log('ER_FACTORY_TEMPLATE_PRESERVED revision2')
else:
    # A duplicate holds the complete candidate until native generation validates
    # every representative seed; live asset changes only after these checks.
    candidate=ue.new_object(ue.ERMapTemplate)
    candidate.set_editor_property('template_id',data['id']);candidate.set_editor_property('source_reference',data['source'])
    candidate.set_editor_property('revision',data['revision']);candidate.set_editor_property('arena_size',ue.Vector(*data['arena_size']))
    candidate.set_editor_property('symmetry',ue.ERTemplateSymmetry.HALF_TURN)
    for field in ['shell_wall_mesh','roof_mesh']:
        mesh=assets.load_asset(data[field])
        if not mesh:raise RuntimeError('Missing Factory package '+data[field])
        candidate.set_editor_property(field,mesh)
    parts=[]
    for item in data['pieces']:
        p=ue.ERMapTemplatePiece()
        for k,value in [('center',ue.Vector(*item['center'])),('size',ue.Vector(*item['size'])),('jitter',ue.Vector2D(*item['jitter'])),('group',item['group']),('destructible',item['destructible']),('material',getattr(ue.ERBlockMaterial,item['material'].upper())),('mirrored',item['mirrored']),('anchored',item['anchored']),('mesh_rotation',ue.Rotator(pitch=item['mesh_rotation'][0],yaw=item['mesh_rotation'][1],roll=item['mesh_rotation'][2]))]:p.set_editor_property(k,value)
        if item['fixed_mesh']:
            if item['destructible']:raise RuntimeError('Vendor geometry on destructible piece')
            mesh=assets.load_asset(item['fixed_mesh'])
            if not mesh:raise RuntimeError('Missing Factory mesh '+item['fixed_mesh'])
            p.set_editor_property('fixed_mesh',mesh)
        parts.append(p)
    candidate.set_editor_property('pieces',parts)
    for team in ['alpha','omega']:
        candidate.set_editor_property(team+'_spawns',[ue.Transform(location=ue.Vector(*s['position']),rotation=ue.Rotator(pitch=0,yaw=s['yaw'],roll=0),scale=ue.Vector(1,1,1)) for s in data['spawns'][team]])
    candidate.set_editor_property('patrol_route',[ue.Vector(*p) for p in data['patrol_route']])
    for seed in sorted(set([0,1,42,7919,-17,2147483647]+list(range(100)))):
        if not candidate.validate_layout(seed):raise RuntimeError('Invalid balanced template at seed '+str(seed))
    for field in ['template_id','source_reference','revision','arena_size','symmetry','shell_wall_mesh','roof_mesh','pieces','alpha_spawns','omega_spawns','patrol_route']:
        template.set_editor_property(field,candidate.get_editor_property(field))
    assets.set_metadata_tag(template,'ERFactoryBalancedRevision','2')
    assets.set_metadata_tag(template,'ERFactoryManifestSHA256',manifest_hash)
    if not assets.save_loaded_asset(template):raise RuntimeError('Cannot save template upgrade')
    # The map's existing starts and patrol actors follow the revised balanced data.
    levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    if not levels.load_level('/Game/EclipseRecoil/Maps/L_Killhouse'):raise RuntimeError('Cannot open map')
    counts={'alpha':0,'omega':0}
    for actor in ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors():
        if isinstance(actor,ue.ERTeamPlayerStart):
            team='alpha' if actor.get_editor_property('team')==ue.ERTeam.ALPHA else 'omega'
            s=data['spawns'][team][counts[team]];counts[team]+=1
            actor.root_component.set_editor_property('mobility',ue.ComponentMobility.MOVABLE)
            actor.set_actor_location(ue.Vector(*s['position']),False,False);actor.set_actor_rotation(ue.Rotator(pitch=0,yaw=s['yaw'],roll=0),False)
        elif isinstance(actor,ue.ERBotPatrolPoint):
            i=actor.get_editor_property('route_index')
            if i<len(data['patrol_route']):
                actor.root_component.set_editor_property('mobility',ue.ComponentMobility.MOVABLE)
                actor.set_actor_location(ue.Vector(*data['patrol_route'][i]),False,False)
    if not levels.save_current_level():raise RuntimeError('Cannot save balanced starts/routes')
    ue.log('ER_FACTORY_TEMPLATE_UPGRADED pieces='+str(len(parts))+' starts='+str(counts))
if hashlib.sha256(catalog.read_bytes()).hexdigest()!=before:raise RuntimeError('Project destruction catalog changed')
receipt={'revision':2,'symmetry':'HalfTurn','destruction_catalog_sha256':before,'destruction_catalog_unchanged':True,'authored_pieces':len(data['pieces']),'meshes':sorted(set(p['fixed_mesh'] for p in data['pieces'] if p['fixed_mesh']))}
(private/'SourceAssets/FactoryEnvironment/template_upgrade.json').write_text(json.dumps(receipt,indent=2)+'\n')
ue.log('ER_FACTORY_TEMPLATE_COMPLETE')
