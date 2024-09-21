// Copyright 2023 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

// QMK headers
#include "util.h"
#include "print.h"
#include "matrix.h"
#include "raw_hid.h"
#include "keyboard.h"
#include "rgblight.h"
#include "eeprom_driver.h"

// TMK headers
#include "host_driver.h"
#include "host.h"

// BMP headers
#include "apidef.h"
#include "cli.h"
#include "bmp.h"
#include "state_controller.h"
#include "bmp_vial.h"
#include "bmp_indicator_led.h"
#include "bmp_flash.h"
#include "bmp_file.h"
#include "eeprom_bmp.h"
#include "bmp_key_override.h"
#include "bmp_settings.h"
#include "bmp_host_driver.h"

#ifndef DISABLE_MSC
#    define DISABLE_MSC 0
#endif

#ifndef BMP_FORCE_SAFE_MODE
#    define BMP_FORCE_SAFE_MODE false
#endif

#ifndef BMP_DIODE_DIRECTION
#    define BMP_DIODE_DIRECTION DIODE_DIRECTION
#endif

#ifndef BMP_STARTUP_ADVERTISE
#    define BMP_STARTUP_ADVERTISE 0
#endif

#ifndef BMP_DEBOUNCE
#    define BMP_DEBOUNCE 1
#endif

#ifndef MATRIX_SCAN_TIME_MS
#    define MATRIX_SCAN_TIME_MS 17
#endif
const uint8_t MAINTASK_INTERVAL       = MATRIX_SCAN_TIME_MS;

/* -------------------------
 *   TMK host driver defs
 * -------------------------
 */

/* declarations */
uint8_t keyboard_leds(void);
void    send_keyboard(report_keyboard_t *report);
void    send_nkro(report_nkro_t *report) {}
void    send_mouse(report_mouse_t *report);
void    send_extra(report_extra_t *report);

/* host struct */
host_driver_t driver = {keyboard_leds, send_keyboard, send_nkro, send_mouse, send_extra};
const bmp_api_config_t *bmp_config;

const bmp_api_config_t default_config = {.version     = CONFIG_VERSION,
                                         .mode        = BMP_DEFAULT_MODE == 0   ? SINGLE
                                                        : BMP_DEFAULT_MODE == 1 ? SPLIT_MASTER
                                                        : BMP_DEFAULT_MODE == 2 ? SPLIT_SLAVE
                                                                                : SINGLE,
                                         .device_info = {PRODUCT_ID, VENDOR_ID, PRODUCT, STR(MANUFACTURER)},
                                         .matrix =
                                             {
                                                 .rows            = MATRIX_ROWS_DEFAULT,
                                                 .cols            = MATRIX_COLS_DEFAULT,
                                                 .device_rows     = THIS_DEVICE_ROWS,
                                                 .device_cols     = THIS_DEVICE_COLS,
                                                 .layer           = 8,
                                                 .debounce        = BMP_DEBOUNCE,
                                                 .diode_direction = BMP_DIODE_DIRECTION,
                                                 .row_pins        = MATRIX_ROW_PINS,
                                                 .col_pins        = MATRIX_COL_PINS,
                                                 .is_left_hand    = true,
                                             },
                                         .encoder = {
                                            .pin_a = ENCODERS_PAD_A,
                                            .pin_b = ENCODERS_PAD_B,
                                         },
                                         .param_peripheral = {60, 30, 7},
                                         .param_central    = {60, 30, 7},
                                         .led              = {.pin = WS2812_DI_PIN, .num = 1},
                                         .startup          = BMP_STARTUP_ADVERTISE,
#ifdef CONFIG_RESERVED
                                         .reserved = CONFIG_RESERVED
#endif
};

static bool is_safe_mode_ = false;
bool is_safe_mode(void) {
    return is_safe_mode_;
}

