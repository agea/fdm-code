"""Import the requested Sketchfab arsenal with Unreal's asset APIs.

Run after bake_sketchfab_weapons.py with Unreal PythonScript. New packages
live under the existing private weapons mount; existing vendor assets stay
unchanged. Blueprint activation happens only after the complete catalog passes.
"""
from pathlib import Path
import csv
import json
import runpy
import re
import unreal as ue

project=Path(ue.Paths.project_dir()).resolve()
root=project/'Assets/weapons/SourceAssets/SketchfabWeapons'
folder='/Game/EclipseRecoil/Weapons/Sketchfab'
assets=ue.EditorAssetLibrary
tools=ue.AssetToolsHelpers.get_asset_tools()
staging='-ERStageReferenceWeapons' in ue.SystemLibrary.get_command_line()
reimport='-ERReimportReferenceWeapons' in ue.SystemLibrary.get_command_line()
repair_materials='-ERRepairReferenceMaterials' in ue.SystemLibrary.get_command_line()
retarget=runpy.run_path(str(project/'scripts/retarget_weapon_hands.py'))['retarget']
palm_tools=runpy.run_path(str(project/'scripts/setup_palm_contacts.py'))
pistol_tools=runpy.run_path(str(project/'scripts/fit_pistol_presentation.py'))
palm_tools['setup']()
selection=re.search(r'-ERReferenceOnly=([A-Za-z,]+)',ue.SystemLibrary.get_command_line())
selected_keys=set(selection.group(1).split(',')) if selection else None
previous_file=root/'import-report.json'
previous={item['key']:item for item in json.loads(previous_file.read_text())} if previous_file.exists() else {}

profiles=[
    dict(key='Knife',prefix='f3140',name='Knife',legacy='M1911',one=True,payload='MELEE',damage=40,interval=.5,ammo=1,reserve=0,range=160,spread=0),
    dict(key='Pistol',prefix='bd896',name='Beretta · Desert Eagle',row='deagle',legacy='M1911',one=True,ammo=7,reserve=35,spread=.25,reload=2.2),
    dict(key='SMG',prefix='ca37',name='SST30 · MP9',row='mp9',legacy='Uzi',ammo=30,reserve=120,auto=True,spread=.35,reload=2.1),
    dict(key='Shotgun',prefix='c3d3',name='Shotgun · XM1014',row='xm1014',legacy='SG_M4',ammo=7,reserve=32,auto=True,spread=3.2,pellets=6,reload=3.0),
    dict(key='Assault',prefix='924e',name='AR15 · AK-47',row='ak47',legacy='AK74',ammo=30,reserve=90,auto=True,spread=.18,reload=2.5),
    dict(key='Carbine',prefix='62977',name='Carbine · SCAR-20',row='scar20',legacy='M4_8',ammo=20,reserve=90,auto=True,spread=.16,reload=3.1,zoom=35),
    dict(key='Sniper',prefix='eae1',name='Sniper · AWP',row='awp',legacy='M107',ammo=5,reserve=30,spread=.12,reload=2.5,zoom=20),
    dict(key='Launcher',prefix='c1ec',name='Milkor M32 MGL',legacy='RPG7',payload='LAUNCHED_HE',damage=140,interval=.5,ammo=6,reserve=12,spread=.3,speed=3600,reload=5.5),
    dict(key='HE',prefix='287306',name='RGD-5 · HE',legacy='Grenade_RGD5',one=True,payload='HE',damage=140,interval=.8,ammo=2,reserve=0,spread=0,speed=1100),
    dict(key='Smoke',prefix='287306',name='M18 · Smoke',legacy='Grenade_Smoke',one=True,payload='SMOKE',damage=0,interval=.8,ammo=1,reserve=0,spread=0,speed=1100),
]
rows={}
with (project/'docs/references/weapon-stats-2069175273.csv').open(newline='') as stream:
    for row in csv.reader(stream):
        try: rows[row[0]]=dict(armor_ratio=float(row[1]),damage=float(row[2]),range_modifier=float(row[3]),shot_interval=float(row[4]))
        except (ValueError,IndexError):pass

def save(asset):
    if not assets.save_loaded_asset(asset,only_if_is_dirty=False):
        raise RuntimeError('Save failed: '+asset.get_path_name())

