// Copyright 2023 sekigon-gonnoc
// SPDX-License-Identifier: GPL-2.0-or-later

#include "keyboard.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "system_rp2040.h"
#include "bootloader.h"
#include "debug.h"
#include "c1.h"

void keyboard_pre_init_kb(void) {
    set_sys_clock_khz(132000, true);
    const uint32_t sys_hz = clock_get_hz(clk_sys);
    // Keep the peripheral clock sourced from clk_sys as ChibiOS configures it.
    clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, sys_hz, sys_hz);
    SystemCoreClock = sys_hz;
    c1_usb_clock_ready();
    keyboard_pre_init_user();
}

__attribute__((weak)) void virtser_recv(uint8_t c) {
    if (c == 'b') {
        bootloader_jump();
    } else if (c == 'd') {
        debug_enable = !debug_enable;
        uprintf("Debug %s\n", debug_enable ? "enabled" : "disabled");
    }
}

bool backing_store_lock(void) {
    c1_after_flash_operation();
    return true;
}

bool backing_store_unlock(void) {
    c1_before_flash_operation();
    return true;
}
