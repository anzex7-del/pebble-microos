# Pebble microOS · engineering preview 0.2.0-preview.1

A little companion for a round touchscreen: care for a pet, focus together, and play short games. Built for Anz, 23 November 2026. **Pebble is a working title, not a cleared commercial brand.**

## Start here

- [Open Pet Studio](https://anzex7-del.github.io/pebble-microos/) to choose a name, color, eye style and focus duration, then import/export a profile or save a portrait.
- [Try the interactive microOS preview](https://anzex7-del.github.io/pebble-microos/demo/) to explore pet care, focus sessions, games and tools.
- [Download compiled T5 preview firmware](https://github.com/anzex7-del/pebble-microos/actions/runs/45082490199): open the successful run and download **pebble-t5-preview-2** from Artifacts (GitHub sign-in required). Artifacts expire; use the [build workflow](https://github.com/anzex7-del/pebble-microos/actions/workflows/firmware.yml) for a fresh or personalized build.

For an offline browser demo, clone or download this repository and run `python3 tools/package.py`. Open the generated **Pebble_Preview.html** in a modern browser. No installation, account, API key or internet connection is required for that demo. Click the device screen or use the menu beside it. Press Escape to return home. Use the 10-second timer demo to see completion immediately; it deliberately gives no focus reward.

The preview is a browser application. It cannot flash or connect to your physical device. The adjacent `firmware/` folder contains a separate native C/LVGL application targeting TuyaOpen. **The T5 firmware successfully cross-compiled in GitHub Actions on 23 November 2026 (Asia/Tbilisi). Physical-device testing is still pending.** Pet Studio exports build-time defaults; it does not synchronize with the simulator or your device.

## Your hardware

The label in your photo identifies **Waveshare T5-E1-Touch-AMOLED-1.75**, using a Tuya T5-E1 processor. Pip-KeyPet targets **Waveshare ESP32-S3-Touch-AMOLED-1.75C**. Their firmware and flashing tools are not interchangeable. The photo does not establish whether an optional GPS module or battery is fitted.

Read [hardware and build instructions](docs/HARDWARE_AND_BUILD.md) before any physical-device work.

## What is included

| Feature | Browser preview | Native C/LVGL source |
|---|---|---|
| Pet care, food/joy/energy, sleep/wake | Implemented | Implemented; board validation pending |
| Personality colors | Four | Four |
| Visible growth stages | Sprout, buddy, bloom | Level counter; growth artwork pending |
| Focus and countdown | 5/25/50 minutes; 10-second demo | Profile default (5–60 minutes), presets and 1-minute timer |
| Timer pause, resume, completion | Implemented | Implemented; awake uptime only |
| Tic-tac-toe vs unbeatable pet | Implemented | Implemented |
| Local two-player tic-tac-toe | Implemented | Pending |
| Memory game | Four pairs; completion reward | Four pairs; completion reward |
| Catch a spark | 20-second touchscreen game | 20-second touchscreen game |
| Stopwatch | Start, pause, reset, last lap | Start, pause, reset |
| Clock | Uses computer clock | RTC integration pending |
| Breathing guide | Animated 4/4/6 rhythm | Text 4/4/6 rhythm |
| Dice / coin | Both | Dice only |
| Pet rename / eye style | Rename in simulator; three eye styles in Studio | Build-time profile; runtime rename pending |
| Portrait PNG export | Implemented | Phone/USB companion bridge pending |
| Persistence | Browser local storage | Versioned CRC save + Tuya KV adapter, untested on flash |
| Screen rest | Dark preview | Black screen only; not deep sleep |
| Sound, motion, battery percentage, BLE, OTA | Not simulated as working | Not implemented |

Browser and native UI are separate implementations. Matching core behaviors have tests in both; this is not a pixel-identical emulator of the firmware. Progress is not synchronized between the browser and hardware.

## Project layout

- `Pebble_Preview.html`: portable, self-contained browser demo.
- `preview/`: editable HTML, CSS and JavaScript sources.
- `web/`: Pet Studio; `tools/build_web.py` assembles the GitHub Pages site.
- `profiles/default.json`: default build-time pet profile.
- `.github/workflows/`: automatic checks, Pages deployment and T5 firmware builds.
- `firmware/`: native application, TuyaOpen configuration, reusable C logic.
- `tests/`: core tests and browser logic tests.
- `docs/PRODUCT_BLUEPRINT.md`: product, scope, launch experiments, economics and roadmap.
- `docs/HARDWARE_AND_BUILD.md`: identification, SDK setup and hardware acceptance checklist.
- `docs/VALIDATION.md`: checks performed, limits and release gates.
- `tools/`: packaging, tests and safe SDK staging.

## Test locally

On macOS with Apple Command Line Tools installed:

```sh
sh tools/test.sh
sh tools/test-preview-macos.sh
```

The core tests use AddressSanitizer and UndefinedBehaviorSanitizer. The preview tests use the system JavaScriptCore engine and a mock DOM; they do **not** prove visual layout, browser download behavior, or hardware operation. The hosted site also has a browser smoke test recorded in [VALIDATION.md](docs/VALIDATION.md). Physical-device acceptance is still required.

## Design principles

No pet death, paid resurrection, compulsory daily streaks, account signup, recording or cloud dependency. A completed focus session of at least five minutes awards 25 sparks. Games award smaller amounts. Each 100 sparks advances a level. Care keeps the pet comfortable without awarding farmable focus credit.

The native timer is intentionally session-only until RTC restore and wake behavior are validated. Unexpected power loss can lose up to 30 seconds of pet changes; Settings → Save pet now requests an immediate save. A timer cannot currently wake the board or sound an alarm. Do not use this prototype as a critical alarm.

## Sources and reuse

Pip-KeyPet was reviewed for product and architecture inspiration, not copied into this source. New pet artwork is procedural. Upstream firmware binaries, fonts and sprites are not redistributed. TuyaOpen and LVGL are external dependencies with their own licenses. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
