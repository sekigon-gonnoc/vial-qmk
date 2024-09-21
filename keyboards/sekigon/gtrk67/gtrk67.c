// Copyright 2024 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "gtrk67.h"

#include <string.h>
#include "debug.h"

#include QMK_KEYBOARD_H
#include "matrix.h"
#include "bmp.h"
#include "state_controller.h"
#include "bmp_matrix.h"
#include "spi.h"
#include "bmp_settings.h"

#include "report.h"
#include "pointing_device.h"

#include "trackball.h"

#define IO_RESET 5
#define TB_POW 15
#define SR_POW 1
#define SR_DATA 14
#define SR_CLK 11
#define CS_PIN_TB0 CONFIG_SS_PIN

#define SPI_MOSI CONFIG_MOSI_PIN
#define SPI_MISO CONFIG_MISO_PIN
#define SPI_SCK CONFIG_SCK_PIN

static const bmp_api_gpio_mode_t bmp_gpio_in_np = {
    .dir  = BMP_MODE_INPUT,
    .pull = BMP_PULL_NONE,
};
#define setPinInputNopull(pin) BMPAPI->gpio.set_mode(pin, &bmp_gpio_in_np)

void keyboard_post_init_kb(void) { debug_enable = false; }

void matrix_init_kb() {

    gpio_write_pin_high(CS_PIN_TB0);
    gpio_write_pin_low(SPI_SCK);
    gpio_write_pin_low(SPI_MOSI);
    gpio_set_pin_output_push_pull(CS_PIN_TB0);
    gpio_set_pin_output_push_pull(SPI_SCK);
    gpio_set_pin_output_push_pull(SPI_MOSI);

    // turn off trackball
    gpio_set_pin_output_push_pull(TB_POW);
    gpio_write_pin_high(TB_POW);

    // reset io expanders
    gpio_set_pin_output_push_pull(IO_RESET);
    gpio_write_pin_low(IO_RESET);

    gpio_write_pin_high(SR_DATA);
    gpio_write_pin_low(SR_CLK);
    gpio_set_pin_output_push_pull(SR_DATA);
    gpio_set_pin_output_push_pull(SR_CLK);

    gpio_write_pin_high(IO_RESET);

    // turn on trackball
    // gpio_write_pin_low(TB_POW);

    gpio_write_pin_high(SR_POW);
    gpio_set_pin_output_push_pull(SR_POW);

    setPinInputNopull(SPI_MISO);

    gpio_write_pin_high(SR_DATA);
    for (int idx = 0; idx < 16; idx++) {
        gpio_write_pin_high(SR_CLK);
        gpio_write_pin_low(SR_CLK);
    }

    matrix_init_user();
}

static const uint16_t   default_tb_cpi = 540;
static uint16_t         tb_cpi         = default_tb_cpi;
static trackball_info_t tb_info;

const trackball_info_t *get_trackball_info() { return &tb_info; }


static inline void check_tb_connection(void) {
}

static int initialize_step = 0;
static uint8_t init_val1= 0x80;
static uint8_t init_val2= 0x80;

