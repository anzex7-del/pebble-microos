# Development roadmap

## Preview 0.2

- [x] Pet, focus timer, three games, stopwatch, breathing and dice source
- [x] Pet Studio: name, color, eye shape, focus duration, profile import/export, portrait export
- [x] Validated profile-to-firmware defaults
- [x] Automated core, profile and preview logic tests
- [x] GitHub Actions T5 cross-build and artifact packaging workflow
- [x] GitHub Pages deployment workflow
- [x] Full T5 build confirmed green ([run 45082490199](https://github.com/anzex7-del/pebble-microos/actions/runs/45082490199))
- [x] Owner confirmed baseline boot/touch; lag reported (23 November 2026)

## Preview 0.3

- [x] Retained controls and background storage/network worker
- [x] Guarded cropped-QSPI display fix and lower input polling interval
- [x] Rounded animated pet artwork in native UI, Studio and simulator
- [x] Wi-Fi scan/join/remember/forget implementation
- [x] BLE profile/time/pat bridge with acknowledgments and a timed access window
- [x] Internet/companion/manual clock with UTC offset
- [x] Native parser/time tests and mocked GATT success/failure tests
- [ ] Measure touch latency and confirm partial-flush correctness on hardware
- [ ] Validate Wi-Fi/BLE coexistence and all clock paths on hardware

## Hardware beta

- [ ] Accurate on-device battery and charging display
- [ ] Display sleep, physical wake and safe shutdown save
- [ ] RTC-backed timer recovery and wake alarm
- [ ] Speaker tone and mute settings
- [ ] Motion expressions with a shared-I2C-safe IMU task
- [x] BLE profile synchronization implemented with device acknowledgment; radio acceptance pending
- [ ] Native on-screen rename keyboard, two-player mode and coin flip (BLE rename exists)
- [ ] 24-hour soak and repeated power-loss tests

## Release candidate

- [ ] Stable recovery procedure and signed update design
- [ ] Measure battery life, touch accuracy and thermal behavior
- [ ] Small adult tester cohort and day-seven retention feedback
- [ ] Final brand selection, notices and distribution decisions

Use issues for individual tasks, branches for changes, and pull requests for review. Keep the board configuration explicit. Do not treat the ESP32 and T5 boards as interchangeable.
