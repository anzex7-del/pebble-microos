from pathlib import Path
import importlib.util
import tempfile
import unittest
spec = importlib.util.spec_from_file_location('stage', Path(__file__).resolve().parents[1] / 'tools/stage_tuya.py')
stage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(stage)


class StageTests(unittest.TestCase):
    def test_pinned_patches_and_idempotency(self):
        with tempfile.TemporaryDirectory() as temp:
            sdk = Path(temp)
            for name, before, after in stage.PATCHES:
                path = sdk / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('prefix\n' + before + '\nsuffix\n')
            stage.patch_display(sdk)
            stage.patch_display(sdk)
            for name, before, after in stage.PATCHES:
                self.assertIn(after, (sdk / name).read_text())
                self.assertNotIn(before, (sdk / name).read_text())

    def test_unknown_revision_does_not_partially_patch(self):
        with tempfile.TemporaryDirectory() as temp:
            sdk = Path(temp)
            for i, (name, before, after) in enumerate(stage.PATCHES):
                path = sdk / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(before if i == 0 else 'upstream changed')
            with self.assertRaises(RuntimeError):
                stage.patch_display(sdk)
            self.assertEqual((sdk / stage.PATCHES[0][0]).read_text(), stage.PATCHES[0][1])
