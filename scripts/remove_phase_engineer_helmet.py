"""Remove the rejected procedural helmet from Omega's Phase Engineer only."""
from pathlib import Path
from datetime import datetime
import zipfile
import unreal as ue

assets = ue.EditorAssetLibrary
paths = ['/Game/EclipseRecoil/Core/BP_ERCharacter',
         '/Game/EclipseRecoil/Core/BP_ERTrainingTarget',
         '/Game/EclipseRecoil/Core/BP_ERCorpse']
project = Path(ue.Paths.project_dir())
backup = project/'Saved/AssetBackups'/('PhaseEngineerNoHelmet-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
backup.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(backup, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in paths:
        filename = project/'Content'/(path.removeprefix('/Game/')+'.uasset')
        archive.write(filename, str(filename.relative_to(project)))
for path in paths:
    bp = ue.load_asset(path)
    defaults = ue.get_default_object(bp.generated_class())
    choices = list(defaults.omega_skins)
    appearance = choices[8]
    assert 'PhaseEngineer' in appearance.mesh.get_path_name()
    parts = [p for p in appearance.attachments if 'Helmet' not in p.mesh.get_name()]
    hidden = set(appearance.hidden_material_slots)
    for index, slot in enumerate(appearance.mesh.materials):
        if 'hair_short' in str(slot.material_slot_name).lower():
            hidden.add(index)
    appearance.set_editor_property('attachments', parts)
    appearance.set_editor_property('hidden_material_slots', sorted(hidden))
    choices[8] = appearance
    defaults.set_editor_property('omega_skins', choices)
    assets.set_metadata_tag(bp, 'ERPhaseEngineerHelmetVersion', 'removed-2')
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assert assets.save_loaded_asset(bp, False)
    assert not list(defaults.omega_skins[8].attachments)
ue.log_warning('ER_PHASE_ENGINEER_NO_HELMET_PASS catalogs=3 backup=%s' % backup)
