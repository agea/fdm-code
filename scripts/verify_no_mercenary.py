"""Read-only fresh-process checks, also usable in the no-mercenary fixture."""
from pathlib import Path
import runpy
import unreal as ue

project = Path(ue.Paths.project_dir()).resolve()
scripts = Path(__file__).resolve().parent
runpy.run_path(str(scripts / 'verify_skin_first_person_forearms.py'))
runpy.run_path(str(scripts / 'verify_team_skins.py'))
runpy.run_path(str(scripts / 'audit-assets.py'))
import json
report = json.loads((project / 'Saved/AssetAudit/asset-audit.json').read_text())
legacy = ('/Game/Mercenary_Male_WaveforceStudio/', '/Game/EclipseRecoil/Characters/Mercenary/')
live = [p['package'] for p in report['packages'] if p['status'] in ('runtime_reachable', 'authoring_reachable')]
assert not any(p.startswith(legacy) for p in live), 'A legacy Mercenary package is still reachable'
for package, dependencies in report['reachable_dependencies'].items():
    assert not any(p.startswith(legacy) for p in dependencies), (package, 'legacy dependency')
for path in ['/Game/EclipseRecoil/Core/BP_ERCharacter', '/Game/EclipseRecoil/Core/BP_ERTrainingTarget']:
    cdo = ue.get_default_object(ue.EditorAssetLibrary.load_blueprint_class(path))
    assert not cdo.alpha_body_materials and not cdo.omega_body_materials
    assert not cdo.alpha_first_person_materials and not cdo.omega_first_person_materials
    assert cdo.mesh.get_skeletal_mesh_asset() == cdo.alpha_skins[0].mesh
    assert cdo.first_person_body.get_skeletal_mesh_asset() == cdo.alpha_skins[0].first_person_mesh
ue.log('ER_NO_MERCENARY_SAVED_COMPLETE reachable=%d legacy_dependencies=0' % len(live))
