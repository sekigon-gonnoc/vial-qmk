# Feather Keyboard Quantizer

This repository is a minimal-diff port of Keyboard Quantizer Mini concepts to the **Adafruit Feather RP2040 with USB Type A Host (#5723)**.

The project keeps the upstream `sekigon-gonnoc/vial-qmk` implementation intact wherever possible and develops the Feather-specific implementation in a separate keyboard target.

## Primary goal

Use an ordinary USB trackball through the Feather as a programmable QMK/Vial HID proxy, including:

- button remapping
- QMK/Vial layers
- tap/hold and macros
- trackball-to-scroll layers
- Logitech MX Master / Logi Options+-style directional gestures
- no PC-side resident software for normal operation

## Target hardware

- Adapter: Adafruit Feather RP2040 with USB Type A Host (#5723)
- Current test device: ELECOM DEFT (exact model to be recorded during Phase 0)
- Primary future target: ELECOM HUGE PLUS
- Host PC: Windows

## Repository policy

The existing upstream implementation under `keyboards/sekigon/**` is reference code and is treated as read-only by default.

Feather implementation code belongs under:

```text
keyboards/feather_quantizer/**
```

Project-specific documentation belongs under:

```text
docs/feather-quantizer/**
```

Read `AGENTS.md` and `docs/feather-quantizer/guardrails.md` before making changes.

## Development stages

1. Baseline: verify upstream KQM Mini build.
2. Task 1: read-only source analysis and port-plan report.
3. Phase 0: verify DEFT/HUGE PLUS HID reports on the Feather using Adafruit examples.
4. Port: create the Feather QMK/Vial target without modifying the KQM Mini target.
5. Passthrough: pointer, buttons, vertical/horizontal wheel.
6. Vial remapping and layers.
7. MX Master-style gesture engine.
8. Scroll layer, tap/gesture coexistence, tuning and regression tests.

See `docs/feather-quantizer/development-plan.md` for acceptance criteria.
