"""Strengthen saved team cloth palettes. Drone colours: setup_drone_team_colors.py.
Run with UnrealEditor-Cmd -run=pythonscript. Owned assets only; preserve originals.
"""
from pathlib import Path
from datetime import datetime
import zipfile
import unreal as ue

assets = ue.EditorAssetLibrary
edit = ue.MaterialEditingLibrary
root = '/Game/EclipseRecoil/Characters/Teams'
selected_teams = globals().get('TEAMS', ('Alpha', 'Omega'))
recon = '/Game/EclipseRecoil/Art/Recon'
project = Path(ue.Paths.project_dir())
backup = project/'Saved/AssetBackups'/('TeamReadability-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
backup.parent.mkdir(parents=True, exist_ok=True)
paths = [p for p in assets.list_assets(root, recursive=True) if p.rsplit('/', 1)[-1].startswith(('M_', 'MI_'))]
with zipfile.ZipFile(backup, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in paths:
        filename = project/'Content'/(path.removeprefix('/Game/').split('.')[0]+'.uasset')
        if filename.exists():
            archive.write(filename, str(filename.relative_to(project)))

def save(asset):
    assert assets.save_loaded_asset(asset, False), asset.get_path_name()

# Existing grounded parameters identify clothing; natural skin/hair remain intact.
# Keep dark neutral gear and trousers, and preserve brightness differences per skin.
count = 0
for path in assets.list_assets(root, recursive=True):
    if not path.rsplit('/', 1)[-1].startswith(('M_', 'MI_')):
        continue
    material = assets.load_asset(path)
    if not isinstance(material, (ue.Material, ue.MaterialInstanceConstant)):
        continue
    team = 'Alpha' if '/Alpha/' in path else 'Omega' if '/Omega/' in path else None
    if team not in selected_teams:
        continue
    if isinstance(material, ue.MaterialInstanceConstant):
        if 'ER_GroundedTint' not in [str(x) for x in edit.get_vector_parameter_names(material)]:
            continue
        color = edit.get_material_instance_vector_parameter_value(material, 'ER_GroundedTint')
    else:
        tint = next((x for x in edit.get_material_expressions(material)
                     if str(x.get_editor_property('desc')) == 'ER_GroundedTint'), None)
        if not tint:
            continue
        color = tint.get_editor_property('default_value')
    dominant = color.b if team == 'Alpha' else color.r
    opposite = color.r if team == 'Alpha' else color.b
    primary_cloth = any(word in material.get_name().lower() for word in ('cloth', 'upperbody', 'torso', 'arms', 'jacket'))
    if not primary_cloth and (dominant < .35 or dominant < opposite*1.35 or (team == 'Omega' and color.g > color.r*.56)):
        continue
    original = assets.get_metadata_tag(material, 'ERTeamReadabilityStrength')
    strength = float(original) if original else max(.70, min(1.0, dominant))
    assets.set_metadata_tag(material, 'ERTeamReadabilityStrength', str(strength))
    target = ue.LinearColor(.10*strength, .30*strength, .72*strength, 1) if team == 'Alpha' else ue.LinearColor(strength, .23*strength, .025*strength, 1)
    if isinstance(material, ue.MaterialInstanceConstant):
        edit.set_material_instance_vector_parameter_value(material, 'ER_GroundedTint', target)
        edit.update_material_instance(material)
    else:
        tint.set_editor_property('default_value', target)
        material.set_editor_property('used_with_skeletal_mesh', True)
        nodes = {str(x.get_editor_property('desc')): x for x in edit.get_material_expressions(material)}
        desat = nodes.get('ER_GroundedDesaturation')
        multiply = nodes.get('ER_GroundedColor')
        if desat and multiply:
            nodes['ER_DesaturationAmount'].set_editor_property('r', 1.0)
            # Retain the cloth texture while lifting its darkest dyed areas.
            # This keeps the team hue readable in shade, without emissive clothing.
            lift = nodes.get('ER_TeamReadabilityLift') or edit.create_material_expression(material, ue.MaterialExpressionLinearInterpolate)
            lift.set_editor_property('desc', 'ER_TeamReadabilityLift')
            lift.set_editor_property('const_b', .65)
            lift.set_editor_property('const_alpha', .30)
            edit.connect_material_expressions(desat, '', lift, 'A')
            edit.connect_material_expressions(lift, '', multiply, 'A')
        edit.recompile_material(material)
    save(material)
    count += 1
assert count > 0, 'No owned team clothing found'

ue.log('ER_TEAM_READABILITY_COMPLETE materials=%d backup=%s' % (count, backup))