void matrix_scan_kb() {
    const int wait0 = 5;
    const int wait1 = 120;
    const int wait2 = 250;
    if (initialize_step < 2) {
        gpio_write_pin_high(TB_POW);
        initialize_step++;
    } else if (initialize_step < wait0) {
        gpio_write_pin_low(TB_POW);
        initialize_step++;
    } else if (initialize_step == wait0) {
        spim_init();
        setPinInputNopull(SPI_MISO);
        initialize_step++;
    } else if (initialize_step < wait1) {
        initialize_step++;
    } else if (initialize_step == wait1) {
        trackball_init(CS_PIN_TB0, init_val1, init_val2);
        initialize_step++;
    } else if (initialize_step < wait2) {
        initialize_step++;
    } else if (initialize_step == wait2) {
        trackball_enable(CS_PIN_TB0);
        initialize_step++;
    } else if (initialize_step > wait2) {
        static int       cnt  = 0;
        static uint32_t  last_motion_time = 0;
        static uint32_t  last_read_time = 0;
        static bool is_sleep = false;

        if (is_sleep) {
            if (timer_elapsed32(last_read_time) >= 80) {
                uint8_t status = trackball_get_status(CS_PIN_TB0);
                if ((status & 0x08) == 0) {
                    is_sleep = false;
                    trackball_wakeup(CS_PIN_TB0);
                    last_motion_time = timer_read32();
                    dprintf("wakeup trackball\n");
                    BMPAPI->app.schedule_next_task(MATRIX_SCAN_TIME_MS);
                }else{
                    BMPAPI->app.schedule_next_task(80);
                }
                last_read_time = timer_read32();
            }
        } else if (timer_elapsed32(last_motion_time) > 10000) {
            trackball_sleep(CS_PIN_TB0);
            is_sleep = true;
            cnt = 0;
            dprintf("sleep trackball\n");
            BMPAPI->app.schedule_next_task(80);
        } else {
            trackball_data_t data0 = trackball_get(CS_PIN_TB0);
            last_read_time = timer_read32();
            // swap x-y axis
            tb_info.x = data0.y;
            tb_info.y = data0.x;

            if (via_get_layout_options() == 1) {
                tb_info.x = -tb_info.y;
                tb_info.y = -tb_info.x;
            }

            tb_info.motion_flag = ((data0.stat & 0x08) == 0) ? 0x80 : 0;
            if (tb_info.motion_flag & 0x80) {
                last_motion_time = timer_read32();

                static int32_t x_surplus, y_surplus;
                int32_t scaled_x = (int32_t)(tb_info.x + x_surplus) * tb_cpi / default_tb_cpi;
                int32_t scaled_y = (int32_t)(tb_info.y + y_surplus) * tb_cpi / default_tb_cpi;

                x_surplus = (int32_t)(tb_info.x + x_surplus) - scaled_x * default_tb_cpi / tb_cpi;
                y_surplus = (int32_t)(tb_info.y + y_surplus) - scaled_y * default_tb_cpi / tb_cpi;

                tb_info.x = scaled_x;
                tb_info.y = scaled_y;
            }

            if (cnt++ % 10 == 0) {
                dprintf("%d %d %02x %d %d\n", tb_info.x, tb_info.y, data0.stat, data0.x, data0.y);
            }

            BMPAPI->app.schedule_next_task(MATRIX_SCAN_TIME_MS);
        }
    }

    if (initialize_step < wait2) {
        bmp_set_enable_task_interval_stretch(false);
    }

    matrix_scan_user();
}

void bmp_before_sleep(void) {
    // turn off trackball
    gpio_write_pin_high(TB_POW);
    // clear all cols
    gpio_write_pin_low(IO_RESET);
    gpio_write_pin_high(IO_RESET);
}

bool checkSafemodeFlag(bmp_api_config_t const *const config) { return false; }

bool bmp_config_overwrite(bmp_api_config_t const *const config_on_storage,
                          bmp_api_config_t *const       keyboard_config) {
    // User can overwrite partial settings
    bmp_api_config_t               new_config = default_config;
    const bmp_api_ble_conn_param_t conn       = {.max_interval = 20, .min_interval = 20, .slave_latency = 25};
    new_config.startup                        = config_on_storage->startup;
    new_config.matrix.debounce                = config_on_storage->matrix.debounce;
    new_config.param_peripheral               = conn;
    new_config.reserved[2]                    = config_on_storage->reserved[2];
    *keyboard_config                          = new_config;
    return true;
}

#include "microshell/util/mscmd.h"

MSCMD_USER_RESULT usrcmd_trackball_init1(MSOPT *           msopt,
                                         MSCMD_USER_OBJECT usrobj) {
    char arg[16];
    init_val1= 0x80;
    init_val2= 0x80;

    if (msopt->argc >= 3) {
        msopt_get_argv(msopt, 1, arg, sizeof(arg));
        init_val1 = (uint8_t)strtol(arg, NULL, 16);
        msopt_get_argv(msopt, 2, arg, sizeof(arg));
        init_val2 = (uint8_t)strtol(arg, NULL, 16);
    }
    initialize_step = 0;

    return 0;
}

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

void pointing_device_driver_init(void) {}

report_mouse_t pointing_device_driver_get_report(report_mouse_t mouse_report) {
    if (tb_info.motion_flag & 0x80) {
        mouse_report.x = tb_info.x;
        mouse_report.y = tb_info.y;
    }

    return mouse_report;
}

uint16_t pointing_device_driver_get_cpi(void) {
    return tb_cpi;
}

void pointing_device_driver_set_cpi(uint16_t cpi) {
    tb_cpi = cpi;
}
