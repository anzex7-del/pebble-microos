# Hardware identification and build path

## The board in your photo

| Item | Identification / implication |
|---|---|
| Family | Waveshare T5-E1-Touch-AMOLED-1.75 |
| MCU | Tuya T5-E1; Armv8-M STAR-MC1, up to 480 MHz |
| Display | Round 1.75-inch 466 × 466 AMOLED; CO5300 controller; QSPI |
| Touch | CST9217; I2C |
| Memory | 8 MB flash, 16 MB PSRAM; manufacturer lists 640 KB SRAM and 64 KB ROM |
| Motion | QMI8658 accelerometer/gyroscope |
| RTC | PCF85063 |
| Other board facilities | USB-C, audio connections, microSD, PWR/KEY, battery connector; 2.4 GHz Wi-Fi and BLE |
| GPS | Optional -G variant; cannot confirm fitted hardware from this photograph |
| Battery capacity | Not shown; do not assume capacity, connector polarity, or runtime |

The board label and Waveshare specification agree. This is not the ESP32-S3-Touch-AMOLED-1.75C used by Pip-KeyPet. Do not use Pip's ESP flashing image or esptool instructions on this Tuya board.

## Software baseline

- Native C application on TuyaOpen and its existing T5 board package.
- LVGL 9 selected explicitly. Pip's LVGL 9.5 code is not imported.
- Reviewed TuyaOpen snapshot: `cc6b7cdc35f0168186859c02b4979e9580b3a760`.
- Board configuration: `WAVESHARE_T5AI_TOUCH_AMOLED_1_75`.
- The source uses vendor board registration and display initialization. It does not invent pin assignments or replace panel initialization sequences.

## Download a compiled preview