bmp_error_t msc_write_callback(const uint8_t *dat, uint32_t len) {
    static uint32_t   offset    = 0;
    static bmp_file_t file_type = BMP_FILE_NONE;

    if (offset == 0) {
        file_type = detect_file_type(dat, len);
        if (file_type != BMP_FILE_NONE)
        {
            printf("File type:%d\n", file_type);
        }
    }

    bmp_file_res_t res = write_bmp_file(file_type, dat, offset, len);
    offset += len;

    if (res != BMP_FILE_CONTINUE) {
        offset    = 0;
        file_type = BMP_FILE_NONE;
    }

    return BMP_OK;
}

bmp_error_t nus_rcv_callback(const uint8_t *dat, uint32_t len) {

#ifdef RGBLIGHT_SPLIT
    if (len == sizeof(rgblight_syncinfo_t)) {
        rgblight_update_sync((rgblight_syncinfo_t *)dat, false);
    }
#endif

    return BMP_OK;
}

int bmp_validate_config(const bmp_api_config_t *config) {
    if (config->version != CONFIG_VERSION                        //
        || config->matrix.rows > 32 || config->matrix.cols > 32  //
        || config->matrix.device_rows > config->matrix.rows      //
        || config->matrix.device_cols > config->matrix.cols      //
        || config->matrix.layer > 32 || config->matrix.layer < 1 //
        || config->mode >= WEBNUS_CONFIG                         //
        || config->led.num > RGBLIGHT_LED_COUNT                  //

    ) {
        return 1;
    }
    return 0;
}

void bmp_battery_check(void) {
    uint16_t vcc_percent   = BMPAPI->app.get_vcc_percent();
    int32_t  battery_level = 1;

    if (vcc_percent > 70) {
        battery_level = 3;
    } else if (vcc_percent > 30) {
        battery_level = 2;
    }

    bmp_indicator_set(INDICATOR_BATTERY, battery_level);
    bmp_set_enable_task_interval_stretch(false);
    bmp_schedule_next_task();
}

__attribute__((weak)) bool bmp_config_overwrite(bmp_api_config_t const *const config_on_storage, bmp_api_config_t *const keyboard_config) {
    return true;
}

void bmp_init(void) {
    if (BMPAPI->api_version != API_VERSION) {
        BMPAPI->bootloader_jump();
    }

    BMPAPI->logger.init();
    BMPAPI->logger.info("logger init");

    is_safe_mode_ = (BMPAPI->app.init() > 0);

    bmp_vial_data_init();
    bmp_config_overwrite(&flash_vial_data.bmp_config, &flash_vial_data.bmp_config);
    bmp_config = &flash_vial_data.bmp_config;

    if (bmp_validate_config(bmp_config) || BMPAPI->app.set_config(bmp_config) || BMP_FORCE_SAFE_MODE) {
        bmp_config = &default_config;
        BMPAPI->app.set_config(bmp_config);
    }

    BMPAPI->usb.set_msc_write_cb(msc_write_callback);
    BMPAPI->app.set_state_change_cb(bmp_state_change_cb);
    BMPAPI->usb.set_raw_receive_cb(bmp_raw_hid_receive_usb);
    BMPAPI->ble.set_raw_receive_cb(bmp_raw_hid_receive_ble);
    if (bmp_config->mode == SPLIT_SLAVE) {
        BMPAPI->ble.set_nus_rcv_cb(nus_rcv_callback);
    }

    bmp_state_controller_init();

    BMPAPI->usb.init(bmp_config, DISABLE_MSC);
    BMPAPI->ble.init(bmp_config);
    BMPAPI->logger.info("usb init");
    cli_init();

    BMPAPI->usb.enable();
    BMPAPI->logger.info("usb enable");
}

static bool do_keyboard_task = false;
void        bmp_main_task(void *_) {
    do_keyboard_task = true;
    bool is_indicator_active = bmp_indicator_task();
    bmp_set_enable_task_interval_stretch(!is_indicator_active);
}

void protocol_setup(void) {
    bmp_init();
    host_set_driver(&driver);
}

