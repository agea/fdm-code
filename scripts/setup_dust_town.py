"""Explicit Dust town revision3: plan graph, architecture and arched timber gates.

Unreal imports project-owned OBJ sources and saves only private assets. A
completed recipe hash protects later artist edits. Existing other maps and the
shared destruction catalog are preserved byte-for-byte.
"""
import hashlib, json
from pathlib import Path
import unreal as ue

root=Path(ue.Paths.project_dir());private=root/'Assets/maps';assets=ue.EditorAssetLibrary
source=private/'SourceAssets/MapTemplates/DustTown/recipe.json'
recipe=json.loads(source.read_text());digest=hashlib.sha256(source.read_bytes()).hexdigest()
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_DustMirror')
if not template: raise RuntimeError('Create the base Dust template first')
folder='/Game/EclipseRecoil/Art/MapTemplates/DustTown'
catalog=assets.load_asset('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials')
protected=[private/'Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset',
           private/'Content/EclipseRecoil/Maps/L_Killhouse.umap',
           private/'Content/EclipseRecoil/Art/MapTemplates/DA_Killhouse.uasset']
hash_file=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before={str(p):hash_file(p) for p in protected}
marker=assets.get_metadata_tag(template,'ERDustTownRecipeSHA256')
if marker:
    if marker!=digest:raise RuntimeError('Completed town recipe changed; use a new explicit migration')
    ue.log('ER_DUST_TOWN_PRESERVED')