The full T5 application compiled and linked successfully in [GitHub Actions build 2](https://github.com/anzex7-del/pebble-microos/actions/runs/45082490199), source commit `8161f34b6f039f1e856c80cfb8431d101a8daec4`. New default builds are published as [numbered prereleases](https://github.com/anzex7-del/pebble-microos/releases).

Download `pebble-t5-preview.zip` from a successful prerelease and extract it. Read `manifest.json` for the exact board, source commit and SHA-256 checksums. The verified SDK output names its serial-flashing image **`pebble_microos_QIO_1.0.0.bin`**. The `1.0.0` filename suffix is the SDK packaging default; this application's preview version is in the manifest. Do not substitute an OTA image for the QIO serial image.

Build 2 is also available as the `pebble-t5-preview-2` Actions artifact (GitHub sign-in required). Its bundled instructions predate the successful build; use this current guide. The owner flashed the 0.2 baseline and confirmed boot and touch, with lag. The new 0.3 firmware requires its own hardware test.

For a custom pet, follow the Studio workflow in [README.md](../README.md). Custom builds are downloaded from the completed workflow's artifacts; they are not automatically published as public releases.

## Build on your Mac

1. Follow the official [macOS environment setup](https://tuyaopen.ai/docs/quick-start/enviroment-setup). Use its macOS tab for required packages. Keep SDK/application paths free of spaces.
2. Clone the SDK in a workspace you choose and pin the reviewed revision:

```sh
git clone https://github.com/tuya/TuyaOpen.git
cd TuyaOpen
git checkout cc6b7cdc35f0168186859c02b4979e9580b3a760
git submodule update --init --recursive
. ./export.sh
tos.py check
```

3. Clone this repository. Optionally run `python3 tools/apply_profile.py /absolute/path/to/pebble-pet-profile.json` (see `--help` for arguments), then copy the native application into the SDK using the included staging script. Replace the example path with your actual SDK path:

```sh
python3 tools/stage_tuya.py /absolute/path/to/TuyaOpen
```

The script creates `examples/pebble_microos` and refuses to overwrite an existing directory. It does not build, erase, or flash anything. It checks that the exact board package exists.

4. In a terminal with the SDK environment activated:

```sh
cd /absolute/path/to/TuyaOpen/examples/pebble_microos
tos.py config choice
# Choose WAVESHARE_T5AI_TOUCH_AMOLED_1_75.config
tos.py build
```

This follows the official [build workflow](https://tuyaopen.ai/docs/quick-start/project-compilation). Treat compiler output as authoritative. Check that LVGL 9, partial display flush and Montserrat 16/20/40 are enabled. The full cross-toolchain and linkage passed in Linux CI. Runtime memory behavior and physical board revisions still require device validation. Keep the full build log and resulting memory report.

## Physical-device installation — only after a successful build

1. Check the exact printed model and revision. Keep the Waveshare factory image and recovery instructions available. Preserve device-specific calibration/authorization data; do not run a blanket full-chip erase. If the official tool supports a full readback for your board, preserve it before writing.
2. Connect through a USB data cable. Close serial-monitor applications that hold the ports. Confirm that your Mac sees the board; use Waveshare's recommended driver only if required for this board revision.
3. For a downloaded QIO binary, use the official T5 flashing tool described in the vendor [flashing and logging guide](https://tuyaopen.ai/docs/quick-start/firmware-burning). Select the extracted QIO binary for this board and follow the tool’s reset prompts. Alternatively, after building the SDK locally, run `tos.py flash` from the application directory; the SDK selects its build output. Use `tos.py flash --help` for available options rather than guessing file arguments. Use the image produced for this exact T5 configuration. Do not manually choose offsets from ESP32 guides.
4. Use `tos.py monitor` and confirm the Pebble startup log, display, touch and save status. Record the SDK revision and board revision with the result.

The supplied 0.3 binary must pass its full build before installation. Earlier 0.2 boot/touch feedback does not validate the new radio, display or clock changes.

## Preview 0.3 changes

[Connectivity and clock guide](CONNECTIVITY.md) covers on-device Wi-Fi, the three-minute BLE settings window and time setup. The display staging script applies two guarded patches to the pinned SDK: correct the inclusive end coordinates for cropped QSPI rectangles, and avoid selecting full dual framebuffers when partial flush is enabled. It refuses unknown source shapes rather than silently patching another SDK revision. Pin mappings and CO5300 initialization remain vendor-provided.

Touch polling is 16 ms and the display refresh target is 33 ms; these are configured intervals, not measured physical latency or guaranteed frame rates. Controls persist during interactions. Flash/network work is moved out of the LVGL thread, pet motion is small, and unchanged labels/styles are not redrawn unnecessarily. Verify touch lag and screen corruption on hardware before wider use.

## Device acceptance checklist

| Test | Pass condition |
|---|---|
| Boot and recovery | Cold boot succeeds repeatedly; factory recovery demonstrated before a pilot |
| Display | Correct orientation/color; all 466 × 466 UI elements within the circle; no clipping or tearing |
| Touch | Every button works with a finger, including near edges; no duplicate moves |
| Wi-Fi | Scan, join, wrong-password feedback, remember/reboot, forget and offline time fallback |
| Bluetooth | Browser discovery, confirmed design/time/pat writes, disconnect, expiry, reconnect, malformed payload rejection |
| Clock | Tbilisi offset, manual adjustment, Wi-Fi/companion sync, midnight rollover, honest unset state after cold boot |
| Core apps | Complete each game; navigate away/back while timer and stopwatch run |
| Save | Care/theme/progress survive 20 controlled power cycles; corrupt save produces defaults, not crash |
| Power loss | Known save-loss window documented; timer cancellation on reboot clearly understood |
| Runtime | 24-hour mixed-use soak without crash, leak or runaway allocation |
| Power management | Measure active/rest/sleep current and wake paths; black pixels alone do not establish low power |
| Battery | Confirm approved chemistry, capacity and polarity; measure charge behavior and enclosure temperature |
| RTC/timer | After implementation, correct elapsed time across sleep, reboot and time changes |
| Inputs | Physical back/wake/long-press escape mapped and tested before carry use |
| Audio | After implementation, confirm volume/mute and completion tone without clipping |
| Release | Reproducible build, version label, checksums, factory recovery and support instructions |

Motion, RTC, audio, battery measurement and hardware sleep are board capabilities, but are not implemented application features in this alpha. Core functions are useful now; these integrations determine whether it is ready to carry or sell.
