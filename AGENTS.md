# Pebble project handoff

## Goal and target
Build a cute, responsive pocket pet for **Waveshare T5-E1-Touch-AMOLED-1.75**: Tuya T5-E1 ARM, 466×466 CO5300 AMOLED, CST92xx touch. This is **not an ESP32 board**.

Repository: https://github.com/anzex7-del/pebble-microos
Pet Studio: https://anzex7-del.github.io/pebble-microos/
Simulator: https://anzex7-del.github.io/pebble-microos/demo/

## Delivery and device feedback
- Preview version **0.3.0-preview.1**. The latest numbered binary is on [GitHub Releases](https://github.com/anzex7-del/pebble-microos/releases). The owner tested build 8 and reported small keys.
- Build 8 baseline: https://github.com/anzex7-del/pebble-microos/releases/tag/preview-build-8
- Build 8 exact-target run: Actions run **45166609546**, source **6391194d0db93328e396ee219d74a3310ab9491a**.
- Flash file: **pebble_microos_QIO_1.0.0.bin**. The SDK filename suffix is not the app version.
- The owner saw display glitches on taps and during animation before build 7. Build 7 aligns cropped redraws to the CO5300’s 2×2 pixel grid. A host check covered every in-bounds start/end pair on both axes. Physical confirmation is pending.

Build 8 adds QWERTY entry for Wi-Fi and pet names, saved AMOLED brightness and independent PWR/KEY short-press shortcuts in **Apps → Style → Controls**, and a bouncing die with pips in **Apps → Tools → Dice**. Holding PWR still uses the board driver’s power-off behavior. A headless LVGL check exercised touch typing, both shortcuts, brightness limits and the six die faces with simulated hardware inputs and display output.

Implemented: retained LVGL controls; background flash/network worker; cropped display updates; animated bunny; Wi-Fi scan/join/optional remember/forget; three-minute BLE Studio link for design, pats and time; internet/computer/manual clock with UTC+4 default.

Clock loses time on cold boot until resynced. Hardware RTC, deep sleep, battery readings and OTA remain pending. Pet sounds use the speaker socket. BLE is a settings bridge, not audio; it has no authenticated ownership during the enabled window.

## Where to work
- `firmware/src/pebble_ui.c`: native UI and pet animation.
- `pebble_services.c`: Wi-Fi, BLE, clock and asynchronous storage.
- `pebble_link.c`: validated protocol, NTP parsing and clock formatting.
- `web/`: Studio and Web Bluetooth; `preview/`: separate browser simulator.
- `docs/CONNECTIVITY.md`: setup and protocol; `docs/HARDWARE_AND_BUILD.md`: build/flashing.
- `tools/stage_tuya.py`: two guarded SDK display patches. Preserve them.
- TuyaOpen pinned at `cc6b7cdc35f0168186859c02b4979e9580b3a760`.

## Next step
For the keyboard, sound and USB update, verify the larger keys on the round display, each care chirp and mute across reboot, then apply a design through USB channel A. Roll the die and repeat the taps and animations that previously glitched. Wi-Fi join/time sync and BLE design/time/pat, reconnect and expiry still need device feedback. Do not describe an untested feature as hardware-validated.

Studio desktop/mobile layouts and reactions were checked. After the Pages asset fingerprinting change (`c2aa823`), a live Edge check of `studio.js?v=5e8693f66d74` exported 1080×1350 PNGs for Moss/Happy and Lilac/Loved. Both showed the bunny’s inner ear and cream belly, with the selected body color and reaction-specific cheeks. Core sanitizer tests, eight Python tests and mocked browser/BLE tests passed in their respective local/CI checks.

## Agent workflow

The coordinator uses the project roles in `.codex/agents/` for Pebble development. The owner reported that the build 8 keyboard was too small and requested larger keys, speaker reactions and USB personalization. GPS/Find My work is deferred at the owner's request.

1. For UI work, ask `ui_designer` for a proposal scoped to the affected UI files, with observable acceptance criteria. An explicit owner request approves that improvement. Bring new proposals to the owner before implementation.
2. Give `feature_developer` one approved improvement at a time. Prepare the change away from main so the full diff is available before a merge or release.
3. Give `security_stability_checker` the diff and acceptance criteria. Send findings to the developer for fixes and repeat affected checks.
4. Give a fresh `independent_code_reviewer` the complete final diff against its base and the check results. Obtain its report before merging, updating main or releasing. If the diff changes, have the checker verify the affected paths and the reviewer inspect the revised final diff.
5. The coordinator resolves blocking findings before integration and reports what changed, checks run and remaining device validation.

## Efficient workflow
Keep tasks focused; inspect relevant files rather than rereading the repository. Run checks appropriate to the change:
```sh
sh tools/test.sh
sh tools/test-preview-macos.sh
python3 -m unittest discover -s tests -p 'test_*.py'
# With Node installed:
node tests/profile-web-test.cjs
node tests/companion-test.cjs
```
GitHub Actions performs the Linux T5 cross-build. Firmware-path changes on main publish numbered prereleases; web changes deploy Pages. Do not repeatedly rebuild unchanged firmware. Never flash hardware or handle credentials without the owner's instruction. Preserve existing work and use GitHub for source/history.
