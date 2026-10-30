"""Historical entry point: preserve the approved authored armored hood."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).with_name('setup_phase_engineer_hood.py')))
