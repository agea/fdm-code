"""Revision7: rectangular arenas, one to five symmetric blocks, variable bases and upper storeys.

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
package=private/'Art/MapTemplates/DA_Village.uasset';backup=root/'Saved/VillageBackup/before-variety.zip';backup.parent.mkdir(parents=True,exist_ok=True)
if not backup.exists():
    with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
        z.write(package,package.name);z.writestr('hashes.json',json.dumps({package.name:sha(package)}))
with zipfile.ZipFile(backup) as z:
    for name,digest in json.loads(z.read('hashes.json')).items():
        if hashlib.sha256(z.read(name)).hexdigest()!=digest:raise RuntimeError('Invalid variety rollback')
recipe=template.get_editor_property('village')
if a.get_metadata_tag(template,'ERVillageVarietyRevision')!='1':
    for key,value in dict(arena_min=ue.Vector2D(3400,4400),arena_max=ue.Vector2D(5600,13200),min_central_blocks=1,max_central_blocks=5,block_width=ue.Vector2D(2000,3000),block_depth=ue.Vector2D(600,800),min_storeys=1,max_storeys=3,storey_budget=10).items():
        recipe.set_editor_property(key,value)
candidate=ue.new_object(ue.ERMapTemplate)
for key in ['template_id','revision','archetype','symmetry','theme_library','source_reference']:
    candidate.set_editor_property(key,template.get_editor_property(key))
candidate.set_editor_property('village',recipe)
for seed in list(range(512))+[7919,-17,2147483647,-2147483648]:
    if not candidate.validate_layout(seed):raise RuntimeError('Invalid variety seed '+str(seed))
if a.get_metadata_tag(template,'ERVillageVarietyRevision')!='1':
    template.set_editor_property('village',recipe)
    template.set_editor_property('revision',template.get_editor_property('revision')+1)
    a.set_metadata_tag(template,'ERVillageVarietyRevision','1')
    if not a.save_loaded_asset(template,False):raise RuntimeError('Cannot save Village variety')
for name,digest in before.items():
    if sha(Path(name))!=digest:raise RuntimeError('Protected asset changed '+name)
(src/'variety-receipt.json').write_text(json.dumps(dict(revision=template.get_editor_property('revision'),marker=1,seeds=516,protected_sha256=before,backup=str(backup),construction='1..5 blocks, odd origin block self-symmetric, variable widths/depths/stagger, 1..3 real storeys, two ground doglegs per block, three paired base variants',storey_budget=10),indent=2)+'\n')
receipt=json.loads((src/'recipe.json').read_text());receipt.update(revision=template.get_editor_property('revision'),variety='variety-receipt.json');(src/'recipe.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('ER_VILLAGE_VARIETY_COMPLETE seeds=516 protected='+str(len(before)))
