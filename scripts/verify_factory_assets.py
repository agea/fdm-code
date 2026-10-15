"""Read-only Unreal validation of the private Factory dependency closure."""
import hashlib,json
from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir());folder='/Game/EclipseRecoil/Art/FactoryEnvironment'
registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
options=ue.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True)
problems=[];count=0
for data in registry.get_assets_by_path(folder,recursive=True):
    path=str(data.package_name);obj=data.get_asset();count+=1
    if not obj:problems.append('Cannot load '+path)
    for dependency in registry.get_dependencies(data.package_name,options):
        dependency=str(dependency)
        if dependency.startswith('/Game/') and not dependency.startswith(folder+'/'):problems.append('Outside namespace '+path+' -> '+dependency)
        if dependency.startswith(folder+'/') and not ue.EditorAssetLibrary.does_asset_exist(dependency):problems.append('Missing dependency '+dependency)
    if isinstance(obj,ue.StaticMesh):
        for i in range(obj.get_num_sections(0)):
            material=obj.get_material(i)
            if not material:problems.append('Missing material '+path+' slot'+str(i))
            elif not material.get_path_name().startswith(folder+'/'):problems.append('Unexpected material '+material.get_path_name())
receipt=json.loads((root/'Assets/maps/SourceAssets/FactoryEnvironment/template_upgrade.json').read_text())
sha=hashlib.sha256((root/'Assets/maps/Content/EclipseRecoil/Art/Destruction/DA_ER_BlockMaterials.uasset').read_bytes()).hexdigest()
if sha!=receipt['destruction_catalog_sha256']:problems.append('Destruction catalog hash changed')
if problems:raise RuntimeError('\n'.join(problems))
ue.log('ER_FACTORY_VERIFY_PASS packages='+str(count)+' destruction_catalog_sha256='+sha)
