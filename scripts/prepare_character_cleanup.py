"""Detach obsolete authoring references, preserving the approved runtime look.

Requires a verified complete character archive. No asset is deleted here.
"""
import hashlib
import json
import runpy
from pathlib import Path
import unreal as ue

project = Path(ue.Paths.project_dir()).resolve()
assets = ue.EditorAssetLibrary
root = '/Game/EclipseRecoil/Characters/Teams'
receipt = json.loads((project/'Saved/AssetAudit/character-cleanup-backup.json').read_text())
assert receipt['verified'] and Path(receipt['archive']).is_file()
backed_up = {r['path']: r for r in receipt['files']}
marker = 'ERCharacterRuntimeClosureVersion'
paths = ['/Game/EclipseRecoil/Core/BP_ERCharacter',
         '/Game/EclipseRecoil/Core/BP_ERTrainingTarget',
         '/Game/EclipseRecoil/Core/BP_ERCorpse']
bp = ue.load_asset(paths[0])
if assets.get_metadata_tag(bp, marker) == '1':
    ue.log('ER_CHARACTER_CLEANUP_PREP_ALREADY_APPLIED')
else:
    modified = set()
    def save(value):
        logical = value.get_path_name().split('.')[0].replace('/Game/', 'Content/')+'.uasset'
        if logical not in modified and (project/logical).is_file():
            record = backed_up[logical]
            with (project/logical).open('rb') as handle:
                assert hashlib.file_digest(handle, 'sha256').hexdigest() == record['sha256'], logical
        assert assets.save_loaded_asset(value, only_if_is_dirty=False)
        modified.add(logical)

    defaults = ue.get_default_object(bp.generated_class())
    material_path = root+'/Gear/M_ER_HiddenSection'
    assert not assets.does_asset_exist(material_path), 'Unexpected existing migration material'
    neutral = ue.AssetToolsHelpers.get_asset_tools().create_asset('M_ER_HiddenSection', root+'/Gear', ue.Material, ue.MaterialFactoryNew())
    edit = ue.MaterialEditingLibrary
    zero = edit.create_material_expression(neutral, ue.MaterialExpressionConstant)
    zero.set_editor_property('r', 0)
    edit.connect_material_property(zero, '', ue.MaterialProperty.MP_BASE_COLOR)
    edit.connect_material_property(zero, '', ue.MaterialProperty.MP_EMISSIVE_COLOR)
    rough = edit.create_material_expression(neutral, ue.MaterialExpressionConstant)
    rough.set_editor_property('r', 1)
    edit.connect_material_property(rough, '', ue.MaterialProperty.MP_ROUGHNESS)
    edit.recompile_material(neutral)
    assert assets.save_loaded_asset(neutral, only_if_is_dirty=False)

    catalogs = {}
    previews = {}
    for team in ['Alpha', 'Omega']:
        choices = list(defaults.get_editor_property(team.lower()+'_skins'))
        assert len(choices) == 10
        for appearance in choices:
            previews.setdefault(appearance.mesh.skeleton.get_path_name(), appearance.mesh)
            materials = list(appearance.materials)
            for slot in appearance.hidden_material_slots:
                materials[slot] = neutral
            appearance.set_editor_property('materials', materials)
            for mesh in [appearance.mesh, appearance.first_person_mesh]:
                slots = list(mesh.materials)
                assert len(slots) == len(materials)
                for slot, material in zip(slots, materials):
                    slot.set_editor_property('material_interface', material)
                mesh.set_editor_property('materials', slots)
                save(mesh)
        catalogs[team] = choices
    for preview in previews.values():
        assert ue.ERCharacter.prepare_team_skeleton(preview.skeleton, preview)
        save(preview.skeleton)

    # Retarget preview meshes are editor construction inputs, not runtime skins.
    sources, replacements = [], []
    for team, hero in [('Alpha', 'Belica'), ('Omega', 'Phase')]:
        source = ue.load_asset(root+'/'+team+'/'+hero+'/SK_ER_'+hero+'_Retarget')
        if source:
            sources.append(source)
            replacements.append(catalogs[team][5].mesh)
    registry = ue.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    registry.wait_for_completion()
    options = ue.AssetRegistryDependencyOptions(include_hard_package_references=True,
        include_soft_package_references=True, include_searchable_names=False,
        include_soft_management_references=True, include_hard_management_references=True)
    old_packages = {a.get_path_name().split('.')[0] for a in sources}
    for data in registry.get_assets_by_path(root, recursive=True):
        package = str(data.package_name)
        if old_packages.intersection(str(d) for d in registry.get_dependencies(package, options)):
            value = data.get_asset()
            if not isinstance(value, (ue.AnimationAsset, ue.AnimBlueprint)):
                continue
            assert ue.ERCharacter.replace_owned_asset_references(value, sources, replacements)
            save(value)
    for path in paths:
        value = ue.load_asset(path)
        cdo = ue.get_default_object(value.generated_class())
        for team, choices in catalogs.items():
            cdo.set_editor_property(team.lower()+'_skins', choices)
        ue.BlueprintEditorLibrary.compile_blueprint(value)
        assets.set_metadata_tag(value, marker, '1')
        save(value)
    (project/'Saved/AssetAudit/character-cleanup-modified.json').write_text(json.dumps(sorted(modified), indent=2)+'\n')
    ue.log('ER_CHARACTER_CLEANUP_PREP_COMPLETE modified=%d' % len(modified))

runpy.run_path(str(Path(__file__).with_name('normalize_character_names.py')))
runpy.run_path(str(Path(__file__).with_name('clear_unused_character_shadows.py')))
runpy.run_path(str(Path(__file__).with_name('finalize_character_previews.py')))
