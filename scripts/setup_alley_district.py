"""Explicit revision3: seeded balanced streets, squares, 2–4 gates and towers."""
import hashlib
import json
import zipfile
from pathlib import Path
import unreal as ue

project=Path(ue.Paths.project_dir()).resolve();assets=ue.EditorAssetLibrary
private=project/'Assets/maps/Content/EclipseRecoil'
target='/Game/EclipseRecoil/Art/MapTemplates/DA_MiddleEasternAlleys'
template=assets.load_asset(target)
if not template or assets.get_metadata_tag(template,'ERAlleyTraversalRevision')!='2':
    raise RuntimeError('Prepare the traversable alley houses first')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
protected=[private/p for p in ['Maps/L_Depot.umap','Maps/L_CentralBuilding.umap','Art/MapTemplates/DA_CentralBuilding.uasset','Art/Destruction/DA_ER_BlockMaterials.uasset','Maps/L_MiddleEasternAlleys.umap']]
protected+=list((private/'Art/MiddleEasternAlleys').rglob('*.uasset'))
before={str(p):sha(p) for p in protected}
if assets.get_metadata_tag(template,'ERAlleyDistrictRevision') in ('3','4','5','6','7'):
    print('ER_ALLEY_DISTRICT_PRESERVED')
else:
    package=private/'Art/MapTemplates/DA_MiddleEasternAlleys.uasset'
    backup=project/'Saved/AlleyTraversalBackup/revision2.zip';backup.parent.mkdir(parents=True,exist_ok=True)
    if not backup.exists():
        with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
            z.write(package,package.name);z.writestr('hashes.json',json.dumps({package.name:sha(package)}))
    with zipfile.ZipFile(backup) as z:
        for p,s in json.loads(z.read('hashes.json')).items():
            if hashlib.sha256(z.read(p)).hexdigest()!=s:raise RuntimeError('Invalid rollback archive')
    candidate=ue.new_object(ue.ERMapTemplate)
    for key in ['template_id','source_reference','revision','archetype','symmetry','arena_size','town']:
        candidate.set_editor_property(key,template.get_editor_property(key))
    candidate.set_editor_property('arena_size',ue.Vector(9400,10600,1600))
    town=candidate.get_editor_property('town')
    for key,value in [('seeded_district',True),('arena_min',ue.Vector2D(8000,8800)),('arena_max',ue.Vector2D(9400,10600)),('min_accesses',2),('max_accesses',4)]:town.set_editor_property(key,value)
    candidate.set_editor_property('town',town)
    invalid=[seed for seed in list(range(512))+[-17,7919,2147483647,-2147483648] if not candidate.validate_layout(seed)]
    if invalid:raise RuntimeError('Invalid district seeds '+str(invalid))
    template.set_editor_property('town',town);template.set_editor_property('arena_size',candidate.get_editor_property('arena_size'));template.set_editor_property('revision',3)
    assets.set_metadata_tag(template,'ERAlleyDistrictRevision','3')
    if not assets.save_loaded_asset(template,False):raise RuntimeError('Cannot save district recipe')
    print('ER_ALLEY_DISTRICT_SAVED seeds=516')
for p,s in before.items():
    if sha(Path(p))!=s:raise RuntimeError('Protected asset changed '+p)
print('ER_ALLEY_DISTRICT_COMPLETE')
