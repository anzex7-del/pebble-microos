#!/usr/bin/env python3
"""Assemble Pages and fingerprint local scripts/styles to avoid mixed releases."""
from pathlib import Path
import hashlib
import re
import shutil

root = Path(__file__).resolve().parents[1]
destination = root / 'dist'
destination.mkdir(exist_ok=True)
for source in (root / 'web').iterdir():
    if source.is_file():
        shutil.copy2(source, destination / source.name)
shutil.copytree(root / 'preview', destination / 'demo', dirs_exist_ok=True)
for page in destination.rglob('*.html'):
    def version_asset(match):
        attribute, name = match.groups()
        asset = page.parent / name
        if asset.suffix not in {'.js', '.css'} or not asset.is_file():
            return match.group(0)
        digest = hashlib.sha256(asset.read_bytes()).hexdigest()[:12]
        return f'{attribute}="{name}?v={digest}"'
    page.write_text(re.sub(r'(src|href)="([^"?:]+)"', version_asset, page.read_text()))
(destination / '.nojekyll').write_text('')
print(f'Static site built at {destination}')
