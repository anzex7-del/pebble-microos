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

## Not yet validated

- Actual browser layout, font fallback, real keyboard focus, touch/pointer behavior and PNG download rendering. The browser tool rejected local-file URLs, and the environment did not allow a localhost server. No visual browser test is claimed.
- Full TuyaOpen application build and link. SDK APIs/configuration were inspected, but this is not a cross-compilation result.
- Device display/touch/audio/RTC/IMU/BLE, physical buttons, power, charge and battery life.
- KV initialization behavior, flash-write endurance or crash consistency on the physical board.
- Native UI and browser UI are not feature-identical; see the README matrix.
- Timer recovery on native reboot; hardware sleep and wake; background alarm sound.
- Production manufacturing, security update signing, compliance or commercial brand clearance.

## Manual preview pass

Open `Pebble_Preview.html` in Chrome, Edge, Safari or Firefox. Test at desktop and phone-width sizes. Complete tic-tac-toe against the pet and in pass-and-play mode; finish a memory garden; play a 20-second catch round. Start the 10-second timer, pause/resume and navigate away. Rename the pet with 16 characters, change color and case finish, refresh, then save a portrait and inspect the PNG. Check that keyboard Tab reaches screen controls and Escape returns home. Check the screen controls fit within the circle.

The mock harness is useful for catching logic errors, but it cannot substitute for this pass.
