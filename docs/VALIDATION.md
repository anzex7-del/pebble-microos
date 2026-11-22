# Validation record — 23 November 2026

## Passed

- Portable C engine compiled with Apple Clang, C11, `-Wall -Wextra -Werror -pedantic`.
- Core test executable passed with AddressSanitizer and UndefinedBehaviorSanitizer.
- Pet stat bounds, sleep recovery, timer elapsed time, pause/resume, completion and single reward checked.
- All 635 completed human-play paths against the deterministic tic-tac-toe opponent explored; opponent never loses when moving second.
- Memory pairs, duplicate taps, completion and reward behavior checked.
- Catch-game duplicate-target prevention and time limit checked.
- Save round trip, truncated payload rejection and corrupted payload rejection checked.
- Browser JavaScript evaluated in macOS JavaScriptCore with a mock DOM and canvas interface.
- All 15 preview screens constructed without JavaScript exceptions in the mock harness.
- Browser focus pause/resume, demo reward exclusion, the same 635 tic-tac-toe paths, memory mismatch delay, stopwatch pause, catch completion and persisted counters checked.

## Hosted preview and CI verification

- Full T5 application compiled and linked with the pinned TuyaOpen SDK; [firmware run 45082490199](https://github.com/anzex7-del/pebble-microos/actions/runs/45082490199) succeeded and uploaded `pebble-t5-preview-2` plus its build log.
- [GitHub Pages deployment 45082490207](https://github.com/anzex7-del/pebble-microos/actions/runs/45082490207) and [automated checks 45082490242](https://github.com/anzex7-del/pebble-microos/actions/runs/45082490242) passed for source commit `8161f34b6f039f1e856c80cfb8431d101a8daec4`.
- Opened the live Pet Studio and simulator in the in-app browser. Visually checked desktop and 390 × 844 phone layouts, including customization controls and simulator display.
- Changed Studio color and confirmed the face, accessibility description and profile JSON updated. Reload preserved the selected color; restored the original design after checking.
- Downloaded a profile JSON and a portrait PNG from the live site. Inspected the JSON fields and the 1080 × 1350 portrait.
- Started the simulator’s 10-second demo, paused, resumed and observed completion with zero focus sessions awarded.
- No captured browser console errors during this Studio smoke test.
- Re-ran local C sanitizer tests, JavaScriptCore preview tests and five Python tests, including source export exclusions. Rebuilt the static site.

## Not yet validated

- Full cross-browser and keyboard accessibility coverage, physical touch behavior, all game flows in a real browser, profile import through a real file picker and storage-denied browser behavior. The hosted smoke test is narrower than a complete acceptance pass.
- Device display/touch/audio/RTC/IMU/BLE, physical buttons, power, charge and battery life.
- KV initialization behavior, flash-write endurance or crash consistency on the physical board.
- Native UI and browser UI are not feature-identical; see the README matrix.
- Timer recovery on native reboot; hardware sleep and wake; background alarm sound.
- Production manufacturing, security update signing, compliance or commercial brand clearance.

## Manual preview pass

Open `Pebble_Preview.html` in Chrome, Edge, Safari or Firefox. Test at desktop and phone-width sizes. Complete tic-tac-toe against the pet and in pass-and-play mode; finish a memory garden; play a 20-second catch round. Start the 10-second timer, pause/resume and navigate away. Rename the pet with 16 characters, change color and case finish, refresh, then save a portrait and inspect the PNG. Check that keyboard Tab reaches screen controls and Escape returns home. Check the screen controls fit within the circle.

The mock harness is useful for catching logic errors, but it cannot substitute for this pass.
