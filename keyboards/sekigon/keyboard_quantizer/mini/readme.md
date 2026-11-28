# Keyboard Quantizer Mini

![sekigon/keyboard_quantizer/mini](https://i.gyazo.com/aca68f4592ff92eea99a55d710bd2ae3.jpg)

*A keyboard converter powered by QMK*

* Keyboard Maintainer: [sekigon-gonnoc](https://github.com/sekigon-gonnoc)
* Hardware Supported: The hardware of [Keyboard Qauntizer](https://github.com/sekigon-gonnoc/keyboard-quantizer-doc)
* Hardware Availability: Available at [BOOTH](https://nogikes.booth.pm/items/2256612)

Make example for this keyboard (after setting up your build environment):

    make sekigon/keyboard_quantizer/mini:default

Flashing example for this keyboard:

    make sekigon/keyboard_quantizer/mini:default:flash

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## Bootloader

Enter the bootloader in 2 ways:

* **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available
* **Virtser**: Send `b` to the device through virtser

## USB packet capture debug

Build the Vial keymap with raw USB capture enabled:

    make sekigon/keyboard_quantizer/mini:vial USB_PACKET_CAPTURE_DEBUG=yes

Serve `capture-gui/` from `localhost` or another secure origin, connect with
Web Serial, then use **Arm / Resume** and **Save now**. Sampling starts at the
next non-SOF USB Host transaction and follows its Full Speed or Low Speed
setting. USB Host communication errors also request a freeze. Use
**Fetch capture** to decode the retained samples, or **Clear buffer** to start
over.

Enable **Auto-save on valid IN DATA+ACK** before arming to freeze after the
first CRC-valid IN data packet accepted by the Host. The GUI filters the packet
list to ACK-completed IN transactions, and packet details show the DATA-to-ACK gap
in bit times. The raw capture retains preceding bus history for context.

The capture ring is 16 KiB. Its final sample boundary is a DMA boundary rather
than a detected USB EOP, so the last packet can be incomplete; the GUI marks
that condition in the status summary.
