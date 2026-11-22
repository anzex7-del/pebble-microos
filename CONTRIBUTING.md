# Working on Pebble

Source, bugs, design proposals and releases live in this repository. Use a feature branch and a pull request for each change. Include reproduction steps for bug fixes and explain what the person using the device will notice.

## Folders

- `firmware/`: C engine, LVGL UI, T5 application/configuration.
- `web/`: Pet Studio, deployed to GitHub Pages.
- `preview/`: full browser microOS demo, deployed under `/demo/`.
- `profiles/`: build-time customization profiles.
- `tests/`: logic and profile validation.
- `docs/`: hardware setup, acceptance criteria and roadmap.

## Checks

```sh
sh tools/test.sh
python3 -m unittest discover -s tests -p 'test_*.py'
node tests/run-browser-tests.cjs
node tests/profile-web-test.cjs
python3 tools/build_web.py
```

On macOS without Node, `sh tools/test-preview-macos.sh` uses JavaScriptCore for the original preview logic. Full website/browser QA remains separate from mock tests.

## Firmware profiles

```sh
python3 tools/apply_profile.py profiles/default.json
python3 tools/stage_tuya.py /absolute/path/to/TuyaOpen
```

For a personalized build on GitHub, run **Build T5 preview firmware** with the exported JSON in `profile_json`. This is maintainer/fork-owner functionality; the public website never asks visitors for GitHub tokens. A successful build uploads a firmware artifact with a manifest and checksums. Build success is distinct from physical-device validation.

Names are restricted to the ASCII glyphs in the bundled native font. Profile values are validated before generating a C header; no profile field is evaluated as shell code. Name, eye style and default focus duration are compile-time values in this preview. Color is the initial default; a previously saved on-device theme takes precedence.

## Releases

Tag tested source revisions and publish release notes with hardware verification status, known limitations, recovery instructions and artifact checksums. Do not upload calibration data, signing keys, tokens, personal logs, SDK build folders or private profiles. Third-party licensing decisions are described in `THIRD_PARTY_NOTICES.md`; no broader license grant is implied by a public repository.
