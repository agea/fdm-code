"""Enable instancing on migrated industrial prop materials, never source assets."""
import json
from pathlib import Path
import unreal as ue

assets=ue.EditorAssetLibrary
template=assets.load_asset('/Game/EclipseRecoil/Art/MapTemplates/DA_CentralBuilding')
recipe=template.get_editor_property('central_building')
bases={};instances={}
for field in ['container_mesh','tank_mesh','barrel_mesh']:
    mesh=recipe.get_editor_property(field)
    if not mesh:continue
    for slot in range(len(mesh.get_editor_property('static_materials'))):
        surface=mesh.get_material(slot)
        if not surface:continue
        base=surface.get_base_material()
        if not base.get_path_name().startswith('/Game/EclipseRecoil/Art/FactoryEnvironment/'):
            raise RuntimeError('Refusing material outside migrated Factory assets: '+base.get_path_name())
        bases[base.get_path_name()]=base
        if isinstance(surface,ue.MaterialInstanceConstant):instances[surface.get_path_name()]=surface
changed=[]
for name,base in bases.items():
    if not base.get_editor_property('used_with_instanced_static_meshes'):
        base.set_editor_property('used_with_instanced_static_meshes',True)
        ue.MaterialEditingLibrary.recompile_material(base)
        if not assets.save_loaded_asset(base):raise RuntimeError('Cannot save '+name)
        changed.append(name)
if changed:
    for name,instance in instances.items():
        ue.MaterialEditingLibrary.update_material_instance(instance)
        if not assets.save_loaded_asset(instance,False):raise RuntimeError('Cannot save '+name)
report=dict(result='PASS',changed_bases=changed,checked_bases=list(bases),checked_instances=list(instances))
(Path(ue.Paths.project_saved_dir())/'Testing/industrial-material-usage-report.json').write_text(json.dumps(report,indent=2)+'\n')
print('ER_INDUSTRIAL_MATERIAL_USAGE_PASS '+json.dumps(report))