else:
    def load(path):
        result=assets.load_asset(path)
        if not result:raise RuntimeError('Missing '+path)
        return result
    def save(obj):
        if not assets.save_loaded_asset(obj):raise RuntimeError('Cannot save '+obj.get_path_name())
    # Closed, curved silhouettes; runtime uses matching authored collision
    # slabs/planks instead of filling the arch opening with a fitted box.
    meshes=[]
    for name in ['SM_ER_DustArch','SM_ER_DustDoorLeft','SM_ER_DustDoorRight']:
        path=folder+'/'+name
        obj_source=source.parent/(name+'.obj');obj_hash=hash_file(obj_source)
        existing=assets.load_asset(path) if assets.does_asset_exist(path) else None
        if not existing or assets.get_metadata_tag(existing,'EROwnedOBJHash')!=obj_hash:
            task=ue.AssetImportTask();task.filename=str(source.parent/(name+'.obj'))
            task.destination_path=folder;task.destination_name=name
            task.automated=True;task.save=True;task.replace_existing=True
            options=ue.FbxImportUI();options.set_editor_property('import_mesh',True)
            options.set_editor_property('import_as_skeletal',False)
            options.set_editor_property('import_materials',False);options.set_editor_property('import_textures',False)
            options.set_editor_property('mesh_type_to_import',ue.FBXImportType.FBXIT_STATIC_MESH)
            options.static_mesh_import_data.set_editor_property('combine_meshes',True)
            options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs',False)
            options.static_mesh_import_data.set_editor_property('convert_scene',False)
            options.static_mesh_import_data.set_editor_property('convert_scene_unit',False)
            task.options=options
            ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        mesh=load(path)
        assets.set_metadata_tag(mesh,'EROwnedOBJHash',obj_hash);save(mesh)
        bounds=mesh.get_bounds().box_extent
        ue.log('ER_DUST_TOWN_MESH '+name+' bounds='+str(bounds))
        if min(bounds.x,bounds.y,bounds.z)<=0:raise RuntimeError('Invalid generated mesh '+name)
        meshes.append(mesh)
    # Reuse the existing owned PBR graphs, adding a tint parameter to private
    # town copies. No shared material/combat profile is edited.
    edit=ue.MaterialEditingLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
    def tinted(name,parent_path,tint):
        path=folder+'/'+name
        if assets.does_asset_exist(path):return load(path)
        material=assets.duplicate_asset(parent_path,path)
        if not isinstance(material,ue.Material):raise RuntimeError('Expected an owned PBR base')
        previous=edit.get_material_property_input_node(material,ue.MaterialProperty.MP_BASE_COLOR)
        color=edit.create_material_expression(material,ue.MaterialExpressionVectorParameter,0,-200)
        color.set_editor_property('parameter_name','TownTint');color.set_editor_property('default_value',ue.LinearColor(*tint,1))
        multiply=edit.create_material_expression(material,ue.MaterialExpressionMultiply,200,0)
        edit.connect_material_expressions(previous,'RGB',multiply,'A');edit.connect_material_expressions(color,'',multiply,'B')
        edit.connect_material_property(multiply,'',ue.MaterialProperty.MP_BASE_COLOR)
        edit.recompile_material(material);save(material);return material
    wall=tinted('M_ER_TownPlaster','/Game/EclipseRecoil/Art/ArabianEnvironment/Materials/Runtime/M_ER_Arabian_Stonewall',(1.05,.92,.72))
    road=tinted('M_ER_TownPaving','/Game/EclipseRecoil/Art/ProceduralTextures/M_ER_PBR_concrete_wall_006',(.9,.79,.61))
    roof=tinted('M_ER_TownRoof','/Game/EclipseRecoil/Art/ProceduralTextures/M_ER_PBR_concrete_wall_006',(.75,.62,.45))
    iron_path=folder+'/M_ER_TownIron'
    iron=assets.load_asset(iron_path) if assets.does_asset_exist(iron_path) else None
    if not iron:
        iron=tools.create_asset('M_ER_TownIron',folder,ue.Material,ue.MaterialFactoryNew())
        iron.set_editor_property('used_with_instanced_static_meshes',True)
        node=edit.create_material_expression(iron,ue.MaterialExpressionConstant3Vector,-200,0)
        node.set_editor_property('constant',ue.LinearColor(.025,.021,.018,1))
        edit.connect_material_property(node,'',ue.MaterialProperty.MP_BASE_COLOR)
        rough=edit.create_material_expression(iron,ue.MaterialExpressionConstant,-200,120);rough.set_editor_property('r',.8)
        edit.connect_material_property(rough,'',ue.MaterialProperty.MP_ROUGHNESS);edit.recompile_material(iron);save(iron)
    wood=next(row.get_editor_property('surface') for row in catalog.get_editor_property('materials') if row.get_editor_property('type')==ue.ERBlockMaterial.WOOD)
    town=ue.ERDustTownVariation()
    for field in ['arena_min','arena_max','building_height','door_gap','crossing']:
        town.set_editor_property(field,ue.Vector2D(*recipe[field]))
    town.set_editor_property('enabled',True);town.set_editor_property('axis_warp',recipe['axis_warp'])
    streets=[]
    for item in recipe['streets']:
        s=ue.ERPlanStreet();s.set_editor_property('name',item['name'])
        s.set_editor_property('points',[ue.Vector2D(*p) for p in item['points']]);s.set_editor_property('width',ue.Vector2D(*item['width']))
        s.set_editor_property('elevated',item['elevated']);streets.append(s)
    town.set_editor_property('streets',streets)
    courts=[]
    for item in recipe['courts']:
        c=ue.ERPlanCourt();c.set_editor_property('name',item['name'])
        for field in ['center','size']:c.set_editor_property(field,ue.Vector2D(*item[field]))
        courts.append(c)
    town.set_editor_property('courts',courts)
    gates=[]
    for item in recipe['gates']:
        g=ue.ERPlanGate();g.set_editor_property('street',item['street']);g.set_editor_property('position',ue.Vector2D(*item['position']));gates.append(g)
    town.set_editor_property('gates',gates);town.set_editor_property('patrol',[ue.Vector2D(*p) for p in recipe['patrol']])
    for field,value in [('wall_surface',wall),('road_surface',road),('roof_surface',roof),('wood_surface',wood),('iron_surface',iron),
                        ('arch_mesh',meshes[0]),('left_door_mesh',meshes[1]),('right_door_mesh',meshes[2])]:town.set_editor_property(field,value)
    candidate=ue.new_object(ue.ERMapTemplate)
    for field in ['template_id','source_reference','arena_size','shell_wall_mesh','pieces','alpha_spawns','omega_spawns','patrol_route']:
        candidate.set_editor_property(field,template.get_editor_property(field))
    candidate.set_editor_property('symmetry',ue.ERTemplateSymmetry.NONE)
    candidate.set_editor_property('revision',recipe['revision']);candidate.set_editor_property('town',town)
    for seed in list(range(512))+[-17,7919,2147483647]:
        if not candidate.validate_layout(seed):raise RuntimeError('Invalid town seed '+str(seed))
    for field in ['revision','symmetry','town']:template.set_editor_property(field,candidate.get_editor_property(field))
    save(template)
    levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    if not levels.load_level('/Game/EclipseRecoil/Maps/L_DustMirror'):raise RuntimeError('Cannot open town map')
    actor_system=ue.get_editor_subsystem(ue.EditorActorSubsystem);patrol_indices=set()
    for actor in actor_system.get_all_level_actors():
        if isinstance(actor,ue.ERProceduralMap):actor.set_editor_property('floor_surface',road)
        if isinstance(actor,ue.ERBotPatrolPoint):patrol_indices.add(actor.get_editor_property('route_index'))
    for index in range(len(recipe['patrol'])):
        if index in patrol_indices:continue
        point=actor_system.spawn_actor_from_class(ue.ERBotPatrolPoint,ue.Vector(0,0,100),ue.Rotator(pitch=0,yaw=0,roll=0))
        point.set_actor_label('Dust_TownPatrol_'+str(index));point.set_editor_property('route_index',index)
    if not levels.save_current_level():raise RuntimeError('Cannot save town surface')
    assets.set_metadata_tag(template,'ERDustTownRecipeSHA256',digest);save(template)
    ue.log('ER_DUST_TOWN_SAVED revision='+str(recipe['revision'])+' seeds=515')
for path,expected in before.items():
    if hash_file(Path(path))!=expected:raise RuntimeError('Protected asset changed '+path)
ue.log('ER_DUST_TOWN_SETUP_COMPLETE')
