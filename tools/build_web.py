#!/usr/bin/env python3
from pathlib import Path
import shutil

root = Path(__file__).resolve().parents[1]
destination = root / "dist"
destination.mkdir(exist_ok=True)
for source in (root / "web").iterdir():
    if source.is_file():
        shutil.copy2(source, destination / source.name)
shutil.copytree(root / "preview", destination / "demo", dirs_exist_ok=True)
(destination / ".nojekyll").write_text("")
print(f"Static site built at {destination}")
