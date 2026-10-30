"""Remove obsolete fallback/preview references without changing selectable appearances.
Run in an isolated, backed-up project with Unreal's Python commandlet.
Only owned packages are saved; no geometry, texture settings or skin choices change.
"""
import json
from pathlib import Path
import unreal as ue

project = Path(ue.Paths.project_dir()).resolve()
registry = ue.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
registry.wait_for_completion()
options = ue.AssetRegistryDependencyOptions(include_hard_package_references=True,
    include_soft_package_references=True, include_searchable_names=False,
    include_soft_management_references=True, include_hard_management_references=True)
root = '/Game/EclipseRecoil/Characters/Teams/'
neutral = ue.load_asset(root+'Gear/M_ER_HiddenSection')
assert neutral
catalog = ue.get_default_object(ue.load_asset('/Game/EclipseRecoil/Core/BP_ERCharacter').generated_class())
alpha, omega = catalog.alpha_skins[0].mesh, catalog.omega_skins[0].mesh
mapping = {
    root+'Alpha/SK_ER_Twinblast_Alpha': alpha,
    root+'Alpha/SK_ER_Twinblast_Alpha_Soldier': alpha,
    root+'Omega/SK_ER_Drongo_Omega': omega,
    root+'Alpha/M_ER_Alpha_M02_Pistol_Grounded': neutral,
    root+'Omega/M_ER_Omega_M04_Weapons_Grounded': neutral,
}
mapping = {p: value for p, value in mapping.items() if ue.EditorAssetLibrary.does_asset_exist(p)}
sources = [ue.load_asset(p) for p in mapping]
assert all(sources)
changed = []
# Freeze registry results before loading/saving packages.
for data in list(registry.get_assets_by_path('/Game/EclipseRecoil', recursive=True)):
    package = str(data.package_name)
    if package in mapping:
        continue
    deps = {str(d) for d in registry.get_dependencies(package, options)}
    if not deps.intersection(mapping):
        continue
    value = data.get_asset()
    assert ue.ERCharacter.replace_owned_asset_references(value, sources, list(mapping.values())), package
    if isinstance(value, ue.Blueprint):
        ue.BlueprintEditorLibrary.compile_blueprint(value)
    assert ue.EditorAssetLibrary.save_loaded_asset(value, only_if_is_dirty=False), package
    changed.append(package)
out = project/'Saved/AssetAudit/character-reference-optimization.json'
out.parent.mkdir(parents=True, exist_ok=True)
if changed or not out.exists():
    out.write_text(json.dumps(dict(mapping={p:v.get_path_name() for p,v in mapping.items()}, changed=changed), indent=2))
ue.log('ER_CHARACTER_REFERENCES_OPTIMIZED count=%d'%len(changed))
