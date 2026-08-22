/* Copyright 2020 sekigon-gonnoc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H

#include "pointing_device.h"
#include "virtser.h"
#include "eeconfig.h"
#include "vial.h"

#include "keymap.h"
#include "quantizer_mouse.h"
#include "report_parser.h"
#include "cli.h"
#include "os_key_override.h"

const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {{
    {0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7},
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07},
    {0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
    {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17},
    {0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f},
    {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27},
    {0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f},
    {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37},
    {0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f},
    {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47},
    {0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f},
    {0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57},
    {0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f},
    {0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67},
    {0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f},
    {0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77},
    {0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f},
    {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87},
    {0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f},
    {0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97},
    {0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f},
    {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7},
    {0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
    {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7},
    {0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf},
    {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7},
    {0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf},
    {0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7},
    {0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf},
    {0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7},
    {0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef},
}};
user_config_t user_config;

int8_t virtser_send_wrap(uint8_t c) {
    virtser_send(c);
    return 0;
}

void keyboard_pre_init_user(void) {
    cli_init();
    print_set_sendchar(virtser_send_wrap);
}

static uint8_t get_gesture_threshold(void) {
    return 50;
}

void keyboard_post_init_user(void) {
    set_mouse_gesture_threshold(get_gesture_threshold());
    os_key_override_init();

    user_config.raw = eeconfig_read_user();
    switch (user_config.key_os_override) {
        case KEY_OS_OVERRIDE_DISABLE:
            remove_all_os_key_overrides();
            break;
        case US_KEY_JP_OS_OVERRIDE_DISABLE:
            register_us_key_on_jp_os_overrides();
            break;
        case JP_KEY_US_OS_OVERRIDE_DISABLE:
            register_jp_key_on_us_os_overrides();
            break;
    }
}

bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) {
    return pre_process_record_mouse(keycode, record);
}

#define DEFFERED_KEY_RECORD_LEN 6
static keyrecord_t deferred_key_record[DEFFERED_KEY_RECORD_LEN];

static void push_deferred_key_record(uint16_t keycode, keyevent_t *event) {
    for (int i = 0; i < DEFFERED_KEY_RECORD_LEN; i++) {
        if (deferred_key_record[i].keycode == KC_NO) {
            keyrecord_t record     = {.event = *event, .keycode = keycode};
            deferred_key_record[i] = record;
            return;
        }
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    bool cont = process_record_mouse(keycode, record);

    // To apply key overrides to keycodes combined shift modifier, separate to two actions
    if (keycode >= QK_MODS && keycode <= QK_MODS_MAX) {
        if (record->event.pressed) {
            register_mods(QK_MODS_GET_MODS(keycode));
            uint16_t   deferred_keycode   = QK_MODS_GET_BASIC_KEYCODE(keycode);
            keyevent_t deferred_key_event = (keyevent_t){.type = KEY_EVENT, .key = (keypos_t){.row = VIAL_MATRIX_MAGIC, .col = VIAL_MATRIX_MAGIC}, .pressed = 1, .time = (timer_read() | 1)};
            push_deferred_key_record(deferred_keycode, &deferred_key_event);
        } else {
            uint16_t   deferred_keycode   = QK_MODS_GET_BASIC_KEYCODE(keycode);
            keyevent_t deferred_key_event = ((keyevent_t){.type = KEY_EVENT, .key = (keypos_t){.row = VIAL_MATRIX_MAGIC, .col = VIAL_MATRIX_MAGIC}, .pressed = 0, .time = (timer_read() | 1)});
            unregister_mods(QK_MODS_GET_MODS(keycode));
            push_deferred_key_record(deferred_keycode, &deferred_key_event);
        }
        return false;
    }

    if (record->event.pressed) {
        switch (keycode) {
            case QK_KB_0:
            remove_all_os_key_overrides();
            user_config.key_os_override = KEY_OS_OVERRIDE_DISABLE;
            eeconfig_update_user(user_config.raw);
            return false;

            case QK_KB_1:
            register_us_key_on_jp_os_overrides();
            user_config.key_os_override = US_KEY_JP_OS_OVERRIDE_DISABLE;
            eeconfig_update_user(user_config.raw);
            return false;

            case QK_KB_2:
            register_jp_key_on_us_os_overrides();
            user_config.key_os_override = JP_KEY_US_OS_OVERRIDE_DISABLE;
            eeconfig_update_user(user_config.raw);
            return false;
        }
    }

    return cont;
}

void post_process_record_user(uint16_t keycode, keyrecord_t* record) {
    post_process_record_mouse(keycode, record);
}

void housekeeping_task_user(void) {
    for (int i = 0; i < DEFFERED_KEY_RECORD_LEN; i++) {
        if (deferred_key_record[i].keycode != KC_NO) {
            g_vial_magic_keycode_override = deferred_key_record[i].keycode;
            action_exec(deferred_key_record[i].event);
            deferred_key_record[i].keycode = KC_NO;
        } else {
            break;
        }
    }
    cli_exec();
}

#include "vial.h"
#include "dynamic_keymap.h"
#include "send_string.h"

#ifdef VIA_ENABLE   
#    include "via.h"
#    define DYNAMIC_KEYMAP_EEPROM_START (VIA_EEPROM_CONFIG_END)
#else
#    include "eeconfig.h"
#    define DYNAMIC_KEYMAP_EEPROM_START (EECONFIG_SIZE)
#endif

#ifdef ENCODER_ENABLE
#    include "encoder.h"
#else
#    define NUM_ENCODERS 0
#endif

#ifndef DYNAMIC_KEYMAP_EEPROM_MAX_ADDR
#    define DYNAMIC_KEYMAP_EEPROM_MAX_ADDR (TOTAL_EEPROM_BYTE_COUNT - 1)
#endif

// If DYNAMIC_KEYMAP_EEPROM_ADDR not explicitly defined in config.h,
#ifndef DYNAMIC_KEYMAP_EEPROM_ADDR
#    define DYNAMIC_KEYMAP_EEPROM_ADDR DYNAMIC_KEYMAP_EEPROM_START
#endif

// Encoders are located right after the dynamic keymap
#define VIAL_ENCODERS_EEPROM_ADDR (DYNAMIC_KEYMAP_EEPROM_ADDR + (DYNAMIC_KEYMAP_LAYER_COUNT * MATRIX_ROWS * MATRIX_COLS * 2))
#define DYNAMIC_KEYMAP_ENCODER_EEPROM_ADDR VIAL_ENCODERS_EEPROM_ADDR

#define VIAL_ENCODERS_SIZE (NUM_ENCODERS * DYNAMIC_KEYMAP_LAYER_COUNT * 2 * 2)

// QMK settings area is just past encoders
#define VIAL_QMK_SETTINGS_EEPROM_ADDR (VIAL_ENCODERS_EEPROM_ADDR + VIAL_ENCODERS_SIZE)

#ifdef QMK_SETTINGS
#include "qmk_settings.h"
#define VIAL_QMK_SETTINGS_SIZE (sizeof(qmk_settings_t))
#else
#define VIAL_QMK_SETTINGS_SIZE 0
#endif

// Tap-dance
#define VIAL_TAP_DANCE_EEPROM_ADDR (VIAL_QMK_SETTINGS_EEPROM_ADDR + VIAL_QMK_SETTINGS_SIZE)

#ifdef VIAL_TAP_DANCE_ENABLE
#define VIAL_TAP_DANCE_SIZE (sizeof(vial_tap_dance_entry_t) * VIAL_TAP_DANCE_ENTRIES)
#else
#define VIAL_TAP_DANCE_SIZE 0
#endif

// Combos
#define VIAL_COMBO_EEPROM_ADDR (VIAL_TAP_DANCE_EEPROM_ADDR + VIAL_TAP_DANCE_SIZE)

#ifdef VIAL_COMBO_ENABLE
#define VIAL_COMBO_SIZE (sizeof(vial_combo_entry_t) * VIAL_COMBO_ENTRIES)
#else
#define VIAL_COMBO_SIZE 0
#endif

// Key overrides
#define VIAL_KEY_OVERRIDE_EEPROM_ADDR (VIAL_COMBO_EEPROM_ADDR + VIAL_COMBO_SIZE)

#ifdef VIAL_KEY_OVERRIDE_ENABLE
#define VIAL_KEY_OVERRIDE_SIZE (sizeof(vial_key_override_entry_t) * VIAL_KEY_OVERRIDE_ENTRIES)
#else
#define VIAL_KEY_OVERRIDE_SIZE 0
#endif

// Dynamic macro
#ifndef DYNAMIC_KEYMAP_MACRO_EEPROM_ADDR
#    define DYNAMIC_KEYMAP_MACRO_EEPROM_ADDR (VIAL_KEY_OVERRIDE_EEPROM_ADDR + VIAL_KEY_OVERRIDE_SIZE)
#endif

// Dynamic macros are stored after the keymaps and use what is available
// up to and including DYNAMIC_KEYMAP_EEPROM_MAX_ADDR.
#ifndef DYNAMIC_KEYMAP_MACRO_EEPROM_SIZE
#    define DYNAMIC_KEYMAP_MACRO_EEPROM_SIZE (DYNAMIC_KEYMAP_EEPROM_MAX_ADDR - DYNAMIC_KEYMAP_MACRO_EEPROM_ADDR + 1)
#endif

#ifndef DYNAMIC_KEYMAP_MACRO_DELAY
#    define DYNAMIC_KEYMAP_MACRO_DELAY TAP_CODE_DELAY
#endif

static uint16_t decode_keycode(uint16_t kc) {
    /* map 0xFF01 => 0x0100; 0xFF02 => 0x0200, etc */
    if (kc > 0xFF00)
        return (kc & 0xFF) << 8;
    return kc;
}

