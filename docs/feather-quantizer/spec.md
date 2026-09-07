# Specification v0.1

## Objective

Build a USB HID proxy on Adafruit Feather RP2040 with USB Type A Host (#5723) that gives ordinary USB trackballs QMK/Vial-style remapping, layers, macros, scroll layers, and MX Master-style directional gestures.

## Hardware topology

```text
Trackball / 2.4 GHz USB receiver
        |
        | USB HID
        v
Feather RP2040 USB Type A Host
  - PIO USB host: D+ GP16, D- GP17
  - USB-A VBUS enable: GP18
  - RP2040 native USB: device connection to PC
        |
        | USB-C
        v
Windows PC
```

## Target devices

Primary:

- ELECOM HUGE PLUS

Secondary / initial development:

- ELECOM DEFT (exact product number to be captured during Phase 0)

Bluetooth is out of scope for v0.1. Use USB wired mode or the 2.4 GHz USB receiver.

## Required v0.1 behavior

- normal pointer X/Y passthrough
- left/right/middle and supported extra mouse buttons
- vertical wheel and horizontal pan/tilt when exposed by the device
- Vial/Remap configurable key assignment
- QMK layers including MO/LT-style use
- trackball-to-scroll layer
- four-direction gesture mode: up/down/left/right
- cursor movement suppressed while a gesture is being recognized
- gesture fires when the movement threshold is reached, without requiring button release
- repeated gestures while the gesture button remains held
- tap action and gesture action must not both fire for the same press when a gesture succeeded
- settings persist across power cycles
- normal operation requires no resident PC software

## Initial gesture model

Accumulate trackball delta while gesture mode is active:

```text
gesture_x += dx
gesture_y += dy
```

When the configured threshold is reached, use the dominant axis:

```text
abs(x) >= abs(y) -> LEFT or RIGHT
abs(y) >  abs(x) -> UP or DOWN
```

After firing, clear the accumulator and enforce a small cooldown before accepting the next gesture.

Initial tuning values are provisional and must be verified on real hardware:

- threshold: 30 logical movement units
- cooldown: 150 ms

## Out of scope for v0.1

- direct Bluetooth host support
- application-aware automatic layer switching on the PC
- custom desktop configuration application
- dependency/toolchain modernization
- refactoring the upstream Keyboard Quantizer implementation
