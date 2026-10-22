"""Explicit revision2: hollow house collision, roof access and graded terrain."""
import hashlib
import json
import zipfile
from pathlib import Path
import unreal as ue

project=Path(ue.Paths.project_dir()).resolve();assets=ue.EditorAssetLibrary
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_MiddleEasternAlleys')
if not template:raise RuntimeError('Create the alley template first')
private=project/'Assets/maps/Content/EclipseRecoil'
protected=[private/p for p in ['Maps/L_Depot.umap','Maps/L_CentralBuilding.umap','Art/MapTemplates/DA_CentralBuilding.uasset','Art/Destruction/DA_ER_BlockMaterials.uasset','Maps/L_MiddleEasternAlleys.umap']]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before={str(p):sha(p) for p in protected}
if assets.get_metadata_tag(template,'ERAlleyTraversalRevision')=='2':
    print('ER_ALLEY_TRAVERSAL_PRESERVED')
else:
    backup=project/'Saved/AlleyTraversalBackup/revision1.zip';backup.parent.mkdir(parents=True,exist_ok=True)
    packages=list((private/'Art/MiddleEasternAlleys').rglob('*.uasset'))+[private/'Art/MapTemplates/DA_MiddleEasternAlleys.uasset']
    if not backup.exists():
        with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as z:
            for p in packages:z.write(p,p.relative_to(private))
            z.writestr('hashes.json',json.dumps({str(p.relative_to(private)):sha(p) for p in packages},indent=2))
    with zipfile.ZipFile(backup) as z:
        for p,s in json.loads(z.read('hashes.json')).items():
            if hashlib.sha256(z.read(p)).hexdigest()!=s:raise RuntimeError('Invalid rollback archive')
    for mesh in template.get_editor_property('town').get_editor_property('building_meshes'):
        if not ue.ERMapTemplate.prepare_alley_house_collision(mesh):raise RuntimeError('Cannot prepare '+mesh.get_name())
        assets.set_metadata_tag(mesh,'ERAlleyShellCollision','2')
        if not assets.save_loaded_asset(mesh,False):raise RuntimeError('Cannot save mesh')
    folder='/Game/EclipseRecoil/Art/MiddleEasternAlleys'
    for name in ['M_ER_HouseConcrete','M_ER_HouseDesert']:
        material=assets.load_asset(folder+'/'+name);material.set_editor_property('two_sided',True)
        ue.MaterialEditingLibrary.recompile_material(material)
        if not assets.save_loaded_asset(material,False):raise RuntimeError('Cannot save inside faces')
    candidate=ue.new_object(ue.ERMapTemplate)
    for field in ['template_id','source_reference','revision','archetype','symmetry','arena_size','town']:
        candidate.set_editor_property(field,template.get_editor_property(field))
    town=candidate.get_editor_property('town');town.set_editor_property('traversable_town',True);candidate.set_editor_property('town',town)
    for seed in list(range(512))+[-17,7919,2147483647,-2147483648]:
        if not candidate.validate_layout(seed):raise RuntimeError('Invalid traversal seed '+str(seed))
    template.set_editor_property('town',town);template.set_editor_property('revision',2)
    assets.set_metadata_tag(template,'ERAlleyTraversalRevision','2')
    if not assets.save_loaded_asset(template,False):raise RuntimeError('Cannot save upgraded template')
    print('ER_ALLEY_TRAVERSAL_SAVED seeds=516')
for p,s in before.items():
    if sha(Path(p))!=s:raise RuntimeError('Protected asset changed '+p)
print('ER_ALLEY_TRAVERSAL_COMPLETE')
