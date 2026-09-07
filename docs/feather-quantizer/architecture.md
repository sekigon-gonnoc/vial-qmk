# Architecture

## Intended data path

```text
USB HID report from trackball
        v
PIO USB Host / TinyUSB Host on RP2040
        v
HID report parser
        v
normalized pointer/buttons/wheels
        v
QMK virtual matrix / pointing-device integration
        v
Vial dynamic keymap + layer/action engine
        v
Feather-specific gesture/scroll processing
        v
RP2040 native USB HID device
        v
PC
```

## Porting strategy

The project does not modify the existing KQM Mini target as its normal implementation mechanism.

Instead:

1. study `keyboards/sekigon/keyboard_quantizer/mini/**` as upstream reference;
2. create the Feather-specific target under `keyboards/feather_quantizer/**`;
3. copy only the minimum implementation necessary to establish a buildable target, preserving licensing headers;
4. make hardware-specific changes locally in the Feather target;
5. add gesture behavior only after ordinary HID passthrough and Vial operation are verified.

## Known hardware seams to verify in Task 1 / Phase 0

- KQM Mini PIO USB D+ pin versus Feather GP16
- Feather D- is paired on GP17
- Feather USB-A VBUS enable on GP18
- any KQM-specific LED pin assumptions
- RP2040 board/flash configuration for the Feather's 8 MB flash
- native USB device configuration and Vial identity
- Core 0 / Core 1 ownership of QMK and USB host processing
- device-specific report IDs/descriptors for DEFT and HUGE PLUS

## Design principle

A buildable board port and ordinary pointer passthrough are prerequisites for gesture work. Gesture logic must not be used to hide unresolved USB-host or HID-parser issues.
