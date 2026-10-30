"""Apply the approved armored hood to Omega Grenadier / Phase Engineer only."""
from pathlib import Path
from datetime import datetime
import zipfile
import unreal as ue

assets = ue.EditorAssetLibrary
project = Path(ue.Paths.project_dir()).resolve()
paths = ['/Game/EclipseRecoil/Core/BP_ERCharacter',
         '/Game/EclipseRecoil/Core/BP_ERTrainingTarget',
         '/Game/EclipseRecoil/Core/BP_ERCorpse']
folder = '/Game/EclipseRecoil/Characters/Teams/Omega/Skins/PhaseEngineer'
gear = '/Game/EclipseRecoil/Characters/Teams/Gear'
mesh_path = folder + '/SM_ER_PhaseEngineer_ArmoredHood'
material_paths = [folder + '/M_ER_PhaseEngineer_Hood',
                  folder + '/M_ER_PhaseEngineer_Visor']
backup = project/'Saved/AssetBackups'/('PhaseEngineerArmoredHood-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
backup.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(backup, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in paths + [mesh_path] + material_paths:
        file = project/'Content'/(path.removeprefix('/Game/')+'.uasset')
        if file.exists():
            archive.write(file, str(file.relative_to(project)))
with zipfile.ZipFile(backup) as archive:
    assert archive.testzip() is None, 'Invalid rollback archive'

# Keep the approved study intact. Production owns its mesh and both materials.
materials = []
for target, source in zip(material_paths, ['M_ER_PhaseHood_Study', 'M_ER_PhaseVisor_Study']):
    material = assets.load_asset(target) if assets.does_asset_exist(target) else assets.duplicate_asset(gear+'/'+source, target)
    assert material, target
    assert assets.save_loaded_asset(material, False)
    materials.append(material)
mesh = assets.load_asset(mesh_path) if assets.does_asset_exist(mesh_path) else assets.duplicate_asset(gear+'/SM_ER_PhaseHood_Study', mesh_path)
assert mesh, mesh_path
slots = list(mesh.get_editor_property('static_materials'))
assert len(slots) == 2, 'The approved hood has separate hood and face-plate materials'
for slot, material in zip(slots, materials):
    slot.set_editor_property('material_interface', material)
mesh.set_editor_property('static_materials', slots)
assets.set_metadata_tag(mesh, 'ERPhaseEngineerHoodVersion', '1')
assets.set_metadata_tag(mesh, 'ERSourceGeometry', gear+'/SM_ER_PhaseHood_Study')
assert assets.save_loaded_asset(mesh, False)

for path in paths:
    bp = assets.load_asset(path)
    defaults = ue.get_default_object(bp.generated_class())
    choices = list(defaults.omega_skins)
    appearance = choices[8]
    assert appearance.display_name == 'Phase · Engineer' and 'PhaseEngineer' in appearance.mesh.get_path_name()
    parts = [p for p in appearance.attachments
             if p.mesh != mesh and 'Helmet' not in p.mesh.get_name()
             and not p.mesh.get_name().startswith('SM_ER_PhaseHood')]
    parts.append(ue.ERAppearanceAttachment(mesh=mesh, bone='head', transform=ue.Transform()))
    hidden = set(appearance.hidden_material_slots)
    hidden.update(i for i, slot in enumerate(appearance.mesh.materials)
                  if 'hair' in str(slot.material_slot_name).lower())
    appearance.set_editor_property('attachments', parts)
    appearance.set_editor_property('hidden_material_slots', sorted(hidden))
    choices[8] = appearance
    defaults.set_editor_property('omega_skins', choices)
    assets.set_metadata_tag(bp, 'ERPhaseEngineerHoodVersion', '1')
    assets.set_metadata_tag(bp, 'ERPhaseEngineerHelmetVersion', 'armored-hood-1')
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assert assets.save_loaded_asset(bp, False), path
    ue.log('ER_PHASE_ENGINEER_HOOD_APPLIED catalog='+path)
ue.log('ER_PHASE_ENGINEER_HOOD_COMPLETE catalogs=3 backup='+str(backup))
