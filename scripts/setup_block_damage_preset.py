"""Apply the 17 October block damage preset once (ERBlockDamagePresetVersion=1).

Run with UnrealEditor-Cmd -run=pythonscript. Updates only MaxHealth, the bullet
and explosion multipliers and blast transmission of the seven profiles. Surface,
impact decal/system and penetration energy are preserved; missing rows are added
from native defaults. Values are a gameplay preset to tune, not physical data.
"""
import unreal as ue

assets = ue.EditorAssetLibrary
path = '/Game/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials'
catalog = assets.load_asset(path)
if not catalog: raise RuntimeError('Missing destruction catalog; run setup_destruction_lab.py first')
M = ue.ERBlockMaterial
# type: (HP per cell, bullet multiplier, explosion multiplier, blast transmission)
preset = {
    M.DRYWALL: (20, 1.0, 2.0, .5), M.BRICK: (100, .75, 1.5, .25), M.CONCRETE: (600, .1, .5, .05),
    M.WOOD: (40, 1.0, 1.5, .5), M.STONE: (1000, .05, .2, .02), M.METAL: (100, .5, 1.0, .5),
    M.GLASS: (8, 1.0, 3.0, .9),
}
if assets.get_metadata_tag(catalog, 'ERBlockDamagePresetVersion') == '1':
    ue.log('ER_BLOCK_DAMAGE_PRESET_PRESERVED')
else:
    rows = list(catalog.get_editor_property('materials'))
    defaults = ue.get_default_object(ue.ERBlockMaterialCatalog).get_editor_property('materials')
    for row in defaults:
        if not any(r.get_editor_property('type') == row.get_editor_property('type') for r in rows): rows.append(row)
    receipt = []
    for row in rows:
        kind = row.get_editor_property('type')
        if kind not in preset: continue
        before = (row.get_editor_property('surface'), row.get_editor_property('impact_decal'), row.get_editor_property('impact_system'))
        hp, bullet, blast, transmission = preset[kind]
        row.set_editor_property('max_health', hp)
        row.set_editor_property('bullet_damage_scale', bullet)
        row.set_editor_property('explosion_damage_scale', blast)
        row.set_editor_property('explosion_transmission', transmission)
        after = (row.get_editor_property('surface'), row.get_editor_property('impact_decal'), row.get_editor_property('impact_system'))
        if before != after: raise RuntimeError('Art references changed for ' + str(kind))
        receipt.append(str(kind) + ' hp=' + str(hp) + ' surface=' + (before[0].get_name() if before[0] else 'none') +
                       ' decal=' + (before[1].get_name() if before[1] else 'none'))
    catalog.set_editor_property('materials', rows)
    assets.set_metadata_tag(catalog, 'ERBlockDamagePresetVersion', '1')
    if not assets.save_loaded_asset(catalog, only_if_is_dirty=False): raise RuntimeError('Cannot save block damage preset')
    for line in receipt: ue.log('ER_BLOCK_DAMAGE_PRESET ' + line)
    ue.log('ER_BLOCK_DAMAGE_PRESET_SAVED rows=' + str(len(rows)))
