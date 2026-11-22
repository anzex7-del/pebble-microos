#!/usr/bin/env python3
"""Bundle the offline preview and source kit; no network or device access."""
from pathlib import Path
import hashlib
import zipfile

root = Path(__file__).resolve().parents[1]
html = (root / "preview/index.html").read_text()
html = html.replace('<link rel="stylesheet" href="style.css">', '<style>' + (root / "preview/style.css").read_text() + '</style>')
html = html.replace('<script src="app.js"></script>', '<script>' + (root / "preview/app.js").read_text() + '</script>')
(root / "Pebble_Preview.html").write_text(html)
archive = root.parent / "Pebble_microOS_Starter_Kit.zip"
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
    for p in sorted(root.rglob("*")):
        if p.is_file() and not any(x in p.relative_to(root).parts for x in ["build", "__pycache__", ".DS_Store"]):
            z.write(p, "pebble-microos/" + str(p.relative_to(root)))
print(f"Created {archive.name}: {archive.stat().st_size} bytes")
print("SHA256 " + hashlib.sha256(archive.read_bytes()).hexdigest())
