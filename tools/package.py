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

# Package project sources, never a checkout's Git metadata, SDK, generated output
# or a visitor's private profile. Keep this list explicit as the project grows.
source_dirs = ("preview", "firmware", "web", "docs", "tests", "tools", ".github")
source_files = ("README.md", "CONTRIBUTING.md", "THIRD_PARTY_NOTICES.md",
                ".gitignore", "profiles/default.json", "Pebble_Preview.html")
excluded = {"build", "dist", "vendor", "node_modules", "__pycache__", ".git", ".DS_Store"}
candidates = [root / name for name in source_files]
for name in source_dirs:
    candidates.extend((root / name).rglob("*"))
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
    for p in sorted(set(candidates)):
        relative = p.relative_to(root)
        private = any(part.startswith(".env") or part.endswith((".pem", ".key")) for part in relative.parts)
        if (p.is_file() and not p.is_symlink() and root in p.resolve().parents
                and not excluded.intersection(relative.parts) and not private):
            z.write(p, "pebble-microos/" + str(relative))
print(f"Created {archive.name}: {archive.stat().st_size} bytes")
print("SHA256 " + hashlib.sha256(archive.read_bytes()).hexdigest())
