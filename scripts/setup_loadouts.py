"""Targeted loadout migration: preserve all approved character assets."""
import unreal as ue
import runpy
import zipfile
from pathlib import Path
from datetime import datetime
project=Path(ue.Paths.project_dir())
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget']
weapons=[f'/Game/EclipseRecoil/Weapons/Sketchfab/{key}/DA_{key}' for key in ('Shotgun','Carbine','Sniper')]
claymore_path='/Game/EclipseRecoil/Weapons/Sketchfab/Claymore/DA_Claymore'
backup=project/'Saved/LoadoutBackups'/datetime.now().strftime('%Y%m%d-%H%M%S.zip')
backup.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as archive:
    for asset in paths+weapons+[claymore_path]:
        filename=project/'Content'/(asset.removeprefix('/Game/')+'.uasset')
        if filename.exists(): archive.write(filename,asset.removeprefix('/Game/')+'.uasset')
claymore=ue.load_asset(claymore_path)
if not claymore:
    claymore=ue.AssetToolsHelpers.get_asset_tools().create_asset('DA_Claymore',claymore_path.rsplit('/',1)[0],ue.ERWeaponData,ue.DataAssetFactory())
assert claymore
claymore.set_editor_property('mesh',ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/Claymore/SK_Claymore'))
claymore.set_editor_property('placed_mesh',ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/Claymore/SM_Claymore'))
# Reuse an existing one-handed presentation rig; no gameplay pawn in previews.
smoke=ue.load_asset('/Game/EclipseRecoil/Weapons/Sketchfab/Smoke/DA_Smoke')
for field in ('idle_animation','grip_transform','first_person_grip_transform','first_person_palm_grip'):
    claymore.set_editor_property(field,smoke.get_editor_property(field))
for prop,value in dict(display_name='Claymore',payload=ue.ERPayload.CLAYMORE,magazine_size=1,initial_reserve=0,automatic=False,one_handed=True).items():
    claymore.set_editor_property(prop,value)
assert ue.EditorAssetLibrary.save_loaded_asset(claymore,only_if_is_dirty=False)
labels=runpy.run_path(str(project/'scripts/presentation_labels.py'))['WEAPON_NAMES']
for key in ('Carbine','Sniper'):
    data=ue.load_asset(f'/Game/EclipseRecoil/Weapons/Sketchfab/{key}/DA_{key}')
    data.set_editor_property('display_name',labels[key]);assert ue.EditorAssetLibrary.save_loaded_asset(data,only_if_is_dirty=False)
runpy.run_path(str(project/'scripts/setup_shotgun.py'))
for path in paths:
    bp=ue.load_asset(path);defaults=ue.get_default_object(bp.generated_class())
    catalog=list(defaults.get_editor_property('reference_arsenal'))
    assert len(catalog)>=10,path
    catalog=catalog[:10]+[claymore]
    defaults.set_editor_property('reference_arsenal',catalog)
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assert ue.EditorAssetLibrary.save_loaded_asset(bp,only_if_is_dirty=False)
runpy.run_path(str(project/'scripts/setup_claymore_lasers.py'))
ue.log('ER_LOADOUT_ASSETS_READY backup='+str(backup))
