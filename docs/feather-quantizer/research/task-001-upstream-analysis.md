# Task 001 — Upstream KQM source analysis

Status: NOT STARTED

This file is the only repository file Task 001 is permitted to create or update.

## 1. Baseline identity

- current branch:
- HEAD commit:
- upstream remote:
- upstream base branch:
- submodule status:

## 2. Baseline build

- command:
- compiler/toolchain:
- result:
- generated artifact:
- warnings/errors:

## 3. Runtime path

Document the exact call/data flow from RP2040 startup through USB host initialization, HID report parsing, QMK/Vial processing, and native USB HID output.

## 4. Source map

| Responsibility | File | Function / macro | Notes |
|---|---|---|---|
| USB host init | | | |
| Core 1 host loop | | | |
| TinyUSB callbacks | | | |
| HID report parser | | | |
| matrix conversion | | | |
| pointing device | | | |
| Vial dynamic keymap | | | |
| gesture handling | | | |
| native USB output | | | |

## 5. Hardware assumptions in KQM Mini

Record every KQM-specific pin, LED, clock, flash, board, boot, or power assumption found in source.

## 6. Feather port seams

For each anticipated Feather change, identify:

- source location
- current KQM behavior
- Feather requirement
- whether the change can stay entirely under `keyboards/feather_quantizer/**`
- risk

## 7. HID compatibility considerations

Identify where DEFT/HUGE PLUS report IDs, button counts, horizontal pan, and vendor-defined reports would be handled.

Do not claim device support without Phase 0 hardware evidence.

## 8. Gesture implementation baseline

Document how upstream KQM gesture recognition currently works and where MX-style four-direction/cursor-suppression behavior should be added in the Feather target.

## 9. Recommended Task 002

Propose the smallest coherent implementation task for the Feather board port. List exact expected files, but do not modify them in Task 001.

## 10. Unknowns / blockers

List unresolved facts separately from conclusions.
