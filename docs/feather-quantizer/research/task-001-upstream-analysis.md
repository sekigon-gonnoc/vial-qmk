# Task 001 — Upstream KQM source analysis

## Scope and evidence labels

This is a read-only analysis of Keyboard Quantizer Mini (KQM Mini). No firmware,
build, dependency, submodule, workflow, or repository-control file was changed.

- **Verified source fact** means the named behavior is directly present in this
  checkout at the paths/symbols cited below.
- **Observed environment fact** means a command was run in this workspace.
- **Hypothesis** is a proposed port design, not behavior proved by this source.
- **Hardware unknown** requires Phase 0 capture or real-board testing.

## 1. Baseline identity

### Observed environment facts

| Item | Result |
|---|---|
| Current branch | `work` |
| HEAD | `6345edc81fb4821558a876ee83fb0a32e7dcee2f` |
| Pinned upstream baseline | `4de02c1aeec5a34f0744a97213499441c508d565` (`Update dependency declaration for bmp_settings.c to use order-only prerequisite`) |
| Remotes | None configured (`git remote -v` produced no output) |
| Expected upstream provenance | `sekigon-gonnoc/vial-qmk`, branch `dev/ble-micro-pro`; provenance only, per `docs/feather-quantizer/upstream-policy.md` |
| KQM Vial header | `keyboards/sekigon/keyboard_quantizer/mini/keymaps/vial/keymap.h` exists; no restoration was needed |

`git submodule status` reported all registered submodules with a leading `-`, so
they are not initialized in this worktree. Recorded commits were:

```text
be44b3305f9a9fe5f2f49a4e7b978db322dc463e lib/chibios
77cb0a4f7589f89e724f5e6ecb1d76d514dd1212 lib/chibios-contrib
e2239ee6043f73722e7aa812a459f54a28552929 lib/googletest
549b97320d515bfca2f95c145a67bd13be968faa lib/lufa
e19410f8f8a256609da72cff549598e0df6fa4cf lib/lvgl
a3398d8d3a772f37fef44a74743a1de69770e9c2 lib/pico-sdk
c2e3b4e10d281e7f0f694d3ecbd9f320977288cc lib/printf
819dbc1e5d5926b17e27e00ca6d3d2988adae04e lib/vusb
```

The working tree was already dirty before Task 001: tracked
`builddefs/docsgen/yarn.lock` was modified, and `builddefs/docsgen/.yarn/` plus
`builddefs/docsgen/.yarnrc.yml` were untracked. These pre-existing, out-of-scope
changes were preserved unchanged.

## 2. Baseline build

- Intended target command: `make sekigon/keyboard_quantizer/mini:vial` (equivalent
  QMK spelling: `qmk compile -kb sekigon/keyboard_quantizer/mini -km vial`).
- Codex workspace result: **not built / unverified in this workspace**. The
  command failed before compilation because `qmk` is absent
  (`/bin/sh: 1: qmk: not found` / `Cannot run "qmk hello"`).
- Toolchain: `arm-none-eabi-gcc` is also absent, so no compiler version was
  available. The only observed supporting runtime version was Python `3.14.4`.
