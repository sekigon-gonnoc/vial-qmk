# Codex Task 001 — Analyze KQM upstream before any Feather implementation

## Goal

Analyze the existing Keyboard Quantizer Mini implementation in this repository and produce a reviewable porting report for Adafruit Feather RP2040 with USB Type A Host (#5723).

This is an analysis task, not an implementation task.

## Mandatory instructions

1. Read the repository-root `AGENTS.md` and all referenced Feather project docs before doing work.
2. Do not modify firmware source code, upstream source, dependency files, submodules, build configuration, workflows, or AGENTS.md.
3. The only repository file you may create or modify is:
   `docs/feather-quantizer/research/task-001-upstream-analysis.md`
4. You may run read-only inspection commands and builds.
5. If a build needs the historically missing KQM Vial `keymap.h`, use the repository helper script or a temporary working-tree restoration that is removed before completion. Do not commit it to the upstream KQM path.
6. End with `git status --short`; apart from the designated report file, the working tree must remain unchanged.

## Required analysis

### A. Establish baseline

Record:

- current branch and HEAD
- remotes
- submodule state
- target build command for `sekigon/keyboard_quantizer/mini:vial`
- toolchain/compiler version used by the build
- whether the baseline builds successfully

### B. Trace the runtime path

Trace the actual source flow, with file paths and functions, for:

1. RP2040 startup / clock setup
2. Core 1 startup
3. PIO USB host initialization
4. TinyUSB host task/callbacks
5. HID descriptor/report parsing
6. mouse buttons, pointer X/Y, vertical wheel, horizontal pan conversion
7. conversion into QMK matrix/pointing-device state
8. Vial dynamic keymap/layer processing
9. gesture handling in the existing KQM Vial keymap
10. native RP2040 USB HID output to the PC

Do not infer a path when source evidence is available. Name the exact functions/macros that prove each step.

### C. Find every KQM Mini hardware assumption

Search for and report all relevant assumptions such as:

- USB host D+ pin
- paired D- behavior
- LED pin
- system clock
- flash/board configuration
- USB power/VBUS handling
- bootloader/reset behavior
- any KQM-only matrix/report dimensions

### D. Map the Feather port seams

For each required Feather adaptation, state:

- current KQM source location
- Feather requirement
- smallest future implementation approach
- whether it can remain entirely inside `keyboards/feather_quantizer/**`
- risk if implemented incorrectly

At minimum assess:

- PIO USB D+ = GP16 / D- = GP17
- USB-A VBUS enable = GP18
- board/flash configuration
- LED assumptions
- native USB device coexistence

### E. Device compatibility path

Identify exactly where support for ELECOM DEFT and ELECOM HUGE PLUS would be determined by their HID descriptors/reports.

Explain what Phase 0 must capture from real hardware, especially extra buttons, wheel tilt/horizontal pan, report IDs, and vendor-defined reports.

Do not claim either device is fully compatible unless the repository itself contains sufficient evidence.

### F. Existing gesture behavior

Document the existing KQM gesture algorithm and its trigger timing. Identify where a future Feather implementation should introduce:

- 4-direction dominant-axis recognition
- cursor suppression during gesture mode
- threshold-time immediate firing
- cooldown
- continuous gestures while held
- suppression of tap action after a successful gesture

Do not implement these in Task 001.

## Deliverable

Complete:

`docs/feather-quantizer/research/task-001-upstream-analysis.md`

The report must distinguish verified source facts from hypotheses and hardware-dependent unknowns.

Finish by proposing **one smallest coherent Task 002** with exact expected files and acceptance criteria.

## Final verification

Run:

```bash
git diff --check
git status --short
git diff --stat
git diff --name-only
```

The final changed-file list must contain only:

```text
docs/feather-quantizer/research/task-001-upstream-analysis.md
```
