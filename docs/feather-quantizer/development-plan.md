# Development plan

## Stage A — repository bootstrap

- fork `sekigon-gonnoc/vial-qmk`
- preserve `bmp-vial-1.0.6` as clean upstream reference
- create `feather-main`
- add project docs, AGENTS.md, guardrail CI, baseline build CI

Exit condition: project-control files are merged and upstream source is unchanged.

## Stage B — Task 1: upstream analysis

Read-only firmware analysis. Create:

`docs/feather-quantizer/research/task-001-upstream-analysis.md`

Exit condition: exact host-init -> report-parser -> QMK/Vial -> HID-output path is documented, and future Feather edit points are identified without source modifications.

## Stage C — Phase 0: hardware/HID characterization

Use Adafruit's USB host examples with the Feather.

For DEFT and later HUGE PLUS, record:

- VID/PID
- interfaces
- report IDs
- pointer X/Y reports
- buttons
- vertical wheel
- horizontal pan/tilt
- Fn/multi-function buttons

Exit condition: a reproducible HID mapping table exists for the real target device.

## Stage D — minimal Feather target

Create `keyboards/feather_quantizer/**`.

Implement only enough to build and initialize the Feather hardware correctly.

Exit condition: Feather target compiles; no upstream KQM source files changed.

## Stage E — ordinary HID passthrough

Exit condition on real hardware:

- cursor movement works
- left/right/middle work
- supported extra buttons work
- wheel/pan work
- no obvious pointer corruption under slow, fast, and diagonal movement

## Stage F — Vial / layer integration

Exit condition:

- device is configurable from Vial/Remap
- button remapping works
- MO/LT-style layer behavior works
- settings persist across power cycle

## Stage G — MX-style gestures

Exit condition:

- up/down/left/right are distinct
- cursor remains stationary during gesture recognition
- gesture fires at threshold without release
- repeated gestures work while held
- gesture completion suppresses conflicting tap action

## Stage H — scroll layer and tuning

Exit condition:

- ball-to-vertical-scroll
- ball-to-horizontal-scroll
- sensible scaling on DEFT and HUGE PLUS
- threshold/cooldown tuned by real use

## v0.1 Definition of Done

- [ ] Feather boots reliably
- [ ] KQM upstream target remains buildable
- [ ] Feather target builds in CI
- [ ] DEFT works as initial target
- [ ] HUGE PLUS validated when obtained
- [ ] Vial remapping/layers work
- [ ] MX-style four-direction gestures work
- [ ] gesture cursor suppression works
- [ ] continuous gesture works
- [ ] tap/gesture double-fire is prevented
- [ ] scroll layer works
- [ ] settings persist
- [ ] no resident PC software required for normal operation