def import_file(filename,destination,name,options=None):
    existing=ue.load_asset(destination+'/'+name)
    if existing and not (options and reimport):return existing
    task=ue.AssetImportTask()
    for prop,value in dict(filename=str(filename),destination_path=destination,destination_name=name,automated=True,save=True,replace_existing=bool(options and reimport)).items():
        task.set_editor_property(prop,value)
    if options:task.set_editor_property('options',options)
    tools.import_asset_tasks([task])
    result=ue.load_asset(destination+'/'+name)
    if not result:raise RuntimeError('Import failed '+str(filename)+' outputs='+str(task.get_editor_property('imported_object_paths')))
    return result

def texture(directory,filename,label,linear=False,normal=False):
    file=directory/'original/textures'/filename
    if not file.exists():raise RuntimeError('Missing supplied texture '+str(file))
    asset=import_file(file,folder+'/'+label+'/Textures','T_'+label+'_'+filename.split('.')[0])
    asset.set_editor_property('srgb',not linear and not normal)
    if normal:
        asset.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP)
        asset.set_editor_property('flip_green_channel','DirectX' not in filename)
    elif linear:asset.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_MASKS)
    asset.set_editor_property('max_texture_size',2048)
    save(asset)
    return asset

maps={
    'Knife':('knifeColor.png','knifeNormal.png','knifeRoughness.png','knifeMetallic.png'),
    'Pistol':('berettaColor.png','berettaNormal.png','berettaRoughness.png','berettaMetallic.png'),
    'SMG':('SST30_Worn_ODGreen_Base.png','SST30_Worn_ODGreen_Normal.png','SST30_Worn_ODGreen_Roughness.png','SST30_Worn_ODGreen_Metallic.png'),
    'Shotgun':('shotgunColor.png','shotgunNormal.png','shotgunsmoothness.png','shotgunmetallic.png'),
    'Assault':('ar15_Material_BaseColor.tga.png','ar15_Material_Normal.tga.png','ar15_Material_Roughness.tga.png','ar15_Material_Metalness.tga.png'),
    'Carbine':('carbineColor.png','carbineNormal.png',None,'carbineMetallic.png'),
    'Sniper':('sniperColor.png','sniperNormal.png','sniperRoughness.png','sniperMetallic.png'),
    'Launcher':('Parte01_Diffuse.png','Parte01_Normal_DirectX.png','Parte01_Glossiness.png',None),
    'HE':('RGD_albedo.jpg','RGD_normal.png','RGD_roughness.jpg','RGD_metallic.jpg'),
    'Smoke':('DefaultMaterial_albedo.jpg','DefaultMaterial_normal.png','DefaultMaterial_roughness.jpg','DefaultMaterial_metallic.jpg'),
}

def material(directory,key,files,suffix=''):
    name='M_'+key+suffix; path=folder+'/'+key+'/'+name
    result=ue.load_asset(path)
    if result and not repair_materials:return result
    if result:ue.MaterialEditingLibrary.delete_all_material_expressions(result)
    else:result=tools.create_asset(name,folder+'/'+key,ue.Material,ue.MaterialFactoryNew())
    result.set_editor_property('used_with_skeletal_mesh',True)
    properties=[ue.MaterialProperty.MP_BASE_COLOR,ue.MaterialProperty.MP_NORMAL,ue.MaterialProperty.MP_ROUGHNESS,ue.MaterialProperty.MP_METALLIC]
    for index,(file,prop) in enumerate(zip(files,properties)):
        if not file:continue
        sample=ue.MaterialEditingLibrary.create_material_expression(result,ue.MaterialExpressionTextureSample,-400,index*180)
        sample.set_editor_property('texture',texture(directory,file,key,linear=index>1,normal=index==1))
        if index==1:sample.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif index>1:sample.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_MASKS)
        if (key=='Shotgun' or key=='Launcher') and index==2:
            invert=ue.MaterialEditingLibrary.create_material_expression(result,ue.MaterialExpressionOneMinus,-150,index*180)
            if not ue.MaterialEditingLibrary.connect_material_expressions(sample,'R',invert,''):
                raise RuntimeError('Cannot connect glossiness inversion '+name)
            ue.MaterialEditingLibrary.connect_material_property(invert,'',prop)
        else:ue.MaterialEditingLibrary.connect_material_property(sample,'RGB' if index<2 else 'R',prop)
    ue.MaterialEditingLibrary.recompile_material(result);save(result)
    return result

