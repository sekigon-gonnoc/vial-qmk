// Copyright 2023 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

// BMP
#include "state_controller.h"
#include "bmp_host_driver.h"
#include "bmp_indicator_led.h"
#include "bmp.h"
#include "bmp_matrix.h"
#include "bmp_file.h"

int sleep_enter_counter     = -1;
int reset_counter           = -1;
int bootloader_jump_counter = -1;

static bool is_usb_connected_ = false;
static bool is_usb_powered_   = false;
static bool is_ble_connected_ = false;
static bool is_event_driven_applicable_ = false;

static uint32_t auto_sleep_timeout_ms = 0;

bool is_usb_connected(void) {
    return is_usb_connected_;
}
bool is_usb_powered(void) {
    return is_usb_powered_;
}
bool is_ble_connected(void) {
    return is_ble_connected_;
}

void set_auto_sleep_timeout(uint32_t ms) {
    auto_sleep_timeout_ms = ms;
}

uint32_t get_auto_sleep_timeout(void) {
    return auto_sleep_timeout_ms;
}

__attribute__((weak)) void bmp_before_sleep(void) {}
__attribute__((weak)) void bmp_enter_sleep(void) {
    bmp_before_sleep();
    BMPAPI->app.enter_sleep_mode();
}
__attribute__((weak)) void bmp_before_shutdown(void) {
    bmp_before_sleep();
}

__attribute__((weak)) void bmp_state_change_cb_user(bmp_api_event_t event) {}
__attribute__((weak)) void bmp_state_change_cb_kb(bmp_api_event_t event) {
    bmp_state_change_cb_user(event);
}

bmp_error_t bmp_state_change_cb(bmp_api_event_t event) {
    switch (event) {
        case USB_CONNECTED:
            is_usb_powered_ = true;
            break;

        case USB_HID_READY:
            is_usb_powered_   = true;
            is_usb_connected_ = true;
            if (!is_ble_connected()) {
                select_usb();
            }
            // update_config_files();
            break;

        case USB_DISCONNECTED:
            is_usb_powered_   = false;
            is_usb_connected_ = false;
            if (is_ble_connected()) {
                select_ble();
            }
            break;

        case BLE_ADVERTISING_START:
            bmp_indicator_set(INDICATOR_ADVERTISING, 0);
            bmp_set_enable_task_interval_stretch(false);
            bmp_schedule_next_task();
            break;

        case BLE_ADVERTISING_STOP:
            bmp_indicator_set(INDICATOR_TURN_OFF, 0);
            bmp_set_enable_task_interval_stretch(false);
            bmp_schedule_next_task();

            if (!is_usb_powered()) {
                sleep_enter_counter = 1;
            }
            break;

        case BLE_CONNECTED:
            is_ble_connected_ = true;
            if (!is_usb_connected()) {
                select_ble();
            }
            bmp_indicator_set(INDICATOR_CONNECTED, 0);
            bmp_set_enable_task_interval_stretch(false);
            bmp_schedule_next_task();
            break;

        case BLE_DISCONNECTED:
            is_ble_connected_ = false;
            // Disable below code because BLE could be disconnected unintentionally
            // if (is_usb_connected()) {
            //     select_usb();
            // }
            break;

        case BMP_LOW_BATTERY:
            bmp_before_shutdown();
            break;

        default:
            break;
    }

    bmp_state_change_cb_kb(event);

    return BMP_OK;
}

