#!/usr/bin/env python3
"""Stage original application source. Never flashes or overwrites existing work."""
from pathlib import Path
import shutil
import sys

if len(sys.argv) != 2:
    raise SystemExit("Usage: python3 tools/stage_tuya.py /absolute/path/to/TuyaOpen")
sdk = Path(sys.argv[1]).expanduser().resolve()
board = sdk / "boards/T5AI/WAVESHARE_T5AI_TOUCH_AMOLED_1_75"
if not board.is_dir() or not (sdk / "export.sh").is_file():
    raise SystemExit("Expected TuyaOpen SDK with the WAVESHARE_T5AI_TOUCH_AMOLED_1_75 board.")
target = sdk / "examples/pebble_microos"
if target.exists():
    raise SystemExit(f"Refusing to overwrite existing application: {target}")
source = Path(__file__).resolve().parents[1] / "firmware"
shutil.copytree(source, target)
print(f"Application staged at {target}. Nothing has been built or flashed.")