- Codex workspace artifact: none.
- Repository CI result: **verified**. The repository's `Feather Quantizer -
  firmware build` CI has successfully completed the `Build upstream KQM Mini
  baseline` step and generated/uploaded the KQM baseline UF2. This is separate
  evidence from the unavailable local toolchain; it does not supply a local
  compiler version.
- Interpretation: the local failure is an environment/toolchain limitation, not
  evidence of a source failure. Per the dependency freeze, Task 001 did not
  install, initialize, or update anything to work around it. Baseline compilation
  and UF2 generation are verified by repository CI, but remain unverified in this
  Codex workspace.

## 3. Verified runtime path

### 3.1 RP2040 startup, clock, and core 1

1. `info.json` selects processor `RP2040` and bootloader `rp2040`
   (`keyboards/sekigon/keyboard_quantizer/mini/info.json`, keys `processor` and
   `bootloader`). The platform startup itself is QMK/ChibiOS framework code.
2. KQM's QMK keyboard hook `keyboard_pre_init_kb()` calls
   `set_sys_clock_khz(120000, true)`, then `keyboard_pre_init_user()`
   (`mini/mini.c`). Thus KQM explicitly changes the system clock to 120 MHz.
3. `mini/mcuconf.h` overrides `RP_CORE1_START` to `TRUE`, while
   `mini/rules.mk` adds `-DCRT0_EXTRA_CORES_NUMBER=1`. Together these are the
   KQM-side evidence that ChibiOS starts the second core and expects `c1_main()`.
4. `c1_main()` waits for the primary ChibiOS system with
   `chSysWaitSystemState(ch_sys_running)`, initializes/unlocks the core-1 ChibiOS
   instance, releases PIO0, PIO1, and DMA from reset, calls `c1_usbh()`, and
   creates `c1_main_task_wrapper()` (`mini/c1_main.c`). That thread repeatedly
   calls `c1_main_task()`, services the flash-operation trap, and sleeps 125 us.

### 3.2 PIO USB host and TinyUSB host

1. `c1_usbh()` copies `PIO_USB_DEFAULT_CONFIG`, overrides `pin_dp = 4`, sets
   `extra_error_retry_count = 10` and `skip_alarm_pool = true`, supplies it to
   TinyUSB root hub/port 1 using
   `tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg)`, then calls
   `tuh_init(1)` and `c1_start_timer()` (`mini/c1_usbh.c`). Pico-PIO-USB treats
   D- as the adjacent pin to D+, so this configuration is GP4/GP5.
2. `c1_start_timer()` installs a continuous 1 ms ChibiOS virtual timer. Its
   `timer_cb()` calls `pio_usb_host_frame()` and corrects the next SOF target;
   the host thread's `c1_main_task()` calls `tuh_task()` (`mini/c1_main.c`,
   `mini/c1_usbh.c`).
3. `mini/rules.mk` explicitly compiles Pico-PIO-USB PIO/host/CRC sources, TinyUSB
   host, hub, HID-host and RP2040 PIO-HCD sources, plus Pico SDK DMA support.
   `mini/tusb_config.h` sets `CFG_TUD_ENABLED=0`, `CFG_TUH_ENABLED=1`,
   `CFG_TUH_RPI_PIO_USB=1`, hub support, `CFG_TUH_DEVICE_MAX=4`, and
   `CFG_TUH_HID=8`. This TinyUSB instance is host-only; QMK's normal native
   RP2040 USB device stack is separate.

### 3.3 Host callbacks, descriptors, and reports

1. TinyUSB invokes `tuh_mount_cb()` for a device and `tuh_hid_mount_cb()` for an
   HID interface (`mini/matrix.c`). The device callback records keyboard address
   and instance by `HID_ITF_PROTOCOL_KEYBOARD`. The HID callback passes the
   report descriptor to `parse_report_descriptor()` using the synthetic
   interface key `(dev_addr * 16) + instance`, then arms
   `tuh_hid_receive_report()`.
2. `parse_report_descriptor()` tokenizes HID short items and stores top-level
   usage page/usage, report ID, report size/count, logical bounds, usage and usage
   ranges in fixed tables (`parser/report_descriptor_parser.c`). Its capacities
   are 8 devices, 16 report-ID collections, 32 report members, and 32 temporary
   usages (`parser/report_descriptor_parser.h`).
3. `tuh_hid_report_received_cb()` waits for the prior single shared 64-byte
   report buffer to be consumed, records the synthetic interface, copies the
   report, publishes `hid_report_size`, and immediately rearms reception
   (`mini/matrix.c`). There is no `len <= 64` guard in this callback; reports over
   64 bytes are therefore a compatibility/safety risk to verify.
4. On the QMK scan side, `matrix_scan_custom()` consumes that buffer via
   `parse_report()` (`mini/matrix.c`). `parse_report()` selects a collection by
   report ID (and removes the ID byte), then dispatches by top-level usage:
   keyboard `0x0106`, mouse `0x0102`, system control `0x0180`, consumer
   `0x0C01`, or `vendor_report_parser()` (`parser/report_parser.c`).
5. On unmount, `tuh_hid_umount_cb()` raises `hid_disconnect_flag`; the next
   `matrix_scan_custom()` clears the entire virtual matrix.

### 3.4 Mouse conversion and QMK/Vial processing

`mouse_report_parser()` uses descriptor-defined bit offsets and signed values:

| HID usage | Destination |
|---|---|
| Button page (`0x0009....`) | `mouse_parse_result_t.button` |
| Generic Desktop X `0x00010030` | `.x` |
| Generic Desktop Y `0x00010031` | `.y` |
| Generic Desktop Wheel `0x00010038` | `.v` |
| Consumer AC Pan `0x000C0238` | `.h` |
| Vendor page `0xFF00` special case | extra SlimBlade button bits |
| Other input | `.undefined` |

It then calls the weak `mouse_report_hook()` (`parser/report_parser.c`). The Vial
keymap supplies a strong replacement in `mini/keymaps/vial/quantizer_mouse.c`:

- Eight mouse buttons are projected into virtual matrix positions derived from
  `KC_MS_BTN1 + bit`, so they enter normal QMK matrix/action processing.
- `calc_mouse_scaled_move()` creates pointer, wheel, and reduced scroll-layer
  deltas (current scale is hard-coded to 16).
- Vertical/horizontal wheel movement is converted into virtual press/release
  actions at `KC_MS_WH_UP/DOWN` and `KC_MS_WH_LEFT/RIGHT` via `action_exec()`;
  `process_record_mouse()` converts those actions back to native mouse `v/h`
  reports after dynamic remapping.
- X/Y normally accumulates in `report_mouse_t` through
  `pointing_device_get_report()` / `pointing_device_set_report()`. Remapping
  `KC_MS_LEFT` or `KC_MS_UP` to wheel keycodes enables trackball-to-scroll.
- `pointing_device_task()` in `mini/matrix.c` observes `mouse_send_flag` and calls
  `pointing_device_send()`.

For keyboard input, `keyboard_report_parser()` makes a 256-bit usage bitmap.
`keyboard_report_hook()` copies usages into rows 1 onward and modifiers from byte
28 into row 0 (`mini/matrix.c`). The 32x8 virtual layout in `mini/info.json` and
the identity base keymap in `mini/keymaps/vial/keymap.c` let QMK matrix scanning
turn those positions into actions. Vial is enabled with eight dynamic layers in
`mini/keymaps/vial/rules.mk` and `config.h`; mouse remapping explicitly calls
`dynamic_keymap_get_keycode()` and walks active layers. `raw_hid.c` multiplexes
VIA and Vial protocol handlers, and the normal QMK dynamic-keymap/Vial machinery
persists mappings in wear-levelled storage (16 KiB backing, 8 KiB logical).

### 3.5 Native USB output to the PC

This KQM-specific code terminates in standard QMK output APIs:

- keyboard matrix actions flow through QMK `action_exec()`;
- pointing data flows through `pointing_device_send()`;
- system/consumer input uses `host_system_send()` / `host_consumer_send()` with
  a delayed zero release (`mini/matrix.c`);
- raw configuration traffic uses `raw_hid_send()` (`keymaps/vial/raw_hid.c`).

**Verified boundary:** `mini/tusb_config.h` disables only the separately compiled
PIO TinyUSB device stack (`CFG_TUD_ENABLED=0`) while QMK provides the native USB
device transport. **Not hardware verified:** simultaneous PIO host and native
USB device operation was not run in this environment.

## 4. KQM Mini hardware and sizing assumptions

| Assumption (verified source fact unless marked) | Evidence | Port significance |
|---|---|---|
| RP2040 MCU, RP2040 UF2 bootloader | `mini/info.json`: `processor`, `bootloader` | Correct MCU family, but Feather board/flash and reset behavior still need selection. |
| 120 MHz system clock | `keyboard_pre_init_kb()` in `mini/mini.c` | PIO USB timing depends on a supported clock; retain only after Feather validation. |
| Host D+ GP4, implicit paired D- GP5 | `pio_cfg.pin_dp = 4` in `mini/c1_usbh.c`; Pico-PIO-USB adjacent-pin convention | Must become GP16/GP17. |
| No explicit VBUS enable/control | No VBUS GPIO setup exists in KQM Mini keyboard sources | Feather requires GP18 asserted before enumeration. Polarity/startup timing require board evidence. |
| Activity LED GP7, active-low blink, initially high | `KQ_PIN_LED`, `matrix_init_custom()`, `housekeeping_task_kb()` in `mini/matrix.c` | GP7 must not be assumed to be Feather's user LED. |
| Core 1 is enabled; host task stack is 2048 bytes; loop sleep is 125 us | `mini/mcuconf.h`, `mini/rules.mk`, `wa_c1_main_task_wrapper` and thread loop in `mini/c1_main.c` | Concurrency, memory, and host servicing assumptions must survive the port. |
| PIO0, PIO1, and DMA are explicitly released from reset | `c1_main()` | Resource coexistence must be checked against native USB and other Feather features. |
| Host settings: retry 10, custom timer, 1 ms SOF | `c1_usbh()`, `timer_cb()` | Timing-sensitive; avoid casual changes. |
| TinyUSB host: hub enabled, `CFG_TUH_DEVICE_MAX=4`, `CFG_TUH_HID=8`, 256-byte enumeration buffer, and 64-byte HID IN/OUT endpoint buffers | `mini/tusb_config.h` | Composite receivers and descriptors must fit these limits. |
| One 64-byte shared input report buffer | `hid_report_buffer` in `mini/matrix.c` | Phase 0 must establish maximum interrupt report length and concurrency needs. |
| Synthetic interface key is `dev_addr * 16 + instance` | mount/report callbacks | Assumes instance fits the low nibble; `CFG_TUH_HID` permits eight HID interfaces. |
| Parser fixed pools: 8 devices / 16 collections / 32 members / 32 usages | `parser/report_descriptor_parser.h` | Complex ELECOM descriptors may exhaust these pools. |
| Virtual matrix is 32 rows x 8 columns; base map represents HID usage bytes; modifiers occupy row 0 | `mini/info.json`, Vial `keymap.c`, `keyboard_report_hook()` | Vial schema and mouse/gesture reserved positions depend on this shape. |
| Eight input mouse buttons are assumed | comment and `uint8_t button_current` in Vial `mouse_report_hook()` | Buttons beyond eight cannot be represented by the current path. |
| Gesture slots use row 31 and four columns | `MATRIX_MSGES_ROW 31`, `process_gesture()` | This is KQM UI/schema coupling, not a physical matrix. |
| Vial has 8 dynamic layers; storage is 16 KiB backing / 8 KiB logical | Vial `config.h` | Flash capacity/layout must be checked for Feather. |
| USB identity is FEED:999C; console, virtual serial, pointing, NKRO and extra-key endpoints enabled | `mini/info.json` | Endpoint/RAM use and product identity need deliberate Feather definitions. |
| Host keyboard LEDs are forwarded as HID output report ID 0 | `send_led_report()` in `mini/matrix.c` | Device output-report format may differ for composite/report-ID keyboards. |
| Flash writes stop the core-1 timer/loop | weak backing-store locks in `mini/config.h`, `mini.c`; trap in `c1_main.c` | Required because XIP flash operations and host execution interact. |
| Serial `b` and CLI boot commands call `bootloader_jump()`; QMK bootloader is RP2040 | `virtser_recv()` / CLI and `info.json` | Feather BOOTSEL/reset entry must be verified, not assumed. |
| Flash/board definition | **Unknown:** no KQM-local board file or explicit flash-size macro was found; build metadata resolves generic RP2040 defaults | Feather flash size and linker/storage placement require build-output inspection once toolchain is available. |

`MATRIX_MSBTN_ROW` in `mini/matrix.c` is defined as 22 but unused; the effective
mouse-button mapping instead derives rows from keycode values in the Vial hook.

## 5. Feather port seams

The approaches below are **hypotheses for a later task**, not Task 001 changes.

| Current KQM location/behavior | Feather requirement | Smallest future approach | Entirely under `keyboards/feather_quantizer/**`? | Risk if wrong |
|---|---|---|---|---|
| `mini/c1_usbh.c`: D+ GP4, implicit D- GP5 | D+ GP16 / D- GP17 | Feather-local copy/wrapper of host initialization setting `pin_dp=16`; document adjacent D-. | Yes | No enumeration, electrical contention, or wrong PIO sampling. |
| No KQM VBUS control | USB-A VBUS enable GP18 | Feather-local pre-init sets GP18 output to the hardware-documented active level before `tuh_init()`; Phase 0 confirms polarity and sequencing. | Yes, expected | Unpowered device or power fault/backfeed if polarity is wrong. |
| `info.json` generic RP2040 metadata and inherited platform board/linker defaults | Correct Feather RP2040 board and flash capacity/layout | Create Feather-local `info.json`, `rules.mk`, and only necessary local board/config files after examining an existing compatible QMK RP2040 board. Do not update SDK/toolchain. | Expected; if framework lacks a suitable local-board hook, stop for approval | Bad linker placement, storage overlap, failed boot, wrong USB identity. |
| `matrix.c`: GP7 active-low activity LED | Use actual Feather LED definition/pin or deliberately disable activity LED | Feather-local macro/config and implementation; do not carry GP7 literally. | Yes | Driving a connected peripheral pin or inverted/stuck LED. |
| `mini.c`: 120 MHz | Maintain PIO USB timing on Feather | Initially retain 120 MHz in Feather-local hook, then compile and hardware-test; change only with timing evidence. | Yes | Host packet/SOF timing failures or unstable system timing. |
| `mcuconf.h`, `rules.mk`, `c1_main.c`: core-1 PIO host | Preserve native RP2040 USB device plus PIO host | Feather-local target reuses the same split: QMK native USB on core 0, PIO TinyUSB host on core 1, with host `CFG_TUD_ENABLED=0`. | Yes, expected | IRQ/DMA/PIO collision, starvation, disconnects, or device enumeration failure. |
| `mini.c` backing-store locks stop core 1 | Safe Vial persistence | Carry the local lock/trap handshake with the host loop. | Yes | Flash writes can crash/stall host processing or corrupt persistence. |
| `mini/matrix.c` + shared parser | Receive arbitrary trackball reports | Initially copy the smallest parser/bridge into Feather-local code or reference existing parser without editing it; size limits must be validated first. | Yes for a copy/local implementation; shared reuse may require build `VPATH` only | Silent report loss, buffer overflow, stale buttons. |
| Vial keymap's 32x8 virtual matrix and reserved gesture row | Preserve remapping/layers while avoiding KQM modifications | Define Feather-local matrix, Vial JSON/config, and identity mapping; port behavior in stages. | Yes | Incompatible stored maps, wrong usage-to-position mapping, inaccessible buttons. |

No verified source fact currently forces a shared/upstream edit. If a future build
shows that a needed RP2040 board hook cannot be supplied locally, work must stop
at that boundary and request approval under guardrail G1.

## 6. ELECOM DEFT / HUGE PLUS compatibility path

Compatibility is determined at these exact seams:

1. TinyUSB enumeration supplies each HID report descriptor to
   `tuh_hid_mount_cb()`.
2. `parse_report_descriptor()` decides whether the descriptor fits the fixed
   pools and records top-level collections, report IDs, field sizes/counts,
   logical ranges, and usages.
3. `parse_report()` matches the incoming first byte to a report-ID collection and
   dispatches based on top-level usage.
4. `mouse_report_parser()` recognizes buttons, X/Y, wheel and AC Pan; unrecognized
   fields only enter `.undefined`, while unrecognized top-level collections enter
   `vendor_report_parser()`.
5. The Vial `mouse_report_hook()` limits the actionable button bitmap to eight
   bits and maps them into the virtual matrix.

### Phase 0 capture required for each connection mode

For the exact DEFT product number and HUGE PLUS, and separately for wired versus
2.4 GHz receiver modes where applicable, capture:

- USB VID, PID, product/revision strings, interface numbers, endpoint addresses,
  polling intervals, maximum packet sizes, and whether a hub/composite device is
  presented;
- the complete raw HID report descriptor for every HID interface;
- every input report ID and exact byte length, plus idle and transition samples;
- annotated reports for each button pressed and released singly and in useful
  combinations, explicitly including all extra buttons;
- positive/negative X and Y movement, vertical wheel both ways, and physical tilt
  or horizontal-pan both ways, determining whether horizontal motion uses
  Consumer AC Pan, another standard usage, or a vendor field;
- consumer/system controls and all vendor-defined reports/usages, including
  whether vendor initialization/output feature reports are needed before full
  functionality appears;
- output reports such as keyboard LED behavior if the receiver exposes a
  keyboard interface;
- maximum descriptor complexity versus the 8/16/32/32 parser capacities and
  maximum report length versus the 64-byte buffer.

**Hardware unknown:** the repository contains no captured descriptor or report
corpus identified as ELECOM DEFT or ELECOM HUGE PLUS. Therefore this analysis
does not claim compatibility for either device. A standard usage alone is not
enough: report-ID layout, field widths, signed logical minima, extra-button count,
and vendor initialization can all affect the result.

## 7. Existing gesture behavior and future seam

### Verified existing KQM behavior

- A Layer-Tap press starts recognition in `pre_process_record_mouse()`; a
  Momentary-layer press starts it in `post_process_record_mouse()`.
- `gesture_start()` clears signed 16-bit X/Y accumulators and sets
  `gesture_wait`. While waiting, the Vial `mouse_report_hook()` accumulates scaled
  X/Y after its normal movement/remap handling. The gesture mechanism itself does
  not automatically suppress cursor movement. An active layer can independently
  remap X/Y movement to wheel movement, which may result in no cursor movement,
  but that is a consequence of the movement remap rather than gesture-mode
  suppression.
- Recognition occurs only on LT/MO **release** in
  `post_process_record_mouse()`, not when the threshold is crossed.
- `recognize_gesture()` requires Manhattan distance
  `abs(x)+abs(y) >= 50` and chooses one of four **diagonal quadrants** based only
  on the signs: down-right, down-left, up-left, up-right.
- `process_gesture()` looks up one of four reserved dynamic-keymap cells on the
  gesture layer and calls `vial_keycode_tap()`. There is no cooldown, accumulator
  reset for repetition, or repeated firing while held.
- There is no explicit “gesture succeeded” state that cancels an LT tap. Because
  processing happens after the normal release path, the source does not prove
  mutual exclusion between the LT tap and gesture action.

### Proposed future Feather insertion points (hypothesis)

Keep this state machine in a Feather-local equivalent of
`keymaps/vial/quantizer_mouse.c`:

1. On the configured gesture-button/LT/MO press, initialize accumulator,
   cooldown deadline, and `gesture_succeeded=false`.
2. In Feather `mouse_report_hook()`, while gesture mode is active, accumulate raw
   or consistently scaled deltas and do **not** add X/Y to `report_mouse_t`.
   Buttons and wheels should have explicitly specified behavior rather than being
   accidentally suppressed.
3. After each accumulated report and outside cooldown, test the configured
   threshold immediately. Select the dominant axis: `abs(x) >= abs(y)` gives
   left/right; otherwise up/down. Fire the mapped action at threshold time, set
   `gesture_succeeded=true`, clear accumulators, and begin the 150 ms provisional
   cooldown.
4. When cooldown expires while the button remains held, continue accumulating and
   permit another threshold firing. Define whether movement during cooldown is
   discarded (simplest/predictable) or accumulated; test this on hardware.
5. On release, if no gesture succeeded, allow the configured tap action; if any
   gesture succeeded, suppress that tap. Integrate at pre/process/post-record
   boundaries carefully so QMK tapping does not emit the tap first.

Risks include signed 16-bit accumulator overflow, diagonal jitter around dominant
axis ties, leaked cursor deltas, duplicate LT taps, and action recursion through
the virtual matrix. Unit-testable recognition/state transitions should be kept
separate from QMK report plumbing in the later gesture task.

## 8. Unknowns and blockers

- The baseline build and UF2 generation succeeded in repository CI. A local
  reproduction, local compiler version, resolved board, linker/flash layout,
  firmware size, and warnings remain unknown in this Codex workspace because QMK
  CLI, ARM GCC, and initialized submodules are unavailable.
- Feather VBUS-enable polarity and required stabilization delay were not verified
  against hardware in this source-only task.
- Feather onboard LED wiring and whether it should be used for host activity need
  an authoritative board definition/schematic and hardware test.
- Native USB plus PIO-host coexistence, suspend/resume, receiver hot-plug, hub
  behavior, and flash-write pauses are not verified on Feather hardware.
- Exact DEFT model, both ELECOM devices' descriptors/reports, extra-button count,
  pan encoding, and vendor protocol requirements remain Phase 0 unknowns.
- The pre-existing docsgen working-tree changes prevent a globally clean status;
  only the Task 001 report is attributable to this task.

## 9. Recommended smallest coherent Task 002

**Task 002: Phase 0 capture procedure and parser-fixture schema (documentation
only).** Do not start the Feather QMK target until real-device evidence exists.

Expected files:

```text
docs/feather-quantizer/phase-0-capture.md
docs/feather-quantizer/research/hid-captures/README.md
```

Acceptance criteria:

1. Documents a reproducible, non-destructive Feather/Adafruit-example procedure
   for capturing USB identity, configuration/interface/endpoint descriptors, all
   HID report descriptors, and timestamped raw input reports.
2. Provides a deterministic naming and metadata schema for DEFT/HUGE PLUS and
   wired/receiver modes without committing generated firmware or dependencies.
3. Includes a stimulus checklist for every button, X/Y, wheel, tilt/pan,
   simultaneous inputs, report IDs, report sizes, and vendor-defined traffic.
4. Includes safety steps for GP18 VBUS enable and explicitly records its observed
   polarity/timing rather than assuming it.
5. Defines how captures will be compared against the current 64-byte report limit,
   eight-button limit, and descriptor-parser pool capacities.
6. Makes no compatibility claim and records hardware unavailable/unperformed
   sections explicitly.

This task is smaller and less risky than firmware scaffolding: it resolves the
highest-impact device and board unknowns while remaining entirely under
`docs/feather-quantizer/**`.