static bool is_event_driven_applicable(void) {
    if (bmp_config->matrix.diode_direction == MATRIX_COL2ROW || bmp_config->matrix.diode_direction == MATRIX_74HC164COL) {
        for (int i = 0; i < bmp_config->matrix.device_rows; i++) {
            if (bmp_config->matrix.row_pins[i] == 5 || bmp_config->matrix.row_pins[i] == 6) {
                return false;
            }
        }
        return true;
    } else if (bmp_config->matrix.diode_direction == MATRIX_COL2ROW || bmp_config->matrix.diode_direction == MATRIX_74HC164ROW) {
        for (int i = 0; i < bmp_config->matrix.device_cols; i++) {
            if (bmp_config->matrix.col_pins[i] == 5 || bmp_config->matrix.col_pins[i] == 6) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
    return false;
}

static bool is_any_key_pressed(void) {
    uint8_t matrix_offset      = bmp_config->matrix.is_left_hand ? 0 : bmp_config->matrix.rows - bmp_matrix_get_device_row();
    bool    _is_any_key_pressed = false;
    for (int i = 0; i < bmp_config->matrix.device_rows; i++) {
        if (matrix_get_row(i + matrix_offset) != 0) {
            _is_any_key_pressed |= true;
        }
    }

    return _is_any_key_pressed;
}

void bmp_state_controller_init(void) {
    is_event_driven_applicable_ = is_event_driven_applicable();
}

void bmp_mode_transition_check(void) {
    // auto sleep check
    if ((auto_sleep_timeout_ms > 0) && !is_usb_connected()) {
        if (last_matrix_activity_elapsed() > auto_sleep_timeout_ms     //
            && last_encoder_activity_elapsed() > auto_sleep_timeout_ms //
            && last_pointing_device_activity_elapsed() > auto_sleep_timeout_ms) {
            sleep_enter_counter = 1;
        }
    }

    // sleep flag check
    if (sleep_enter_counter > 0) {
        if (!is_any_key_pressed()) {
            bmp_enter_sleep();
            sleep_enter_counter = 0;
        }
    }

    // reset flag check
    if (reset_counter > 0) {
        reset_counter--;
        if (reset_counter == 0) {
            // reset without safemode flag
            BMPAPI->app.reset(0);
        }
    }

    // bootloader jump flag check
    if (bootloader_jump_counter > 0) {
        bootloader_jump_counter--;
        if (bootloader_jump_counter == 0) {
            BMPAPI->bootloader_jump();
        }
    }

    flush_bmp_file();
}

static bool task_interval_stretch;
void        bmp_set_enable_task_interval_stretch(bool enable) {
    task_interval_stretch = enable;
}

bool bmp_get_enable_task_interval_stretch(void) {
    return task_interval_stretch;
}

static void schedule_next_task_internal(uint32_t interval_ms) {
    if (!BMPAPI->app.has_schedule_in_range(0, interval_ms << 1)) {
        BMPAPI->app.schedule_next_task(interval_ms);
    }
}

void bmp_schedule_next_task(void) {
    static uint32_t last_key_press_time;

    if (is_any_key_pressed()) {
        last_key_press_time = timer_read32();
        schedule_next_task_internal(MAINTASK_INTERVAL);
    } else if (timer_elapsed32(last_key_press_time) < MAINTASK_INTERVAL //
               || !task_interval_stretch) {
        schedule_next_task_internal(MAINTASK_INTERVAL);
    } else {
        if (is_event_driven_applicable_ && bmp_config->matrix.diode_direction == MATRIX_COL2ROW){
            for (int i = 0; i < bmp_config->matrix.device_rows; i++) {
                gpio_write_pin_low(bmp_config->matrix.row_pins[i]);
            }
            BMPAPI->app.schedule_next_task(BMP_SCHEDULE_WAIT_NEXT_EVENT);
        } else if (is_event_driven_applicable_ && bmp_config->matrix.diode_direction == MATRIX_74HC164COL) {
            gpio_write_pin_low(bmp_config->matrix.col_pins[2]);
            gpio_write_pin_high(bmp_config->matrix.col_pins[2]);
            BMPAPI->app.schedule_next_task(BMP_SCHEDULE_WAIT_NEXT_EVENT);
        } else if (is_event_driven_applicable_ && bmp_config->matrix.diode_direction == MATRIX_ROW2COL) {
            for (int i = 0; i < bmp_config->matrix.device_cols; i++) {
                gpio_write_pin_low(bmp_config->matrix.col_pins[i]);
            }
            BMPAPI->app.schedule_next_task(BMP_SCHEDULE_WAIT_NEXT_EVENT);
        } else if (is_event_driven_applicable_ && bmp_config->matrix.diode_direction == MATRIX_74HC164ROW) {
            gpio_write_pin_low(bmp_config->matrix.row_pins[2]);
            gpio_write_pin_high(bmp_config->matrix.row_pins[2]);
            BMPAPI->app.schedule_next_task(BMP_SCHEDULE_WAIT_NEXT_EVENT);
        } else {
            if (timer_elapsed32(last_key_press_time) > 200) {
                schedule_next_task_internal(MAINTASK_INTERVAL * 3);
            } else {
                schedule_next_task_internal(MAINTASK_INTERVAL);
            }
        }
    }
}