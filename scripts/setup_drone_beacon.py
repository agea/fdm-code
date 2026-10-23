"""Compatibility entry point: the approved drone now uses team emblems."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).with_name('setup_drone_symbols.py')))
