"""Regenerate only the owned smoke material/defaults, preserving HE assets."""
from pathlib import Path
source = Path(__file__).with_name('setup_grenade_effects.py').read_text()
bootstrap = source[:source.index("source=ROOT+")]
smoke = source[source.index("mat=ue.load_asset"):source.index("projectile=load(")]
exec(compile(bootstrap + smoke, 'setup_smoke_shape', 'exec'))
ue.log('ER_SMOKE_SHAPE_READY radius_cm=400 height_cm=400 growth_seconds=2.2 fade_seconds=4 moving_contour=1 irregular=replicated_phase')
