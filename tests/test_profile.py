import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("apply_profile", Path(__file__).resolve().parents[1] / "tools/apply_profile.py")
profile = importlib.util.module_from_spec(spec)
spec.loader.exec_module(profile)

class ProfileTests(unittest.TestCase):
    def setUp(self):
        self.valid = dict(schemaVersion=1, name="Milo", theme="moss", eyes="round", focusMinutes=25)

    def test_header(self):
        self.assertIn('#define PB_PET_NAME "Milo"', profile.header(self.valid))
        self.assertIn('#define PB_FOCUS_MINUTES 25', profile.header(self.valid))

    def test_valid_combinations(self):
        for theme in profile.THEMES:
            for eyes in profile.EYES:
                self.assertEqual(profile.validate({**self.valid, "theme": theme, "eyes": eyes})["eyes"], eyes)

    def test_unsafe_names(self):
        for name in ['"\n#error bad', "", "a" * 17, " Milo", "Milo ", "<script>", "🐶"]:
            with self.subTest(name=name), self.assertRaises(ValueError):
                profile.validate({**self.valid, "name": name})

    def test_invalid_fields(self):
        for key, value in [("schemaVersion", 2), ("schemaVersion", True), ("theme", "red"), ("eyes", "unknown"), ("focusMinutes", 0), ("focusMinutes", 61), ("focusMinutes", 5.1), ("focusMinutes", True)]:
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                profile.validate({**self.valid, key: value})
        with self.assertRaises(ValueError):
            profile.validate({**self.valid, "command": "anything"})

if __name__ == "__main__":
    unittest.main()
