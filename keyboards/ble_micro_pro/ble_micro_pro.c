/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "ble_micro_pro.h"

#include "bmp.h"
#include "bmp_settings.h"

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    bool cont = process_record_bmp(keycode, record);

    if (cont) {
        cont = process_record_user(keycode, record);
    }

    return cont;
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    // data = [ command_id, channel_id, value_id, value_data ]
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);

    if (*channel_id == id_custom_channel) {
        printf("command:%d channel:%d value_id:%d value:%d\n", *command_id, *channel_id, value_id_and_data[0], value_id_and_data[1]);
    }

    via_custom_value_command_bmp(data, length);
}

void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &(data[0]);
    if (*command_id == id_set_keyboard_value) {
        // This version does not handle save flag
    } else if (*command_id == id_unhandled) {
        via_custom_value_command_kb(&data[1], length - 1);
    } else {
        *command_id = id_unhandled;
    }
}