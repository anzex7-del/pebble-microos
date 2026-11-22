"""Source exports must not contain local checkout metadata or private files."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile


class PackageTests(unittest.TestCase):
    def test_source_kit_excludes_checkout_state(self):
        script = Path(__file__).resolve().parents[1] / "tools/package.py"
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "project"
            (root / "tools").mkdir(parents=True)
            (root / "preview").mkdir()
            shutil.copy2(script, root / "tools/package.py")
            (root / "preview/index.html").write_text('<link rel="stylesheet" href="style.css"><script src="app.js"></script>')
            (root / "preview/style.css").write_text("body{color:green}")
            (root / "preview/app.js").write_text("/* pet preview */")
            for name in ("README.md", "profiles/default.json", ".github/workflows/checks.yml",
                         ".git/config", "vendor/sdk.bin", "dist/index.html", ".env",
                         "profiles/private.json", "web/.env.local", "firmware/secret.key",
                         "tools/__pycache__/cache.pyc"):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("fixture")
            subprocess.run([sys.executable, str(root / "tools/package.py")],
                           check=True, capture_output=True)
            with zipfile.ZipFile(Path(temp) / "Pebble_microOS_Starter_Kit.zip") as archive:
                names = set(archive.namelist())
                self.assertIn("pebble-microos/.github/workflows/checks.yml", names)
                self.assertIn("pebble-microos/profiles/default.json", names)
                self.assertIn("pebble-microos/README.md", names)
                self.assertEqual(len(names), 8)
                html = archive.read("pebble-microos/Pebble_Preview.html").decode()
                self.assertIn("<style>body{color:green}</style>", html)
                self.assertIn("<script>/* pet preview */</script>", html)


if __name__ == "__main__":
    unittest.main()
