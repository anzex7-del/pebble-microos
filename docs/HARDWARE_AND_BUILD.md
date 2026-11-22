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

## What was not possible here

The local environment has a C compiler but no configured TuyaOpen cross-toolchain. Network access from the build shell is restricted. Native portable logic was compiled and tested; the full T5 SDK application was not compiled or flashed. A browser policy also prevents local-file visual inspection. Those are open validation steps, not successes.

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

3. From the extracted Pebble package, copy the native application into the SDK using the included staging script. Replace the example path with your actual SDK path:

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

This follows the official [build workflow](https://tuyaopen.ai/docs/quick-start/project-compilation). Treat compiler output as authoritative. Check that LVGL 9 and Montserrat 20/40 are enabled. The package has not yet verified the full toolchain, linkage, runtime memory allocation or board revisions. Keep the full build log and resulting memory report.

## Physical-device installation — only after a successful build

1. Check the exact printed model and revision. Keep the Waveshare factory image and recovery instructions available. Preserve device-specific calibration/authorization data; do not run a blanket full-chip erase. If the official tool supports a full readback for your board, preserve it before writing.
2. Connect through a USB data cable. Close serial-monitor applications that hold the ports. Confirm that your Mac sees the board; use Waveshare's recommended driver only if required for this board revision.
3. Follow the vendor [flashing and logging guide](https://tuyaopen.ai/docs/quick-start/firmware-burning). From the successfully built application directory, use `tos.py flash`, select the proper port, and follow its reset prompts. Use the image produced for this exact T5 configuration. Do not manually choose offsets from ESP32 guides.
4. Use `tos.py monitor` and confirm the Pebble startup log, display, touch and save status. Record the SDK revision and board revision with the result.

No flash command has been run in this session. No binary is provided because the target build has not passed.

## Device acceptance checklist

| Test | Pass condition |
|---|---|
| Boot and recovery | Cold boot succeeds repeatedly; factory recovery demonstrated before a pilot |
| Display | Correct orientation/color; all 466 × 466 UI elements within the circle; no clipping or tearing |
| Touch | Every button works with a finger, including near edges; no duplicate moves |
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
