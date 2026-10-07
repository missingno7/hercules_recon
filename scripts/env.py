"""Local analysis dependencies; never mutate the originals or download implicitly."""
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEPS = Path(os.environ.get("HERCULES_PYTHON_DEPS", r"C:\tools\hercules\python"))
if DEPS.is_dir():
    sys.path.insert(0, str(DEPS))

