"""Revision8: standing lateral cover, wider and more numerous in sparse layouts.

Run through Unreal Python after building the editor. Other map/theme/catalog
packages are protected; the owned Village recipe has a verified rollback ZIP.
"""
from pathlib import Path
import hashlib,json,zipfile
import unreal as ue

root=Path(ue.Paths.project_dir()).resolve();private=root/'Assets/maps/Content/EclipseRecoil'
src=root/'Assets/maps/SourceAssets/Village';a=ue.EditorAssetLibrary
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
protected=list((private/'Art/MapTemplates').glob('*.uasset'))+list((private/'Maps').glob('*.umap'))+[private/'Art/Destruction/DA_ER_BlockMaterials.uasset']
protected=[p for p in protected if p.stem!='DA_Village'];before={str(p):sha(p) for p in protected}
template=a.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_Village')
if not template or template.get_editor_property('template_id')!='village.jungle':raise RuntimeError('Unowned or missing Village recipe')
package=private/'Art/MapTemplates/DA_Village.uasset';backup=root/'Saved/VillageBackup/before-side-walls.zip';backup.parent.mkdir(parents=True,exist_ok=True)
if not backup.exists():
    with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
        z.write(package,package.name);z.writestr('hashes.json',json.dumps({package.name:sha(package)}))
with zipfile.ZipFile(backup) as z:
    for name,digest in json.loads(z.read('hashes.json')).items():
        if hashlib.sha256(z.read(name)).hexdigest()!=digest:raise RuntimeError('Invalid side-wall rollback')
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
    if not template.validate_layout(seed):raise RuntimeError('Invalid side-cover seed '+str(seed))
if a.get_metadata_tag(template,'ERVillageSideWallsRevision')!='1':
    template.set_editor_property('revision',template.get_editor_property('revision')+1)
    a.set_metadata_tag(template,'ERVillageSideWallsRevision','1')
    if not a.save_loaded_asset(template,False):raise RuntimeError('Cannot save Village side walls')
for name,digest in before.items():
    if sha(Path(name))!=digest:raise RuntimeError('Protected asset changed '+name)
(src/'side-walls-receipt.json').write_text(json.dumps(dict(revision=template.get_editor_property('revision'),marker=1,seeds=516,protected_sha256=before,backup=str(backup),construction='240cm tall lateral timber walls; 700cm wide / four in one-block maps, 580cm / four in two-block maps, 260cm / two in denser maps; standing bypasses and protected stair approaches',sparse_approach_extra_cm=300),indent=2)+'\n')
receipt=json.loads((src/'recipe.json').read_text());receipt.update(revision=template.get_editor_property('revision'),side_walls='side-walls-receipt.json');(src/'recipe.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('ER_VILLAGE_SIDE_WALLS_COMPLETE seeds=516 protected='+str(len(before)))
