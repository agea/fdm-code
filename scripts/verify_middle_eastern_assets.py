"""Read-only saved-asset, material graph and source checks inside Unreal."""
import hashlib
import json
from pathlib import Path
import unreal as ue

project=Path(ue.Paths.project_dir()).resolve();source=project/'Assets/maps/SourceAssets/MiddleEasternAlleys'
assets=ue.EditorAssetLibrary;edit=ue.MaterialEditingLibrary
registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
folder='/Game/EclipseRecoil/Art/MiddleEasternAlleys';problems=[];counts={};report={}
options=ue.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True)
for data in registry.get_assets_by_path(folder,recursive=True):
    obj=data.get_asset();path=str(data.package_name)
    if not obj:problems.append('Missing '+path);continue
    kind=obj.get_class().get_name();counts[kind]=counts.get(kind,0)+1
    for dep in registry.get_dependencies(data.package_name,options):
        dep=str(dep)
        if dep.startswith('/Game/') and (not dep.startswith('/Game/EclipseRecoil/Art/') or not assets.does_asset_exist(dep)):problems.append('Invalid private dependency '+dep)
    if isinstance(obj,ue.StaticMesh):
        if 'House_' in obj.get_name() and assets.get_metadata_tag(obj,'ERAlleyShellCollision')!='2':problems.append('Missing open-shell collision '+path)
        b=obj.get_bounds().box_extent
        if min(b.x,b.y,b.z)<=0:problems.append('Empty mesh '+path)
        report[path]=dict(extent=[b.x,b.y,b.z],materials=[str(m.material_interface.get_path_name()) if m.material_interface else None for m in obj.get_editor_property('static_materials')])
    if isinstance(obj,ue.Material):
        if obj.get_name() in ['M_ER_HouseConcrete','M_ER_HouseDesert'] and not obj.get_editor_property('two_sided'):problems.append('Invisible inside faces '+path)
        if not obj.get_editor_property('used_with_instanced_static_meshes'):problems.append('Missing ISM usage '+path)
        if not edit.get_material_property_input_node(obj,ue.MaterialProperty.MP_BASE_COLOR):problems.append('Unconnected colour '+path)
        if obj.get_name()=='M_ER_ThreeSands':
            samples=[n for n in edit.get_material_expressions(obj) if isinstance(n,ue.MaterialExpressionTextureSample)]
            if len(samples)!=12:problems.append('Expected twelve samples from all three PBR sets')
            for sample in samples:
                if not edit.get_inputs_for_material_expression(obj,sample):problems.append('Unconnected ground UV')
for entry in json.loads((source/'manifest.json').read_text()):
    if hashlib.sha256((source/entry['uid']/'original.zip').read_bytes()).hexdigest()!=entry['archive_sha256']:problems.append('Source archive hash mismatch '+entry['key'])
    for relative,sha in entry['files'].items():
        if hashlib.sha256((source/entry['uid']/relative).read_bytes()).hexdigest()!=sha:problems.append('Source hash mismatch '+relative)
if counts.get('StaticMesh')!=15 or counts.get('Material')!=5 or counts.get('Texture2D')!=14:problems.append('Unexpected asset inventory '+str(counts))
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_MiddleEasternAlleys')
if not template or not template.validate_layout(42):problems.append('Invalid saved template')
if template and template.get_editor_property('revision')>=3:
    if assets.get_metadata_tag(template,'ERAlleyDistrictRevision') not in ('3','4','5') or not template.get_editor_property('town').get_editor_property('seeded_district'):problems.append('Missing district recipe revision3')
if template and template.get_editor_property('revision')>=4:
    town=template.get_editor_property('town')
    if assets.get_metadata_tag(template,'ERAlleyDistrictRevision')not in ('4','5') or town.get_editor_property('min_central_walls')!=0 or town.get_editor_property('max_central_walls')!=2:problems.append('Missing core recipe revision4')
if template and template.get_editor_property('revision')>=5:
    if assets.get_metadata_tag(template,'ERAlleyDistrictRevision')!='5' or not template.get_editor_property('town').get_editor_property('low_cover_and_underpass'):problems.append('Missing passages recipe revision5')
if not assets.does_asset_exist('/Game/EclipseRecoil/Maps/L_MiddleEasternAlleys'):problems.append('Missing saved map')
(project/'Saved/Testing/alleys-assets-report.json').write_text(json.dumps(dict(counts=counts,meshes=report,problems=problems),indent=2)+'\n')
if problems:raise RuntimeError('\n'.join(problems))
print('ER_ALLEYS_ASSETS_PASS '+json.dumps(counts))
