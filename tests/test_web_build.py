from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


class WebBuildTests(unittest.TestCase):
    def test_new_scripts_change_asset_urls_and_keep_offline_source(self):
        script = Path(__file__).resolve().parents[1] / 'tools/build_web.py'
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for name in ['tools', 'web', 'preview']:
                (root / name).mkdir()
            shutil.copy2(script, root / 'tools/build_web.py')
            original = '<link href="style.css"><script src="app.js"></script><a href="https://example.com">Link</a>'
            for name in ['web', 'preview']:
                (root / name / 'index.html').write_text(original)
                (root / name / 'app.js').write_text('let v=1;')
                (root / name / 'style.css').write_text('body{color:green}')
            def build():
                subprocess.run([sys.executable, str(root / 'tools/build_web.py')], check=True, capture_output=True)
            build()
            first = (root / 'dist/demo/index.html').read_text()
            self.assertIn('app.js?v=', first)
            self.assertIn('style.css?v=', first)
            self.assertIn('https://example.com', first)
            self.assertEqual((root / 'preview/index.html').read_text(), original)
            (root / 'preview/app.js').write_text('let v=2;')
            build()
            self.assertNotEqual((root / 'dist/demo/index.html').read_text(), first)
