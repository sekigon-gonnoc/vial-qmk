// Copyright 2023 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

// TMK headers
#include "host_driver.h"
#include "host.h"

// BMP headers
#include "apidef.h"
#include "bmp_vial.h"
#include "state_controller.h"

uint8_t     keyboard_idle __attribute__((aligned(2)))      = 0;
uint8_t     keyboard_protocol __attribute__((aligned(2)))  = 1;
uint16_t    keyboard_led_stats __attribute__((aligned(2))) = 0;
static bool usb_enabled                                    = true;
static bool ble_enabled                                    = true;
static bool has_ble                                        = true;
static bool has_usb                                        = true;

uint8_t keyboard_leds(void) {
    return 0;
}

bool get_ble_enabled(void) {
    return ble_enabled & has_ble;
}

void set_ble_enabled(bool enabled) {
    ble_enabled = enabled;
}

bool get_usb_enabled(void) {
    return usb_enabled & has_usb;
}

void set_usb_enabled(bool enabled) {
    usb_enabled = enabled;
}

void select_ble(void) {
    if (usb_enabled) {
        report_keyboard_t report_keyboard = {0};
        report_mouse_t    rep_mouse       = {0};
        host_keyboard_send(&report_keyboard);
        host_mouse_send(&rep_mouse);
        default_layer_or(get_bmp_default_layer_cache());
    }
    ble_enabled = true;
    usb_enabled = false;
}

void select_usb(void) {
    if (ble_enabled) {
        report_keyboard_t report_keyboard = {0};
        report_mouse_t    rep_mouse       = {0};
        host_keyboard_send(&report_keyboard);
        host_mouse_send(&rep_mouse);
        default_layer_and(~get_bmp_default_layer_cache());
    }
    ble_enabled = false;
    usb_enabled = true;
}

void send_keyboard(report_keyboard_t *report) {
    if (get_ble_enabled()) {
        BMPAPI->ble.send_key((bmp_api_key_report_t *)report);
    }
    if (get_usb_enabled()) {
        BMPAPI->usb.send_key((bmp_api_key_report_t *)report);
    }
}
_Static_assert(sizeof(report_keyboard_t) == sizeof(bmp_api_key_report_t), "Invalid report definition. Check SHARED_EP options");

void send_mouse(report_mouse_t *report) {
    if (get_ble_enabled()) {
        static bool is_zeros_send = false;
        if (report->buttons == 0 && report->x == 0 && report->y == 0 && report->v == 0 && report->h == 0) {
            if (is_zeros_send) {
                // skip no move packet if it has been already send
                return;
            } else {
                is_zeros_send = true;
            }
        } else {
            is_zeros_send = false;
        }

        BMPAPI->ble.send_mouse((bmp_api_mouse_report_t *)report);
    }

    if (get_usb_enabled()) {
        BMPAPI->usb.send_mouse((bmp_api_mouse_report_t *)report);
    }
}

_Static_assert(sizeof(report_mouse_t) == sizeof(bmp_api_mouse_report_t), "Invalid report definition. Check MOUSE_SHARED_EP options");

static void send_system(uint16_t data) {
    if (get_ble_enabled()) {
        BMPAPI->ble.send_system(data);
    }
    if (get_usb_enabled()) {
        BMPAPI->usb.send_system(data);
    }
}

static void send_consumer(uint16_t data) {
    if (get_ble_enabled()) {
        BMPAPI->ble.send_consumer(data);
    }
    if (get_usb_enabled()) {
        BMPAPI->usb.send_consumer(data);
    }
}

void send_extra(report_extra_t *report) {
    if (report->report_id == REPORT_ID_SYSTEM) {
        send_system(report->usage);
    } else if (report->report_id == REPORT_ID_CONSUMER) {
        send_consumer(report->usage);
    }
}

void serial_putc(void *p, char c) {
    BMPAPI->usb.serial_putc(c);
}

static uint8_t raw_hid_send_protocol = 0;
void           raw_hid_send(uint8_t *data, uint8_t length) {
    if (raw_hid_send_protocol == 0) {
        BMPAPI->usb.raw_send(data, length);
    } else {
        BMPAPI->ble.raw_send(data, length);
    }
}

void bmp_raw_hid_receive_usb(const uint8_t *data, uint8_t len) {
    raw_hid_send_protocol = 0;
    bmp_raw_hid_receive_common(data, len);
}

void bmp_raw_hid_receive_ble(const uint8_t *data, uint8_t len) {
    raw_hid_send_protocol = 1;
    bmp_raw_hid_receive_common(data, len);
}
