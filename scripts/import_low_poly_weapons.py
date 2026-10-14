"""Import the user-supplied Fab FBX/palette from project-local SourceAssets.

Run with Unreal's PythonScript commandlet. Existing assets are preserved.
"""
from pathlib import Path
import json
import unreal as ue

source = Path(ue.Paths.project_dir()).resolve() / 'SourceAssets/LowPolyWeapons'
folder = '/Game/EclipseRecoil/Weapons/LowPoly'
tools = ue.AssetToolsHelpers.get_asset_tools()
assets = ue.EditorAssetLibrary

def import_asset(filename, name, skeletal=False):
    existing = ue.load_asset(folder + '/' + name)
    if existing:
        return existing
    task = ue.AssetImportTask()
    task.set_editor_property('filename', str(filename))
    task.set_editor_property('destination_path', folder)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    if skeletal:
        options = ue.FbxImportUI()
        options.set_editor_property('automated_import_should_detect_type', False)
        options.set_editor_property('mesh_type_to_import', ue.FBXImportType.FBXIT_SKELETAL_MESH)
        options.set_editor_property('import_as_skeletal', True)
        options.set_editor_property('import_mesh', True)
        options.set_editor_property('import_animations', False)
        options.set_editor_property('import_materials', False)
        options.set_editor_property('import_textures', False)
        task.set_editor_property('options', options)
    tools.import_asset_tasks([task])
    result = ue.load_asset(folder + '/' + name)
    if not result:
        raise RuntimeError('Import failed: ' + name)
    return result

texture = import_asset(source / 'T_Gun.TGA', 'T_Gun')
texture.set_editor_property('srgb', True)
texture.set_editor_property('filter', ue.TextureFilter.TF_NEAREST)
assets.save_loaded_asset(texture)
material = ue.load_asset(folder + '/M_LowPolyGun')
if not material:
    material = tools.create_asset('M_LowPolyGun', folder, ue.Material, ue.MaterialFactoryNew())
    sample = ue.MaterialEditingLibrary.create_material_expression(material, ue.MaterialExpressionTextureSample, -300, 0)
    sample.set_editor_property('texture', texture)
    ue.MaterialEditingLibrary.connect_material_property(sample, 'RGB', ue.MaterialProperty.MP_BASE_COLOR)
    roughness = ue.MaterialEditingLibrary.create_material_expression(material, ue.MaterialExpressionConstant, -300, 160)
    roughness.set_editor_property('r', .65)
    ue.MaterialEditingLibrary.connect_material_property(roughness, '', ue.MaterialProperty.MP_ROUGHNESS)
    ue.MaterialEditingLibrary.recompile_material(material)
material.set_editor_property('used_with_skeletal_mesh', True)
ue.MaterialEditingLibrary.recompile_material(material)
assets.save_loaded_asset(material)

report = []
for filename in sorted((source / 'Meshes').glob('*.fbx')):
    mesh = import_asset(filename, filename.stem, skeletal=True)
    # UE 5.8's legacy Materials property is transient; use its native setter
    # to update the serialized mesh material resources.
    if not ue.ERCharacter.assign_weapon_material(mesh, material):
        raise RuntimeError('Cannot assign palette: ' + filename.stem)
    mesh_materials = mesh.get_editor_property('materials')
    assets.save_loaded_asset(mesh)
    skeleton = mesh.get_editor_property('skeleton')
    component = ue.SkeletalMeshComponent()
    component.set_skeletal_mesh_asset(mesh)
    box = mesh.get_imported_bounds()
    minimum, maximum = box.origin - box.box_extent, box.origin + box.box_extent
    info = {'name':filename.stem, 'path':mesh.get_path_name(),
            'skeleton':skeleton.get_path_name(),
            'bones':[str(n) for n in component.get_all_socket_names()],
            'minimum':str(minimum), 'maximum':str(maximum),
            'bone_transforms':{str(n):str(component.get_socket_transform(n)) for n in component.get_all_socket_names()},
            'materials':len(mesh_materials),
            'material_paths':[s.material_interface.get_path_name() if s.material_interface else None for s in mesh.get_editor_property('materials')]}
    if any(s != material.get_path_name() for s in info['material_paths']):
        raise RuntimeError('Palette assignment failed: ' + filename.stem)
    report.append(info)
    ue.log('ER_LOW_POLY_IMPORT ' + json.dumps(info))
(source / 'import-report.json').write_text(json.dumps(report, indent=2) + '\n')
ue.log('ER_LOW_POLY_IMPORT_COMPLETE meshes=%d texture=1 material=1' % len(report))
