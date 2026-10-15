"""Wire owned town PBR samples to their world projection using actual pin names."""
import json
from pathlib import Path
import unreal as ue

assets=ue.EditorAssetLibrary;edit=ue.MaterialEditingLibrary
folder='/Game/EclipseRecoil/Art/MapTemplates/DustTown';receipt=[]
for name in ['M_ER_TownPlaster','M_ER_TownPaving','M_ER_TownRoof','M_ER_TownTimber']:
    material=assets.load_asset(folder+'/'+name)
    if not material:raise RuntimeError(name)
    if assets.get_metadata_tag(material,'ERDustTownProjectionRevision')=='1':
        ue.log('ER_DUST_PROJECTION_PRESERVED '+name);continue
    nodes=edit.get_material_expressions(material)
    uv=next(n for n in nodes if isinstance(n,ue.MaterialExpressionCustom) and n.get_editor_property('output_type')==ue.CustomMaterialOutputType.CMOT_FLOAT2)
    for node in nodes:
        if not isinstance(node,ue.MaterialExpressionTextureSample):continue
        names=[str(s) for s in edit.get_material_expression_input_names(node)]
        pin=next((s for s in names if s.lower() in ['uvs','uv','coordinates']),None)
        if pin is None:raise RuntimeError('No UV input '+str(names))
        before=[n.get_class().get_name() if n else None for n in edit.get_inputs_for_material_expression(material,node)]
        if not edit.connect_material_expressions(uv,'',node,pin):raise RuntimeError('Cannot connect '+name+' '+pin)
        after=edit.get_inputs_for_material_expression(material,node)
        if uv not in after:raise RuntimeError('Projection connection did not persist')
        receipt.append({'material':name,'texture':node.get_editor_property('texture').get_path_name(),'pin':pin,'previous_inputs':before})
    edit.recompile_material(material)
    assets.set_metadata_tag(material,'ERDustTownProjectionRevision','1')
    if not assets.save_loaded_asset(material):raise RuntimeError('Cannot save '+name)
if receipt:Path(ue.Paths.project_dir(),'Assets/maps/SourceAssets/MapTemplates/DustTown/projection_repair.json').write_text(json.dumps(receipt,indent=2)+'\n')
ue.log('ER_DUST_PROJECTION_COMPLETE samples='+str(len(receipt)))