void dynamic_keymap_macro_send(uint8_t id) {
    if (id >= DYNAMIC_KEYMAP_MACRO_COUNT) {
        return;
    }

    // Check the last byte of the buffer.
    // If it's not zero, then we are in the middle
    // of buffer writing, possibly an aborted buffer
    // write. So do nothing.
    void *p = (void *)(DYNAMIC_KEYMAP_MACRO_EEPROM_ADDR + DYNAMIC_KEYMAP_MACRO_EEPROM_SIZE - 1);
    if (eeprom_read_byte(p) != 0) {
        return;
    }

    // Skip N null characters
    // p will then point to the Nth macro
    p         = (void *)(DYNAMIC_KEYMAP_MACRO_EEPROM_ADDR);
    void *end = (void *)(DYNAMIC_KEYMAP_MACRO_EEPROM_ADDR + DYNAMIC_KEYMAP_MACRO_EEPROM_SIZE);
    while (id > 0) {
        // If we are past the end of the buffer, then the buffer
        // contents are garbage, i.e. there were not DYNAMIC_KEYMAP_MACRO_COUNT
        // nulls in the buffer.
        if (p == end) {
            return;
        }
        if (eeprom_read_byte(p) == 0) {
            --id;
        }
        ++p;
    }

    // Send the macro string one or three chars at a time
    // by making temporary 1 or 3 char strings
    char data[4] = {0, 0, 0, 0};
    // We already checked there was a null at the end of
    // the buffer, so this cannot go past the end
    while (1) {
        data[0] = eeprom_read_byte(p++);
        data[1] = 0;
        // Stop at the null terminator of this macro string
        if (data[0] == 0) {
            break;
        }
        if (data[0] == SS_QMK_PREFIX) {
            // If the char is magic, process it as indicated by the next character
            // (tap, down, up, delay)
            data[1] = eeprom_read_byte(p++);
            if (data[1] == 0)
                break;
            if (data[1] == SS_TAP_CODE || data[1] == SS_DOWN_CODE || data[1] == SS_UP_CODE) {
                // For tap, down, up, just stuff it into the array and send_string it
                data[2] = eeprom_read_byte(p++);
                if (data[2] != 0)
                    send_string(data);
            } else if (data[1] == VIAL_MACRO_EXT_TAP || data[1] == VIAL_MACRO_EXT_DOWN || data[1] == VIAL_MACRO_EXT_UP) {
                data[2] = eeprom_read_byte(p++);
                if (data[2] != 0) {
                    data[3] = eeprom_read_byte(p++);
                    if (data[3] != 0) {
                        uint16_t kc;
                        memcpy(&kc, &data[2], sizeof(kc));
                        kc = decode_keycode(kc);
                        switch (data[1]) {
                        case VIAL_MACRO_EXT_TAP:
                            vial_keycode_tap(kc);
                            break;
                        case VIAL_MACRO_EXT_DOWN:
                            vial_keycode_down(kc);
                            break;
                        case VIAL_MACRO_EXT_UP:
                            vial_keycode_up(kc);
                            break;
                        }
                    }
                }
            } else if (data[1] == SS_DELAY_CODE) {
                // Check delay code type is VIA(0xC) or not
                bool is_via = true;
                int  ms     = 0;
                for (int idx = 0; idx < 5; idx++) {
                    uint8_t c = eeprom_read_byte(p + idx);
                    if (c >= '0' && c <= '9') {
                        ms = ms * 10 + c - '0';
                    } else if (c == '|' && idx != 0) {
                        p += idx + 1;
                        break;
                    } else {
                        is_via = false;
                        break;
                    }
                }

                if (!is_via) {
                    // For delay, decode the delay and wait_ms for that amount
                    uint8_t d0 = eeprom_read_byte(p++);
                    uint8_t d1 = eeprom_read_byte(p++);
                    if (d0 == 0 || d1 == 0) break;
                    // we cannot use 0 for these, need to subtract 1 and use 255 instead of 256 for delay calculation
                    ms = (d0 - 1) + (d1 - 1) * 255;
                }

                while (ms--) wait_ms(1);
            }
        } else {
            // If the char wasn't magic, just send it
            send_string_with_delay(data, DYNAMIC_KEYMAP_MACRO_DELAY);
        }
    }
} // test build
bool led_update_user(led_t led_state) {
    if (led_state.caps_lock) {
        writePinLow(GP25);
    } else {
        writePinHigh(GP25);
    }
    return true;
}
