"""Remove project field-pack attachments from every live/target/corpse skin."""
from pathlib import Path
import hashlib,json,shutil
import unreal as ue

project=Path(ue.Paths.project_dir()).resolve();assets=ue.EditorAssetLibrary
paths=['/Game/EclipseRecoil/Core/BP_ERCharacter','/Game/EclipseRecoil/Core/BP_ERTrainingTarget','/Game/EclipseRecoil/Core/BP_ERCorpse']
backup=project/'Saved/SkinBackpackBackup/2026-10-26';backup.mkdir(parents=True,exist_ok=True)
receipt=[]
for path in paths:
    blueprint=assets.load_asset(path)
    if not blueprint:raise RuntimeError('Missing '+path)
    source=project/('Assets/maps/Content/'+path.removeprefix('/Game/')+'.uasset')
    target=backup/source.name
    if not target.exists():shutil.copy2(source,target)
    if not target.exists():raise RuntimeError('Backup failed '+str(source))
    defaults=ue.get_default_object(blueprint.generated_class());removed=[]
    for team in ('alpha','omega'):
        skins=list(defaults.get_editor_property(team+'_skins'))
        for index,skin in enumerate(skins):
            kept=[]
            for part in skin.get_editor_property('attachments'):
                mesh=part.get_editor_property('mesh');name=mesh.get_name().lower() if mesh else ''
                if 'pack' in name or 'backpack' in name:
                    removed.append(dict(team=team,index=index,mesh=mesh.get_path_name()))
                else:kept.append(part)
            skin.set_editor_property('attachments',kept);skins[index]=skin
        defaults.set_editor_property(team+'_skins',skins)
    if removed:
        ue.BlueprintEditorLibrary.compile_blueprint(blueprint)
        if not assets.save_loaded_asset(blueprint,False):raise RuntimeError('Cannot save '+path)
    receipt.append(dict(asset=path,removed=removed,before_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),after_sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
(backup/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('ER_REMOVE_SKIN_BACKPACKS PASS removed='+str(sum(len(x['removed']) for x in receipt)))
