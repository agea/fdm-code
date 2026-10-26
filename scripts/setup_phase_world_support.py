"""Inset only Phase's world support hand; preserve every other skin/profile."""
from pathlib import Path
import hashlib,json,shutil
import unreal as ue

root=Path(ue.Paths.project_dir()).resolve()
assets=ue.EditorAssetLibrary
backup=root/'Saved/PhaseWorldSupportBackup/2026-10-26'
backup.mkdir(parents=True,exist_ok=True)
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
weapons=list((root/'Assets/weapons/Content/EclipseRecoil/Weapons/Sketchfab').glob('*/DA_*.uasset'))
weapon_hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in weapons}

def path(value):return value.get_path_name() if value else None
def xyz(value):return [value.x,value.y,value.z]
def transform(value):
    q=value.rotation
    return [xyz(value.translation),[q.x,q.y,q.z,q.w],xyz(value.scale3d)]
def protected(skin):
    return dict(name=skin.display_name,mesh=path(skin.mesh),fps=path(skin.first_person_mesh),
        animation=path(skin.animation_class),corpse=path(skin.corpse_pose),
        materials=[path(m) for m in skin.materials],hidden_bones=[str(b) for b in skin.hidden_bones],
        hidden_slots=list(skin.hidden_material_slots),scale=skin.scale,feet=skin.feet_offset,
        copy_physics=skin.copy_physics_pose_for_rendering,
        attachments=[[path(p.mesh),path(p.material),str(p.bone),transform(p.transform)] for p in skin.attachments])

receipt=[]
for asset_path in paths:
    blueprint=assets.load_asset(asset_path)
    if not blueprint:raise RuntimeError('Missing '+asset_path)
    source=root/('Assets/maps/Content/'+asset_path.removeprefix('/Game/')+'.uasset')
    saved=backup/source.name
    if not saved.exists():
        shutil.copy2(source,saved)
        if source.read_bytes()!=saved.read_bytes():raise RuntimeError('Backup mismatch '+asset_path)
    defaults=ue.get_default_object(blueprint.generated_class())
    changed=[]
    for team in ('alpha','omega'):
        skins=list(defaults.get_editor_property(team+'_skins'))
        for index,skin in enumerate(skins):
            before=protected(skin)
            phase=bool(skin.mesh and skin.mesh.get_name().startswith('SK_ER_Phase_'))
            offset=skin.get_editor_property('world_support_palm_offset_cm')
            wanted=ue.Vector(0,2,0) if phase else offset
            if not phase and any(abs(v)>.001 for v in xyz(offset)):raise RuntimeError('Unexpected non-Phase offset')
            if phase and any(abs(a-b)>.001 for a,b in zip(xyz(offset),xyz(wanted))):
                skin.set_editor_property('world_support_palm_offset_cm',wanted)
                changed.append(dict(team=team,index=index,mesh=path(skin.mesh),offset_cm=xyz(wanted)))
            if before!=protected(skin):raise RuntimeError('Protected appearance changed '+asset_path)
            skins[index]=skin
        defaults.set_editor_property(team+'_skins',skins)
    if changed:
        ue.BlueprintEditorLibrary.compile_blueprint(blueprint)
        if not assets.save_loaded_asset(blueprint,False):raise RuntimeError('Save failed '+asset_path)
    receipt.append(dict(asset=asset_path,changed=changed,before_sha256=hashlib.sha256(saved.read_bytes()).hexdigest(),after_sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
if weapon_hashes!={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in weapons}:raise RuntimeError('Shared weapon profile changed')
(backup/'receipt.json').write_text(json.dumps(dict(assets=receipt,unchanged_weapon_profiles=weapon_hashes),indent=2)+'\n')
print('ER_PHASE_WORLD_SUPPORT PASS changed='+str(sum(len(r['changed']) for r in receipt)))
