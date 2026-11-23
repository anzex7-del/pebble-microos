# Pebble project handoff

## Goal and target
Build a cute, responsive pocket pet for **Waveshare T5-E1-Touch-AMOLED-1.75**: Tuya T5-E1 ARM, 466×466 CO5300 AMOLED, CST92xx touch. This is **not an ESP32 board**.

Repository: https://github.com/anzex7-del/pebble-microos
Pet Studio: https://anzex7-del.github.io/pebble-microos/
Simulator: https://anzex7-del.github.io/pebble-microos/demo/

## Current delivery
- Version: **0.3.0-preview.1**, firmware release **preview-build-7**.
- Download: https://github.com/anzex7-del/pebble-microos/releases/tag/preview-build-7
- Successful exact-target build: Actions run **45163884307**, source **76be85712617120092197178c123e541d6579271**.
- Flash file: **pebble_microos_QIO_1.0.0.bin**. The SDK filename suffix is not the app version.
- The owner saw display glitches on taps and during animation after installing the previous preview. Build 7 aligns cropped redraws to the CO5300’s 2×2 pixel grid. A host check covered every in-bounds start/end pair on both axes. Physical confirmation is pending.

Implemented: retained LVGL controls; background flash/network worker; cropped display updates; animated bunny; Wi-Fi scan/join/optional remember/forget; three-minute BLE Studio link for design, pats and time; internet/computer/manual clock with UTC+4 default.

Clock loses time on cold boot until resynced. Hardware RTC, deep sleep, battery readings, sound and OTA remain pending. BLE is a settings bridge, not audio; it has no authenticated ownership during the enabled window.

## Where to work
- `firmware/src/pebble_ui.c`: native UI and pet animation.
- `pebble_services.c`: Wi-Fi, BLE, clock and asynchronous storage.
- `pebble_link.c`: validated protocol, NTP parsing and clock formatting.
- `web/`: Studio and Web Bluetooth; `preview/`: separate browser simulator.
- `docs/CONNECTIVITY.md`: setup and protocol; `docs/HARDWARE_AND_BUILD.md`: build/flashing.
- `tools/stage_tuya.py`: two guarded SDK display patches. Preserve them.
- TuyaOpen pinned at `cc6b7cdc35f0168186859c02b4979e9580b3a760`.

## Next step
On build 7, repeat the taps and animations that glitched and check whether corruption returns. Wi-Fi join/time sync and BLE design/time/pat, reconnect and expiry still need device feedback. Do not describe an untested feature as hardware-validated.

Studio desktop/mobile layouts and reactions were checked. After the Pages asset fingerprinting change (`c2aa823`), a live Edge check of `studio.js?v=5e8693f66d74` exported 1080×1350 PNGs for Moss/Happy and Lilac/Loved. Both showed the bunny’s inner ear and cream belly, with the selected body color and reaction-specific cheeks. Core sanitizer tests, eight Python tests and mocked browser/BLE tests passed in their respective local/CI checks.

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
