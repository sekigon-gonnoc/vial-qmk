// Copyright 2023 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "action_layer.h"

#include "apidef.h"

__attribute__((weak)) void bmp_before_sleep(void);
__attribute__((weak)) void bmp_enter_sleep(void);
__attribute__((weak)) void bmp_state_change_cb_user(bmp_api_event_t event);
__attribute__((weak)) void bmp_state_change_cb_kb(bmp_api_event_t event);

bool          is_usb_connected(void);
bool          is_usb_powered(void);
bool          is_ble_connected(void);
void          set_auto_sleep_timeout(uint32_t ms);
uint32_t      get_auto_sleep_timeout(void);
layer_state_t get_bmp_default_layer_cache(void);

bmp_error_t bmp_state_change_cb(bmp_api_event_t event);
void bmp_mode_transition_check(void);

void bmp_state_controller_init(void);
void bmp_schedule_next_task(void);
void bmp_set_enable_task_interval_stretch(bool enable);
bool bmp_get_enable_task_interval_stretch(void);

extern int sleep_enter_counter;

bool get_ble_enabled(void);