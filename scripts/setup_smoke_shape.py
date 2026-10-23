"""Regenerate only the owned smoke material/defaults, preserving HE assets."""
from pathlib import Path
from datetime import datetime
import zipfile
import unreal as ue
project=Path(ue.Paths.project_dir())
backup=project/'Saved/AssetBackups'/('SmokeShape-'+datetime.now().strftime('%Y%m%d-%H%M%S')+'.zip')
backup.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(backup,'w',zipfile.ZIP_DEFLATED) as archive:
    for name in ['M_ER_ContainedSmoke','BP_ERSmokeCloud']:
        path=project/'Content/EclipseRecoil/Effects/Smoke'/(name+'.uasset')
        assert path.exists(),path
        archive.write(path,str(path.relative_to(project)))
source = Path(__file__).with_name('setup_grenade_effects.py').read_text()
bootstrap = source[:source.index("source=ROOT+")]
smoke = source[source.index("mat=ue.load_asset"):source.index("projectile=load(")]
exec(compile(bootstrap + smoke, 'setup_smoke_shape', 'exec'))
ue.log('ER_SMOKE_SHAPE_READY radius_cm=440 height_cm=400 growth_seconds=2.2 fade_seconds=4 moving_contour=1 irregular=replicated_phase')