def glass(key):
    name='M_'+key+'_Lens';result=ue.load_asset(folder+'/'+key+'/'+name)
    if result:return result
    result=tools.create_asset(name,folder+'/'+key,ue.Material,ue.MaterialFactoryNew())
    result.set_editor_property('used_with_skeletal_mesh',True)
    result.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT)
    color=ue.MaterialEditingLibrary.create_material_expression(result,ue.MaterialExpressionConstant3Vector,-250,0)
    color.set_editor_property('constant',ue.LinearColor(.025,.04,.06,1))
    opacity=ue.MaterialEditingLibrary.create_material_expression(result,ue.MaterialExpressionConstant,-250,150)
    opacity.set_editor_property('r',.15)
    ue.MaterialEditingLibrary.connect_material_property(color,'',ue.MaterialProperty.MP_BASE_COLOR)
    ue.MaterialEditingLibrary.connect_material_property(opacity,'',ue.MaterialProperty.MP_OPACITY)
    ue.MaterialEditingLibrary.recompile_material(result);save(result)
    return result

report=[];catalog=[]
slots={profile['key']:index for index,profile in enumerate(profiles)}
# The pistol framing uses the actual imported assault rifle as its reference.
for profile in sorted(profiles,key=lambda item:item['key']!='Assault'):
    key=profile['key'];directory=next(root.glob(profile['prefix']+'*'))
    if selected_keys and key not in selected_keys:
        if key not in previous:raise RuntimeError('Partial import requires a complete existing catalog: '+key)
        data=ue.load_asset(previous[key]['data'])
        if not data:raise RuntimeError('Missing saved catalog entry '+key)
        report.append(previous[key]);catalog.append(data)
        continue
    manifest_file=directory/'derived'/('manifest-'+key+'.json')
    if not manifest_file.exists():manifest_file=directory/'derived'/'manifest.json'
    if not manifest_file.exists():
        if staging:
            ue.log_warning('ER_REFERENCE_PENDING '+key)
            continue
        raise RuntimeError('Missing prepared source '+key)
    manifest=json.loads(manifest_file.read_text())
    options=ue.FbxImportUI()
    for prop,value in dict(automated_import_should_detect_type=False,mesh_type_to_import=ue.FBXImportType.FBXIT_SKELETAL_MESH,
                           import_as_skeletal=True,import_mesh=True,import_animations=True,import_materials=False,import_textures=False,create_physics_asset=False).items():
        options.set_editor_property(prop,value)
    # Interchange mesh reimport preserves existing animation packages/reference
    # poses. These calibrated derivatives need a fresh skeleton and clip set.
    version='/V4' if key in ('Assault','SMG') else '/V3'
    model_folder=folder+'/'+key+(version if key not in ('HE','Smoke') else '')
    mesh=import_file(root/manifest['fbx'],model_folder,'SK_'+key,options)
    files=maps.get(key)
    if not files:raise RuntimeError('Launcher material mapping must be supplied after original inspection')
    main=material(directory,key,files)
    interfaces=[]
    for slot in mesh.get_editor_property('materials'):
        name=str(slot.get_editor_property('material_slot_name')).lower()
        selected=main
        if key=='SMG' and '458' in name:
            selected=material(directory,key,tuple('SST30_Handguard_Worn_ODGreen_'+s for s in ['Base.png','Normal.png','Roughness.png','Metallic.png']),'_Handguard')
        if key=='Launcher' and 'lens' not in name:
            part=re.search(r'(parte\d\d(?:_2)?|vertical_grip|bullet)',name)
            if part:
                token=part.group(1)
                stem='Vertical_Grip' if token=='vertical_grip' else 'Bullet' if token=='bullet' else token[0].upper()+token[1:]
                normal=stem+'_Normal_DirectX.png'
                if not (directory/'original/textures'/normal).exists():normal=stem+'_Normal_DirectX.jpeg'
                roughness=stem+'_Glossiness.png' if (directory/'original/textures'/(stem+'_Glossiness.png')).exists() else None
                selected=material(directory,key,(stem+'_Diffuse.png',normal,roughness,None),'_'+stem)
        if 'lens' in name or 'scopereticle' in name:selected=glass(key)
        interfaces.append(selected)
    if not ue.ERCharacter.assign_weapon_materials(mesh,interfaces):raise RuntimeError('Cannot assign owned weapon material '+key)
    save(mesh)
    clips={}
    for path in assets.list_assets(model_folder,recursive=False,include_folder=False):
        asset=ue.load_asset(path)
        if isinstance(asset,ue.AnimSequence):
            for label in manifest['clips']:
                if asset.get_name().endswith('_'+label):clips[label]=asset
    for required in ([] if key in ('HE','Smoke') else ['Idle'])+(['Reload'] if profile.get('row') else []):
        if required not in clips:raise RuntimeError('Missing '+key+' '+required+' clips='+str(clips))
    legacy=ue.load_asset('/Game/EclipseRecoil/Weapons/LowPoly/DA_'+profile['legacy'])
    if not legacy:raise RuntimeError('Missing fitted legacy pose '+profile['legacy'])
    motion=json.loads((directory/'derived'/('motion-'+key+'.json')).read_text())
    hand_scale=.5 if profile.get('one') else .72
    support=motion['support']
    idle_path=folder+'/'+key+'/A_'+key+'_Hands_Idle'
    pose_options={
        'ER_POSE_SOURCE':'/Game/Characters/Mannequins/Anims/'+('Pistol/MF_Pistol_Idle_ADS' if profile.get('one') else 'Rifle/MF_Rifle_Idle_ADS'),
        'ER_POSE_MESH':ue.load_asset('/Game/EclipseRecoil/Characters/Teams/Alpha/FirstPerson/SK_ER_Twinblast_Alpha_Officer_Forearms'),
        'ER_POSE_DESTINATION':idle_path,'ER_POSE_HEIGHT_OFFSET':15,'ER_POSE_HAND_SCALE':hand_scale,
        'ER_POSE_ARM_THICKNESS':.82 if profile.get('one') else .95,'ER_POSE_MANNY_GRIP_ORIENTATION':True,
        'ER_POSE_LEFT_ELBOW_POLE':(0,30,125),
        'ER_POSE_LEFT_GRIP_OFFSET':(support[1]*100*hand_scale,support[0]*100*hand_scale,support[2]*100*hand_scale)}
    pistol_receipt={}
    if key=='Pistol':pose_options.update(pistol_tools['pose_options'](mesh,clips.get('Idle'),pistol_receipt))
    runpy.run_path(str(project/'scripts/create_rifle_pose.py'),init_globals=pose_options)
    data=ue.load_asset(folder+'/'+key+'/DA_'+key)
    if not data:
        factory=ue.DataAssetFactory();factory.set_editor_property('data_asset_class',ue.ERWeaponData)
        data=tools.create_asset('DA_'+key,folder+'/'+key,ue.ERWeaponData,factory)
    settings=dict(display_name=profile['name'],mesh=mesh,idle_animation=ue.load_asset(idle_path),
        fire_animation=legacy.get_editor_property('fire_animation'),one_handed=profile.get('one',False),
        weapon_idle_animation=clips.get('Idle'),weapon_fire_animation=clips.get('Fire'),weapon_reload_animation=clips.get('Reload'),
        grip_transform=ue.Transform(rotation=ue.Rotator(pitch=0,yaw=90,roll=0)),
        first_person_palm_grip=key=='Pistol',first_person_grip_transform=ue.Transform(),
        fit_support_hand=not profile.get('one',False),support_hand_offset=ue.Vector(support[0]*100,-support[1]*100,support[2]*100),
        support_palm_rotation=ue.MathLibrary.make_rot_from_xz(ue.Vector(1,0,0),ue.Vector(0,1,0)) if key=='Launcher' else ue.Rotator(pitch=0,yaw=90,roll=0),
        muzzle_offset=ue.ERCharacter.get_weapon_muzzle_from_geometry(mesh),
        view_offset=legacy.get_editor_property('view_offset'),sound_pitch=legacy.get_editor_property('sound_pitch'),
        automatic=profile.get('auto',False),magazine_size=profile['ammo'],initial_reserve=profile['reserve'],
        reload_duration=profile.get('reload',1.5),standing_spread_degrees=profile['spread'],
        pellets=profile.get('pellets',1),payload=getattr(ue.ERPayload,profile.get('payload','BULLET')),
        zoom_fov=profile.get('zoom',75 if profile.get('one') else 60),
        damage=profile.get('damage',0),shot_interval=profile.get('interval',.1),armor_ratio=1 if profile.get('payload') in ('HE','LAUNCHED_HE') else 2,
        hitscan_range=profile.get('range',60000),range_modifier=1,explosion_radius=350,fuse_duration=2.5,
        projectile_speed=profile.get('speed',30000),projectile_life=8,bloom_per_shot_degrees=0 if profile.get('one') else .12)
    if profile.get('row'):settings.update(rows[profile['row']])
    settings['support_surface_contacts']=palm_tools['surface_contacts'](mesh,key,settings['support_hand_offset'],clips.get('Idle')) if settings['fit_support_hand'] else []
    if settings['support_surface_contacts']:settings['support_hand_offset']=settings['support_surface_contacts'][0]
    right_socket='ER_Palm_R' if key=='Pistol' else 'HandGrip_R';weapon_origin=None
    if key=='Pistol':
        weapon_origin=pistol_tools['frame_settings'](settings,pose_options['ER_POSE_MESH'],pistol_receipt)
    if key=='Launcher':
        projectile_manifest=json.loads((directory/'derived/manifest-Projectile.json').read_text())
        projectile=import_file(root/projectile_manifest['fbx'],folder+'/Launcher/Projectile','SK_M32Projectile',options)
        projectile_material=material(directory,key,('Bullet_Diffuse.png','Bullet_Normal_DirectX.png','Bullet_Glossiness.png',None),'_Projectile')
        if not ue.ERCharacter.assign_weapon_material(projectile,projectile_material):raise RuntimeError('Cannot assign projectile material')
        save(projectile);settings['projectile_mesh']=projectile
    motion_file=directory/'derived'/('motion-'+key+'.json')
    if motion_file.exists() and key not in ('HE','Smoke'):
        motion=json.loads(motion_file.read_text())
        fit_options=dict(weapon_scale=1.,motion_basis=pistol_tools['motion_basis']()) if key=='Pistol' else {}
        for label,property_name in [('Fire','fire_animation'),('Reload','reload_animation')]:
            if label in motion['clips']:
                settings[property_name]=retarget(settings['idle_animation'],folder+'/'+key+'/A_'+key+'_Hands_'+label,
                                                motion['clips'][label],motion['fps'],profile.get('one',False),right_socket,weapon_origin,
                                                **fit_options)
    if key in ('HE','Smoke'):
        frames=[]
        for weight in [0,1,.72,.4,.18,.05,0]:
            frames.append(dict(root=[[1,0,0,-.018*weight],[0,1,0,0],[0,0,1,.003*weight],[0,0,0,1]]))
        settings['fire_animation']=retarget(settings['idle_animation'],folder+'/'+key+'/A_'+key+'_Hands_Fire',frames,60,True)
    for prop,value in settings.items():data.set_editor_property(prop,value)
    save(data);catalog.append(data)
    report.append(dict(key=key,model=mesh.get_path_name(),data=data.get_path_name(),row=profile.get('row'),
        clips={k:dict(path=v.get_path_name(),seconds=v.get_play_length()) for k,v in clips.items()},
        source=manifest['source'],stats={k:settings[k] for k in ['damage','shot_interval','armor_ratio','range_modifier']}))
    ue.log('ER_REFERENCE_WEAPON '+json.dumps(report[-1]))

catalog.sort(key=lambda data:slots[data.get_name().removeprefix('DA_')])
report.sort(key=lambda item:slots[item['key']])
if len(catalog)!=10 and not staging:raise RuntimeError('Incomplete catalog; keeping current loadout')
if not staging:
    for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget']:
        bp=ue.load_asset(path);ue.get_default_object(bp.generated_class()).set_editor_property('reference_arsenal',catalog)
        ue.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
    bp=ue.load_asset('/Game/EclipseRecoil/Core/BP_ERPlayerController')
    defaults=ue.get_default_object(bp.generated_class())
    character_bp=ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter')
    legacy=list(ue.get_default_object(character_bp.generated_class()).get_editor_property('arsenal'))
    defaults.set_editor_property('primary_options',legacy[:7])
    defaults.set_editor_property('reference_arsenal_menu',True)
    ue.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
(root/'import-report.json').write_text(json.dumps(report,indent=2)+'\n')
ue.log('ER_REFERENCE_IMPORT_'+('STAGED' if staging else 'COMPLETE')+' weapons='+str(len(catalog)))