void vial_pre_init(void) {
    bmp_dynamic_keymap_init();
}

// clang-format off
static const char bmp_version_info[] = "API version: " STR(API_VERSION) "\n"
                                       "Config version: " STR(CONFIG_VERSION) "\n"
                                       "Build from: " STR(GIT_DESCRIBE) "\n"
                                       "Build Target: " STR(TARGET);
// clang-format on

const char *bmp_get_version_info(void) {
    return bmp_version_info;
}

void protocol_pre_init(void) {
    BMPAPI->usb.create_file("EEPROM  BIN", eeprom_cache, EEPROM_SIZE);
    BMPAPI->usb.create_file("CONFIG  BIN", BMPAPI->flash.get_base_address() + BMP_USER_FLASH_PAGE_SIZE * FLASH_PAGE_ID_VIAL, BMP_USER_FLASH_PAGE_SIZE);
    BMPAPI->usb.create_file("DEFAULT BIN", BMPAPI->flash.get_base_address() + BMP_USER_FLASH_PAGE_SIZE * FLASH_PAGE_ID_EEPROM_DEFAULT0, BMP_USER_FLASH_PAGE_SIZE * 2);

    const flash_vial_data_t *vial = (flash_vial_data_t *)(BMPAPI->flash.get_base_address() + BMP_USER_FLASH_PAGE_SIZE * FLASH_PAGE_ID_VIAL);
    BMPAPI->usb.create_file("VIALJSONBIN", vial->vial_data, vial->len);
    BMPAPI->usb.create_file("VERSION TXT", (uint8_t *)bmp_version_info, strlen(bmp_version_info));

    set_auto_sleep_timeout(bmp_config->reserved[2] * 10 * 60 * 1000);
}

void protocol_post_init(void) {
    rgblight_set_clipping_range(0, bmp_config->led.num);
    rgblight_set_effect_range(0, bmp_config->led.num);
    bmp_indicator_init(bmp_config->indicator_led);

    eeprom_bmp_flush();
    eeprom_bmp_set_cache_mode(EEPROM_BMP_CACHE_WRITE_THROUGH);

    print_set_sendchar((sendchar_func_t)BMPAPI->usb.serial_putc);

    bmp_key_override_init();
    bmp_settings_init();


    BMPAPI->app.main_task_init(bmp_main_task, MAINTASK_INTERVAL);
    BMPAPI->app.schedule_next_task(MAINTASK_INTERVAL);

    bmp_battery_check();

    if (bmp_config->mode == SPLIT_SLAVE || ((bmp_config->mode == SINGLE || bmp_config->mode == SPLIT_MASTER) //
                                            && bmp_config->startup == 1)) {
        BMPAPI->ble.advertise(255);
    }
}

void protocol_pre_task(void) {
    BMPAPI->app.process_task();
}

void protocol_post_task(void) {
#ifdef RGBLIGHT_SPLIT
    if (bmp_config->mode == SPLIT_MASTER && rgblight_get_change_flags()) {
        static rgblight_syncinfo_t rgblight_sync;
        rgblight_get_syncinfo(&rgblight_sync);
        BMPAPI->ble.nus_send_bytes((uint8_t *)&rgblight_sync, sizeof(rgblight_sync));
        rgblight_clear_change_flags();
    }
#endif

    protocol_post_task_bmp();

    BMPAPI->usb.process();
    cli_exec();
    bmp_mode_transition_check();
}

void protocol_keyboard_task(void) {
    if (do_keyboard_task) {
        do_keyboard_task = false;
        keyboard_task();
        bmp_post_keyboard_task();
        bmp_schedule_next_task();
    }
}

void raw_hid_task(void) {}
void console_task(void) {}

bool is_keyboard_master(void) {
    return BMPAPI->app.get_config()->mode != SPLIT_SLAVE;
}

bool is_keyboard_left(void) {
    return BMPAPI->app.get_config()->matrix.is_left_hand;
}
