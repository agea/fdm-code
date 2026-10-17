"""Switch only HE art to supplied F1; remove the flash sphere, retaining light/audio."""
from pathlib import Path
import ast,json
import unreal as ue
project=Path(ue.Paths.project_dir()).resolve()
root=project/'Assets/weapons/SourceAssets/SketchfabWeapons'
folder='/Game/EclipseRecoil/Weapons/Sketchfab'
assets=ue.EditorAssetLibrary;tools=ue.AssetToolsHelpers.get_asset_tools()
reimport=False;repair_materials=False
# Reuse import/material helpers without running the arsenal/stat/pose migration.
source=ast.parse((project/'scripts/import_sketchfab_weapons.py').read_text())
helpers=[n for n in source.body if isinstance(n,ast.FunctionDef) and n.name in ('save','import_file','texture','material')]
exec(compile(ast.Module(body=helpers,type_ignores=[]),'import_helpers','exec'))
directory=next(root.glob('287306*'))
manifest=json.loads((directory/'derived/manifest-HE.json').read_text())
assert manifest['materials']==['HE_Material_004'], 'Bake F1 before this migration'
options=ue.FbxImportUI()
for key,value in dict(automated_import_should_detect_type=False,mesh_type_to_import=ue.FBXImportType.FBXIT_SKELETAL_MESH,import_as_skeletal=True,import_mesh=True,import_animations=True,import_materials=False,import_textures=False,create_physics_asset=False).items():options.set_editor_property(key,value)
mesh=import_file(root/manifest['fbx'],folder+'/HE/F1','SK_HE',options)
main=material(directory,'HE',('F1_albedo.jpg','F1_normal.png','F1_roughness.jpg','F1_metallic.jpg'))
assert ue.ERCharacter.assign_weapon_materials(mesh,[main for _ in mesh.get_editor_property('materials')])
save(mesh)
clips=[ue.load_asset(p) for p in assets.list_assets(folder+'/HE/F1',recursive=False,include_folder=False)]
idle=next((a for a in clips if isinstance(a,ue.AnimSequence) and a.get_name().endswith('_Idle')),None)
data=ue.load_asset(folder+'/HE/DA_HE')
data.set_editor_property('mesh',mesh)
data.set_editor_property('weapon_idle_animation',idle)
# Explicitly clear prior-mesh clips, HE has no weapon fire/reload clip.
data.set_editor_property('weapon_fire_animation',None)
data.set_editor_property('weapon_reload_animation',None)
save(data)
he=ue.load_asset('/Game/EclipseRecoil/Effects/Grenade/BP_ERGrenadeExplosion')
cdo=ue.get_default_object(he.generated_class())
audio=cdo.get_editor_property('explosion_sound')
assert audio.get_name()=='S_ER_GrenadeExplosion'
cdo.set_editor_property('flash_mesh',None)
cdo.get_editor_property('flash').set_static_mesh(None)
ue.BlueprintEditorLibrary.compile_blueprint(he);save(he)
assert ue.get_default_object(he.generated_class()).get_editor_property('explosion_sound')==audio
assert cdo.get_editor_property('flash_mesh') is None
assert cdo.get_editor_property('ground_fragment_mesh') is not None
assert cdo.get_editor_property('burst_template') is not None
report_file=root/'import-report.json';report=json.loads(report_file.read_text())
for item in report:
    if item['key']=='HE':
        item['model']=mesh.get_path_name();item['source_model']='F1'
        item['clips']={'Idle':dict(path=idle.get_path_name(),seconds=idle.get_play_length())} if idle else {}
report_file.write_text(json.dumps(report,indent=2)+'\n')
ue.log('ER_F1_READY mesh='+mesh.get_path_name()+' sphere=off light=preserved audio='+audio.get_path_name())
