"""Remove legacy capsule-shadow asset links when that renderer is disabled.

No rendered shadow setting, physics asset, geometry or animation is changed.
"""
import hashlib
import json
from pathlib import Path
import unreal as ue

p = Path(ue.Paths.project_dir()).resolve()
a = ue.EditorAssetLibrary
paths = ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget', '/Game/EclipseRecoil/Core/BP_ERCorpse']
marker = 'ERUnusedCapsuleShadowVersion'
bp = ue.load_asset(paths[0])
if a.get_metadata_tag(bp, marker) != '1':
    receipt = json.loads((p/'Saved/AssetAudit/character-cleanup-backup.json').read_text())
    assert receipt['verified'] and Path(receipt['archive']).is_file()
    saved = {r['path']: r for r in receipt['files']}
    modified_path = p/'Saved/AssetAudit/character-cleanup-modified.json'
    modified = set(json.loads(modified_path.read_text()))
    for path in paths:
        cdo = ue.get_default_object(a.load_blueprint_class(path))
        for component in cdo.get_components_by_class(ue.SkeletalMeshComponent):
            assert not component.get_editor_property('cast_capsule_direct_shadow')
            assert not component.get_editor_property('cast_capsule_indirect_shadow')
    cdo = ue.get_default_object(bp.generated_class())
    meshes = {}
    for team in ['alpha', 'omega']:
        for look in list(cdo.get_editor_property(team+'_skins'))+[cdo.get_editor_property(team+'_appearance')]:
            for mesh in [look.mesh, look.first_person_mesh]:
                if mesh:
                    meshes[mesh.get_path_name()] = mesh
    changed = 0
    for mesh in meshes.values():
        if not mesh.get_editor_property('shadow_physics_asset'):
            continue
        logical = mesh.get_path_name().split('.')[0].replace('/Game/', 'Content/')+'.uasset'
        if logical not in modified:
            with (p/logical).open('rb') as handle:
                assert hashlib.file_digest(handle, 'sha256').hexdigest() == saved[logical]['sha256']
        mesh.set_editor_property('shadow_physics_asset', None)
        assert a.save_loaded_asset(mesh, only_if_is_dirty=False)
        modified.add(logical)
        changed += 1
    for path in paths:
        value = ue.load_asset(path)
        a.set_metadata_tag(value, marker, '1')
        assert a.save_loaded_asset(value, only_if_is_dirty=False)
    modified_path.write_text(json.dumps(sorted(modified), indent=2)+'\n')
    ue.log('ER_UNUSED_CAPSULE_SHADOW_REMOVED meshes=%d' % changed)
else:
    ue.log('ER_UNUSED_CAPSULE_SHADOW_ALREADY_APPLIED')
