"""Detach the remaining BlendSpace/AimOffset editor preview meshes, once."""
import hashlib
import json
from pathlib import Path
import unreal as ue

p = Path(ue.Paths.project_dir()).resolve()
a = ue.EditorAssetLibrary
paths = ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget', '/Game/EclipseRecoil/Core/BP_ERCorpse']
root = '/Game/EclipseRecoil/Characters/Teams'
marker = 'ERCharacterPreviewVersion'
bp = ue.load_asset(paths[0])
if a.get_metadata_tag(bp, marker) != '1':
    backup = json.loads((p/'Saved/AssetAudit/character-cleanup-backup.json').read_text())
    assert backup['verified'] and Path(backup['archive']).is_file()
    saved = {r['path']: r for r in backup['files']}
    modified_path = p/'Saved/AssetAudit/character-cleanup-modified.json'
    modified = set(json.loads(modified_path.read_text()))
    d = ue.get_default_object(bp.generated_class())
    sources, replacements = [], []
    for team, hero in [('Alpha','Belica'), ('Omega','Phase')]:
        source = ue.load_asset(root+'/'+team+'/'+hero+'/SK_ER_'+hero+'_Retarget')
        assert source
        sources.append(source)
        replacements.append(d.get_editor_property(team.lower()+'_skins')[5].mesh)
    old = {x.get_path_name().split('.')[0] for x in sources}
    r = ue.AssetRegistryHelpers.get_asset_registry()
    r.search_all_assets(True); r.wait_for_completion()
    options = ue.AssetRegistryDependencyOptions(include_hard_package_references=True,
        include_soft_package_references=True, include_searchable_names=False,
        include_soft_management_references=True, include_hard_management_references=True)
    count = 0
    for data in r.get_assets_by_path(root, recursive=True):
        if not old.intersection(str(x) for x in r.get_dependencies(str(data.package_name), options)):
            continue
        value = data.get_asset()
        if not isinstance(value, (ue.AnimationAsset, ue.AnimBlueprint)):
            continue
        logical = str(data.package_name).replace('/Game/', 'Content/')+'.uasset'
        if logical not in modified:
            with (p/logical).open('rb') as handle:
                assert hashlib.file_digest(handle,'sha256').hexdigest() == saved[logical]['sha256']
        assert ue.ERCharacter.replace_owned_asset_references(value, sources, replacements)
        assert a.save_loaded_asset(value, only_if_is_dirty=False)
        modified.add(logical); count += 1
    for path in paths:
        value = ue.load_asset(path); a.set_metadata_tag(value, marker, '1')
        assert a.save_loaded_asset(value, only_if_is_dirty=False)
    modified_path.write_text(json.dumps(sorted(modified), indent=2)+'\n')
    ue.log('ER_CHARACTER_PREVIEWS_FINALIZED assets=%d' % count)
else:
    ue.log('ER_CHARACTER_PREVIEWS_ALREADY_FINALIZED')
