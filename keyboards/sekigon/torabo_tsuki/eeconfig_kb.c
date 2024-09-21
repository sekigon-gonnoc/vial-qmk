// Copyright 2024 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include QMK_KEYBOARD_H
#include "eeconfig_kb.h"
#include "bmp.h"

void eeconfig_init_kb_datablock(void) {
    memset(&eeconfig_kb, 0, sizeof(eeconfig_kb));
    eeconfig_kb.version                        = EECONFIG_KB_VERSION;
    eeconfig_kb.cursor.cpi_200                 = (400 / 200) - 1;
    eeconfig_kb.cursor.fine_layer              = 0;
    eeconfig_kb.cursor.fine_div                = 1;
    eeconfig_kb.cursor.rough_layer             = 0;
    eeconfig_kb.cursor.rough_mul               = 1;
    eeconfig_kb.cursor.rotate                  = 0;
    eeconfig_kb.aml.timeout                    = 100;
    eeconfig_kb.aml.options.enable             = false;
    eeconfig_kb.aml.layer                      = 1;
    eeconfig_kb.aml.debounce                   = 25;
    eeconfig_kb.aml.threshold                  = 10;
    eeconfig_kb.aml.delay                      = 200;
    eeconfig_kb.scroll.divide                  = 1;
    eeconfig_kb.battery.custom.periph_interval = 10;
    eeconfig_kb.battery.custom.periph_sl       = 15;
    eeconfig_kb.pseudo_encoder.divide          = 50;
    eeconfig_update_kb_datablock(&eeconfig_kb);
}

__attribute__((weak)) void via_custom_value_command_user(uint8_t *data, uint8_t length) {}

#define BYTE_ACCESS(x) (*(uint8_t*)&(x))
#define GET_ID(member) (offsetof(eeconfig_kb_t, member))

typedef void (*eeconfig_kb_hook)(int32_t);
typedef struct {
    uint32_t id;
    uint8_t size;
    eeconfig_kb_hook initialize;
} eeconfig_kb_member_t;

static void cpi_200_init(int32_t c) {
    pointing_device_set_cpi(c * 200 + 200);
}
static void aml_option_init(int32_t _) {
    set_auto_mouse_enable(eeconfig_kb.aml.options.enable);
}
static void aml_layer_init(int32_t _) {
    set_auto_mouse_layer(eeconfig_kb.aml.layer);
}
static void aml_timeout_init(int32_t _) {
    set_auto_mouse_timeout(eeconfig_kb.aml.timeout);
}
static void aml_debounce_init(int32_t _) {
    set_auto_mouse_debounce(eeconfig_kb.aml.debounce);
}
uint16_t get_auto_mouse_delay(void) {
    return eeconfig_kb.aml.delay;
}
uint8_t get_auto_mouse_threshold(void) {
    return eeconfig_kb.aml.threshold;
}

void battery_type_init(int32_t _) {
    if (eeconfig_kb.battery.type == 0) {
        BMPAPI->adc.config_vcc_channel(BAT_IN, 1300, 1050);
    } else {
        BMPAPI->adc.config_vcc_channel(BAT_IN, 1500, 1000);
    }
}

