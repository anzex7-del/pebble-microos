# Development roadmap

## Preview 0.2

- [x] Pet, focus timer, three games, stopwatch, breathing and dice source
- [x] Pet Studio: name, color, eye shape, focus duration, profile import/export, portrait export
- [x] Validated profile-to-firmware defaults
- [x] Automated core, profile and preview logic tests
- [x] GitHub Actions T5 cross-build and artifact packaging workflow
- [x] GitHub Pages deployment workflow
- [x] Full T5 build confirmed green ([run 45082490199](https://github.com/anzex7-del/pebble-microos/actions/runs/45082490199))
- [ ] First physical-device smoke test

## Hardware beta

- [ ] Accurate on-device battery and charging display
- [ ] Display sleep, physical wake and safe shutdown save
- [ ] RTC-backed timer recovery and wake alarm
- [ ] Speaker tone and mute settings
- [ ] Motion expressions with a shared-I2C-safe IMU task
- [ ] USB or BLE profile synchronization with device acknowledgement
- [ ] Native rename, two-player mode and coin flip
- [ ] 24-hour soak and repeated power-loss tests

## Release candidate

- [ ] Stable recovery procedure and signed update design
- [ ] Measure battery life, touch accuracy and thermal behavior
- [ ] Small adult tester cohort and day-seven retention feedback
- [ ] Final brand selection, notices and distribution decisions

Use issues for individual tasks, branches for changes, and pull requests for review. Keep the board configuration explicit. Do not treat the ESP32 and T5 boards as interchangeable.
