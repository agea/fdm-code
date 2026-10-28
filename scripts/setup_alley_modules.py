"""Revision7: dimensioned procedural houses, clear interiors and climbable roof rims."""
import hashlib
import json
import zipfile
from pathlib import Path
import unreal as ue

root = Path(ue.Paths.project_dir()).resolve()
private = root / 'Assets/maps/Content/EclipseRecoil'
a = ue.EditorAssetLibrary
target = '/Game/EclipseRecoil/Art/MapTemplates/DA_MiddleEasternAlleys'
template = a.load_asset(target)
if not template or a.get_metadata_tag(template, 'ERAlleyDistrictRevision') not in ('6', '7'):
    raise RuntimeError('Prepare revision6 destructible Alleys first')
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
package = private / 'Art/MapTemplates/DA_MiddleEasternAlleys.uasset'
previous_hash = sha(package)
already_prepared = a.get_metadata_tag(template, 'ERAlleyDistrictRevision') == '7'
protected = list((private / 'Art/MapTemplates').rglob('*.uasset'))
protected += list((private / 'Maps').glob('*.umap'))
protected += list((private / 'Art/MiddleEasternAlleys').rglob('*.uasset'))
protected += [private / 'Art/Destruction/DA_ER_BlockMaterials.uasset']
before = {str(p): sha(p) for p in protected if p != package}
backup = root / 'Saved/AlleyTraversalBackup/before-procedural-houses.zip'
backup.parent.mkdir(parents=True, exist_ok=True)
if not backup.exists():
    with zipfile.ZipFile(backup, 'w', zipfile.ZIP_DEFLATED) as z:
        z.write(package, package.name)
        z.writestr('hashes.json', json.dumps({package.name: sha(package)}))
with zipfile.ZipFile(backup) as z:
    for name, digest in json.loads(z.read('hashes.json')).items():
        if hashlib.sha256(z.read(name)).hexdigest() != digest:
            raise RuntimeError('Invalid procedural-house rollback')
candidate = ue.new_object(ue.ERMapTemplate)
for field in ['template_id', 'source_reference', 'revision', 'archetype', 'symmetry', 'arena_size', 'town', 'theme_library']:
    candidate.set_editor_property(field, template.get_editor_property(field))
town = candidate.get_editor_property('town')
town.set_editor_property('procedural_houses', True)
candidate.set_editor_property('town', town)
for seed in list(range(512)) + [-17, 7919, 2147483647, -2147483648]:
    if not candidate.validate_layout(seed):
        raise RuntimeError('Invalid procedural-house seed ' + str(seed))
if not already_prepared:
    template.set_editor_property('town', town)
    template.set_editor_property('revision', template.get_editor_property('revision') + 1)
    a.set_metadata_tag(template, 'ERAlleyDistrictRevision', '7')
    if not a.save_loaded_asset(template, False):
        raise RuntimeError('Cannot save procedural-house recipe')
elif sha(package) != previous_hash:
    raise RuntimeError('Prepared recipe changed during validation')
for name, digest in before.items():
    if sha(Path(name)) != digest:
        raise RuntimeError('Protected asset changed ' + name)
receipt = dict(revision=template.get_editor_property('revision'), seeds=516,
               protected_sha256=before, backup=str(backup),
               door_cm=[160, 240], roof_rim_cm=[40, 25], floor_rise_cm=15,
               roofs='flat or low rim; 180cm openings at stairs and adjacent roof crossings',
               idempotent_recipe=already_prepared, recipe_sha256=sha(package))
(root / 'Assets/maps/SourceAssets/MiddleEasternAlleys/procedural-houses-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print('ER_ALLEY_MODULES_COMPLETE seeds=516 protected=' + str(len(before)))