static const eeconfig_kb_member_t eeconfig_kb_members[] = {
    [GET_ID(cursor.cpi_200)]                 = {GET_ID(cursor.cpi_200), .size = 1, .initialize = cpi_200_init},    //
    [GET_ID(cursor.fine_layer)]              = {GET_ID(cursor.fine_layer), .size = 1},                             //
    [GET_ID(cursor.fine_div)]                = {GET_ID(cursor.fine_div), .size = 1},                               //
    [GET_ID(cursor.rough_layer)]             = {GET_ID(cursor.rough_layer), .size = 1},                            //
    [GET_ID(cursor.rough_mul)]               = {GET_ID(cursor.rough_mul), .size = 1},                              //
    [GET_ID(cursor.rotate)]                  = {GET_ID(cursor.rotate), .size = 1},                                 //
    [GET_ID(aml.options)]                    = {GET_ID(aml.options), .size = 1, .initialize = aml_option_init},    //
    [GET_ID(aml.layer)]                      = {GET_ID(aml.layer), .size = 1, .initialize = aml_layer_init},       //
    [GET_ID(aml.timeout)]                    = {GET_ID(aml.timeout), .size = 2, .initialize = aml_timeout_init},   //
    [GET_ID(aml.debounce)]                   = {GET_ID(aml.debounce), .size = 1, .initialize = aml_debounce_init}, //
    [GET_ID(aml.threshold)]                  = {GET_ID(aml.threshold), .size = 1},                                 //
    [GET_ID(aml.delay)]                      = {GET_ID(aml.delay), .size = 2},                                     //
    [GET_ID(scroll.layer)]                   = {GET_ID(scroll.layer), .size = 1},                                  //
    [GET_ID(scroll.options)]                 = {GET_ID(scroll.options), .size = 1},                                //
    [GET_ID(scroll.divide)]                  = {GET_ID(scroll.divide), .size = 1},                                 //
    [GET_ID(battery.type)]                   = {GET_ID(battery.type), .size = 1},                                  //
    [GET_ID(battery.mode)]                   = {GET_ID(battery.mode), .size = 1},                                  //
    [GET_ID(battery.custom.periph_interval)] = {GET_ID(battery.custom.periph_interval), .size = 1},                //
    [GET_ID(battery.custom.periph_sl)]       = {GET_ID(battery.custom.periph_sl), .size = 1},                      //
    [GET_ID(pseudo_encoder.layer)]           = {GET_ID(pseudo_encoder.layer), .size = 1},                          //
    [GET_ID(pseudo_encoder.snap_layer)]      = {GET_ID(pseudo_encoder.snap_layer), .size = 1},                     //
    [GET_ID(pseudo_encoder.divide)]          = {GET_ID(pseudo_encoder.divide), .size = 1},                         //
};

static void process_custom_value_command(uint8_t *data, uint8_t length) {
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);

    if (*command_id == id_eeprom_reset) {
        eeconfig_init_kb_datablock();
        return;
    }

    if (*channel_id != id_custom_channel) {
        return;
    }

    if (value_id_and_data[0] >= sizeof(eeconfig_kb_members) / sizeof(eeconfig_kb_members[0])) {
        return;
    }

    const eeconfig_kb_member_t *member = &eeconfig_kb_members[value_id_and_data[0]];

    if (member->size == 0) {
        return;
    }

    if (*command_id == id_custom_set_value) {
        int32_t data = ((value_id_and_data[1]) | ((int32_t)value_id_and_data[2] << 8) | ((int32_t)value_id_and_data[3] << 16) | ((int32_t)value_id_and_data[4] << 24));
        memcpy(eeconfig_kb.bytes + member->id, &data, member->size);
        if (member->initialize) member->initialize(data);
    } else if (*command_id == id_custom_get_value) {
        int32_t data;
        memcpy(&data, eeconfig_kb.bytes + member->id, member->size);
        memcpy(&value_id_and_data[1], &data, member->size);
    } else if (*command_id == id_custom_save) {
        for (int i = 0; i < member->size; i++) {
            eeprom_update_byte(EECONFIG_KB_DATABLOCK + member->id + i, eeconfig_kb.bytes[member->id + i]);
        }
    }
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    // data = [ command_id, channel_id, value_id, value_data ]
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);

    if (*channel_id == id_custom_channel) {
        printf("command:%d channel:%d value_id:%d value:%d\n", *command_id, *channel_id, value_id_and_data[0], value_id_and_data[1]);
    } else {
        via_custom_value_command_user(data, length);
        return;
    }
    
    process_custom_value_command(data, length);

    via_custom_value_command_user(data, length);
}
