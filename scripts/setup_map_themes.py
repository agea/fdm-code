"""Bind the four owned production recipes to independent shared visual themes.
Run through UnrealEditor -run=pythonscript. Existing art/catalog stay untouched.
"""
from pathlib import Path
import hashlib
import json
import zipfile
import unreal as ue

root = Path(ue.Paths.project_dir()).resolve()
a = ue.EditorAssetLibrary
base = '/Game/EclipseRecoil/Art/MapTemplates/'
names = ['CentralBuilding', 'MiddleEasternAlleys', 'FireTraining', 'Rooms']
recipes = [a.load_asset(base + 'DA_' + n) for n in names]
if not all(recipes):
    raise RuntimeError('Missing production recipe')
files = [root / ('Assets/maps/Content/EclipseRecoil/Art/MapTemplates/DA_' + n + '.uasset') for n in names]
catalog_file = root/'Assets/maps/Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset'
def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()
protected = sha(catalog_file)
backup = root/'Saved/MapThemesBackup'
backup.mkdir(parents=True, exist_ok=True)
archive = backup/'before-shared-themes-v1.zip'
if not archive.exists():
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
        for p in files:
            z.write(p, str(p.relative_to(root)))
    with zipfile.ZipFile(archive) as z:
        for p in files:
            if hashlib.sha256(z.read(str(p.relative_to(root)))).hexdigest() != sha(p):
                raise RuntimeError('Backup mismatch')
path = base + 'DA_MapThemes'
library = a.load_asset(path) if a.does_asset_exist(path) else None
if library is None:
    central = recipes[0].get_editor_property('central_building')
    town = recipes[1].get_editor_property('town')
    fire = recipes[2].get_editor_property('fire_training')
    defaults = list(a.load_asset('/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials').get_editor_property('materials'))
    fallback = [p.get_editor_property('surface') for p in defaults]
    if len(fallback) != 7 or not all(fallback):
        raise RuntimeError('Incomplete material catalog')
    def get(r, k):
        return r.get_editor_property(k)
    def first(r, k):
        return list(get(r, k))[0]
    palettes = [
        ('Industrial', first(central,'concrete_surfaces'), get(central,'deck_surface'), get(central,'steel_surface'), first(central,'wood_surfaces'), first(central,'brick_surfaces'), list(get(central,'ground_surfaces'))),
        ('Desert', get(town,'wall_surface'), get(town,'road_surface'), get(town,'roof_surface'), get(town,'wood_surface'), get(town,'wall_surface'), [get(town,'ground_surface')]),
        ('Warehouse', get(fire,'steel_surface'), get(fire,'ground_surface'), get(fire,'deck_surface'), first(fire,'wood_surfaces'), first(fire,'concrete_surfaces'), [get(fire,'ground_surface')]),
    ]
    themes = []
    for name, wall, floor, ceiling, wood, brick, grounds in palettes:
        theme = ue.ERMapThemePalette()
        materials = fallback.copy()
        materials[1:6] = [brick, wall, wood, wall, get(town,'iron_surface') if name == 'Desert' else get(central if name == 'Industrial' else fire,'steel_surface')]
        for key, value in [('name',name), ('materials',materials), ('wall',wall), ('floor',floor), ('ceiling',ceiling), ('grounds',grounds)]:
            theme.set_editor_property(key,value)
        themes.append(theme)
    factory = ue.DataAssetFactory()
    factory.set_editor_property('data_asset_class', ue.ERMapThemeLibrary)
    library = ue.AssetToolsHelpers.get_asset_tools().create_asset('DA_MapThemes',base.rstrip('/'),ue.ERMapThemeLibrary,factory)
    library.set_editor_property('themes',themes)
    if not a.save_loaded_asset(library,False):
        raise RuntimeError('Cannot save theme library')
for recipe in recipes:
    bound = recipe.get_editor_property('theme_library')
    if bound and bound != library:
        raise RuntimeError('Recipe has another theme library')
    if bound:
        continue
    recipe.set_editor_property('theme_library',library)
    for seed in [0, 13, 42, 73, 511, -17, 2147483647, -2147483648]:
        if not recipe.validate_layout(seed):
            raise RuntimeError('Invalid themed recipe seed=' + str(seed))
    recipe.set_editor_property('revision',recipe.get_editor_property('revision')+1)
    a.set_metadata_tag(recipe,'ERSharedThemesRevision','1')
    if not a.save_loaded_asset(recipe,False):
        raise RuntimeError('Cannot save recipe')
if sha(catalog_file) != protected:
    raise RuntimeError('Shared damage catalog changed')
(backup/'receipt.json').write_text(json.dumps(dict(library=path, themes=['Industrial','Desert','Warehouse'], recipes={n:sha(p) for n,p in zip(names,files)}, catalog_sha256=protected, backup=str(archive)),indent=2)+'\n')
print('ER_MAP_THEMES_SETUP_COMPLETE templates=4 themes=3')
