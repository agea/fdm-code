"""Read-only checks for owned town architecture, PBR graphs and private refs."""
import json
from pathlib import Path
import unreal as ue

folder='/Game/EclipseRecoil/Art/MapTemplates/DustTown';assets=ue.EditorAssetLibrary
registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
options=ue.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True)
problems=[];count=0;materials={}
for data in registry.get_assets_by_path(folder,recursive=True):
    obj=data.get_asset();path=str(data.package_name);count+=1
    if not obj:problems.append('Missing '+path)
    for dep in registry.get_dependencies(data.package_name,options):
        dep=str(dep)
        if dep.startswith('/Game/'):
            if not dep.startswith('/Game/EclipseRecoil/Art/'):problems.append('Outside private art '+dep)
            if not assets.does_asset_exist(dep):problems.append('Missing dependency '+dep)
    if isinstance(obj,ue.Material):
        if not obj.get_editor_property('used_with_instanced_static_meshes'):problems.append('Missing ISM shader usage '+path)
        if not ue.MaterialEditingLibrary.get_material_property_input_node(obj,ue.MaterialProperty.MP_BASE_COLOR):problems.append('Unconnected base colour '+path)
        materials[path]=[{'type':str(node.get_editor_property('output_type')),'code':node.get_editor_property('code')} for node in ue.MaterialEditingLibrary.get_material_expressions(obj) if isinstance(node,ue.MaterialExpressionCustom)]
        nodes=ue.MaterialEditingLibrary.get_material_expressions(obj)
        uv=next((n for n in nodes if isinstance(n,ue.MaterialExpressionCustom) and n.get_editor_property('output_type')==ue.CustomMaterialOutputType.CMOT_FLOAT2),None)
        if uv:
            for node in nodes:
                if isinstance(node,ue.MaterialExpressionTextureSample) and uv not in ue.MaterialEditingLibrary.get_inputs_for_material_expression(obj,node):
                    problems.append('Unconnected world UV '+path)
    if isinstance(obj,ue.StaticMesh):
        b=obj.get_bounds().box_extent
        if min(b.x,b.y,b.z)<=0:problems.append('Empty owned mesh '+path)
if count!=8:problems.append('Expected three owned meshes and five town materials, found '+str(count))
Path(ue.Paths.project_dir(),'Saved/Testing/dust-town-material-graphs.json').write_text(json.dumps(materials,indent=2)+'\n')
if problems:raise RuntimeError('\n'.join(problems))
ue.log('ER_DUST_TOWN_ASSETS_PASS packages='+str(count))
