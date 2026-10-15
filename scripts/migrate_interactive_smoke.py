"""Run in the supplied BulletInteractiveSmoke project with Python enabled.
Use Unreal migration to preserve package references and leave original assets intact.
"""
from pathlib import Path
import json
import unreal as ue

output = Path('/Users/agea/git/respawn-and-rage/core/SourceAssets/InteractiveSmoke')
output.mkdir(parents=True,exist_ok=True)
registry = ue.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
root = '/Game/InteractiveSmoke/Bp_InteractiveSmoke'
options = ue.MigrationOptions()
options.set_editor_property('prompt',False)
options.set_editor_property('ignore_dependencies',False)
options.set_editor_property('asset_conflict',ue.AssetMigrationConflict.SKIP)
ue.AssetToolsHelpers.get_asset_tools().migrate_packages([root],'/Users/agea/git/respawn-and-rage/core/Content',options)
ue.log('ER_SMOKE_MIGRATION_DONE')

bp=ue.load_asset(root)
if not bp: raise RuntimeError('No smoke Blueprint')
ue.BlueprintEditorLibrary.compile_blueprint(bp)
defaults=ue.get_default_object(bp.generated_class())
properties={}
for name in dir(defaults):
    if name.startswith('_'):continue
    try:
        value=defaults.get_editor_property(name)
        if not callable(value):properties[name]=str(value)
    except Exception:pass
ue.log('ER_SMOKE_DEFAULTS '+json.dumps(properties))
methods=[]
for name in dir(defaults):
    if name.startswith('_'):continue
    try:
        if callable(getattr(defaults,name)):methods.append(name)
    except Exception:pass
ue.log('ER_SMOKE_METHODS '+str(methods))
subsystem=ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
components=[]
for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
    data=ue.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    obj=ue.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if not obj:continue
    info={'name':obj.get_name(),'class':obj.get_class().get_name()}
    for name in dir(obj):
        if name.startswith('_'):continue
        try:
            value=obj.get_editor_property(name)
            if not callable(value):info[name]=str(value)
        except Exception:pass
    components.append(info)
ue.log('ER_SMOKE_COMPONENTS '+json.dumps(components))
(output/'inspection.json').write_text(json.dumps({'defaults':properties,'components':components},indent=2)+'\n')
