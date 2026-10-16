"""Update only owned explosion assets; preserve smoke and imported Epic source."""
from pathlib import Path
source=Path(__file__).with_name('setup_grenade_effects.py').read_text()
exec(compile(source[:source.index("mat=ue.load_asset")], 'setup_explosion_refinement', 'exec'))
ue.log('ER_EXPLOSION_REFINEMENT_READY fire=off smoke_particles=3 flash_seconds=.065 ground_chips=3..6')
