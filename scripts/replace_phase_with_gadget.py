"""Remove Phase and Murdock: Gadget takes Omega indices5..9, both teams return to ten skins.

Run in the full editor via -ExecutePythonScript. Only the three appearance catalogs
are saved; meshes, materials, palms and FPS forearms keep their existing assets.
"""
from pathlib import Path
import json
import unreal as ue

BP_PATHS = ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget', '/Game/EclipseRecoil/Core/BP_ERCorpse']
# Phase slot -> Gadget source index, matched by role (Recon, Assault, Patrol, Engineer, Support).
GADGET_ORDER = [12, 10, 11, 13, 14]
MARKER, VERSION = 'ERPhaseGadgetVersion', '1'

report = []
for path in BP_PATHS:
    bp = ue.load_asset(path)
    if ue.EditorAssetLibrary.get_metadata_tag(bp, MARKER) == VERSION:
        print('ER_PHASE_GADGET_SKIP', path); continue
    cdo = ue.get_default_object(bp.generated_class())
    alpha, omega = list(cdo.get_editor_property('alpha_skins')), list(cdo.get_editor_property('omega_skins'))
    if len(alpha) == 10 and [str(s.display_name) for s in omega[5:]] == [str(s.display_name) for s in ue.get_default_object(ue.load_asset(BP_PATHS[0]).generated_class()).get_editor_property('omega_skins')[5:]]:
        # Child blueprints inherit the already migrated parent catalog.
        assert all(str(s.display_name).startswith('Gadget') for s in omega[5:]), path
        ue.EditorAssetLibrary.set_metadata_tag(bp, MARKER, VERSION); ue.EditorAssetLibrary.save_loaded_asset(bp, False)
        print('ER_PHASE_GADGET_INHERITED', path); continue
    assert len(alpha) == 15 and len(omega) == 15, (path, len(alpha), len(omega))
    assert all(str(s.display_name).startswith('Murdock') for s in alpha[10:]), path
    assert all(str(s.display_name).startswith('Phase') for s in omega[5:10]), path
    assert all(str(s.display_name).startswith('Gadget') for s in omega[10:]), path
    omega = omega[:5] + [omega[i] for i in GADGET_ORDER]
    cdo.set_editor_property('alpha_skins', alpha[:10])
    cdo.set_editor_property('omega_skins', omega)
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    ue.EditorAssetLibrary.set_metadata_tag(bp, MARKER, VERSION)
    assert ue.EditorAssetLibrary.save_loaded_asset(bp, False), path
    report.append(dict(path=path, omega=[str(s.display_name) for s in omega]))
    print('ER_PHASE_GADGET_READY', path)
out = Path(ue.Paths.project_dir()) / 'Saved/Testing/phase-gadget-catalog.json'
out.parent.mkdir(parents=True, exist_ok=True); out.write_text(json.dumps(report, indent=2))
print('ER_PHASE_GADGET_COMPLETE alpha=10 omega=10')
