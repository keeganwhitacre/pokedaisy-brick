"""Compatibility entry point; shared tables belong to the portable C core."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).resolve().parents[1]/'3ds/tools/gen_experience.py'), run_name='__main__')
