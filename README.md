# Pebble microOS · engineering preview 0.2.0-preview.1

A little companion for a round touchscreen: care for a pet, focus together, and play short games. Built for Anz, 23 November 2026. **Pebble is a working title, not a cleared commercial brand.**

## Start here

- **[Customize your pet](https://anzex7-del.github.io/pebble-microos/)** — name, color, eyes, focus duration, profile JSON and portrait export.
- **[Try microOS in your browser](https://anzex7-del.github.io/pebble-microos/demo/)** — click the round screen or the menu beside it. Escape returns home.
- **[Download T5 preview builds](https://github.com/anzex7-del/pebble-microos/releases)** — compiled firmware with checksums and installation instructions.
- **[Build verification](https://github.com/anzex7-del/pebble-microos/actions/runs/45082490199)** — the first successful full T5 ARM build. Its `pebble-t5-preview-2` artifact is also downloadable while retained by GitHub; artifact downloads require sign-in.

**Status:** full target compilation and automated engine/preview/profile tests pass. Pet Studio is deployed and its editing controls were checked in a real browser. The firmware has **not been flashed or tested on a physical device**. This is an engineering preview, not a finished consumer release.

The browser demo and native C/LVGL firmware are separate implementations. The website cannot currently connect to or flash your device. The 10-second browser timer demo intentionally gives no focus reward.

## Customize and install

1. Open Pet Studio, choose a name (up to 16 English letters/numbers with internal spaces, underscores or hyphens), one of four colors, an eye style, and a 5–60 minute focus preset.
2. Download the profile, or use **Your device → Copy profile JSON**. **Show profile JSON** is available if clipboard access is unavailable.
3. Repository owners can open **Actions → Build T5 preview firmware → Run workflow**. Other users first fork the repository and enable Actions. Paste the JSON in `profile_json`; leaving it blank builds Milo.
4. Wait for a successful build and download its firmware artifact. Follow [the installation guide](docs/HARDWARE_AND_BUILD.md).

Customization is applied when firmware is compiled. It is not live USB/BLE synchronization. A previously saved on-device color takes precedence over the compiled default; select the new color in device Settings after updating. Each site's demo and studio keep their own browser-local state; editing one does not synchronize the other.

For an offline demo, run `python3 tools/package.py` and open the generated `Pebble_Preview.html`. It works without an account or network connection.

## Your hardware

The label in your photo identifies **Waveshare T5-E1-Touch-AMOLED-1.75**, using a Tuya T5-E1 processor. Pip-KeyPet targets **Waveshare ESP32-S3-Touch-AMOLED-1.75C**. Their firmware and flashing tools are not interchangeable. The photo does not establish whether an optional GPS module or battery is fitted.

Read [hardware and build instructions](docs/HARDWARE_AND_BUILD.md) before any physical-device work.

## What is included

| Feature | Browser preview | Native C/LVGL source |
|---|---|---|
| Pet care, food/joy/energy, sleep/wake | Implemented | Implemented; board validation pending |
| Personality colors | Four | Four |
| Visible growth stages | Sprout, buddy, bloom | Level counter; growth artwork pending |
| Focus and countdown | 5/25/50 minutes; 10-second demo | 5/custom/50 minutes; 1-minute timer |
| Timer pause, resume, completion | Implemented | Implemented; awake uptime only |
| Tic-tac-toe vs unbeatable pet | Implemented | Implemented |
| Local two-player tic-tac-toe | Implemented | Pending |
| Memory game | Four pairs; completion reward | Four pairs; completion reward |
| Catch a spark | 20-second touchscreen game | 20-second touchscreen game |
| Stopwatch | Start, pause, reset, last lap | Start, pause, reset |
| Clock | Uses computer clock | RTC integration pending |
| Breathing guide | Animated 4/4/6 rhythm | Text 4/4/6 rhythm |
| Dice / coin | Both | Dice only |
| Pet name / eye style | Demo rename; Studio name and three eye styles | Compiled from the Studio profile |
| Portrait PNG export | Implemented | Phone/USB companion bridge pending |
| Persistence | Browser local storage | Versioned CRC save + Tuya KV adapter, untested on flash |
| Screen rest | Dark preview | Black screen only; not deep sleep |
| Sound, motion, battery percentage, BLE, OTA | Not simulated as working | Not implemented |

Browser and native UI are separate implementations. Matching core behaviors have tests in both; this is not a pixel-identical emulator of the firmware. Progress is not synchronized between the browser and hardware.

## Project layout

- `web/`: public Pet Studio and validated profile schema.
- `profiles/`: default pet design.
- `.github/`: automatic tests, firmware builds, Pages publishing and issue templates.
- `Pebble_Preview.html`: generated portable demo (`python3 tools/package.py`).
- `preview/`: editable HTML, CSS and JavaScript sources.
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
python3 -m unittest discover -s tests -p 'test_*.py'
```

The core tests use AddressSanitizer and UndefinedBehaviorSanitizer. The preview tests use the system JavaScriptCore engine and a mock DOM; they do **not** prove visual layout, browser download behavior, or hardware operation. Real-browser editing checks are recorded in [VALIDATION.md](docs/VALIDATION.md); physical-device acceptance remains open.

## Design principles

No pet death, paid resurrection, compulsory daily streaks, account signup, recording or cloud dependency. A completed focus session of at least five minutes awards 25 sparks. Games award smaller amounts. Each 100 sparks advances a level. Care keeps the pet comfortable without awarding farmable focus credit.

The native timer is intentionally session-only until RTC restore and wake behavior are validated. Unexpected power loss can lose up to 30 seconds of pet changes; Settings → Save pet now requests an immediate save. A timer cannot currently wake the board or sound an alarm. Do not use this prototype as a critical alarm.

## Sources and reuse

Pip-KeyPet was reviewed for product and architecture inspiration, not copied into this source. New pet artwork is procedural. No Pip firmware, fonts or sprites are included. Compiled T5 releases contain TuyaOpen/LVGL dependencies and include collected upstream notice files with their own licenses. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Future updates

Use [Issues](https://github.com/anzex7-del/pebble-microos/issues/new/choose) for bugs and feature ideas. Changes go through branches and pull requests with automated checks. Pushes to `main` redeploy changed web files; firmware changes build the exact T5 target and publish a numbered engineering prerelease. Custom workflow builds remain Actions artifacts. See [CONTRIBUTING.md](CONTRIBUTING.md), [the roadmap](docs/ROADMAP.md), and [the product blueprint](docs/PRODUCT_BLUEPRINT.md).
