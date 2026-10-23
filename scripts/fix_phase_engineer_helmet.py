"""Historical entry point: the rejected procedural helmet is now removed."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).with_name('remove_phase_engineer_helmet.py')))
