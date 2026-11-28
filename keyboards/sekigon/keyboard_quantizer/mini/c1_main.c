// Copyright 2023 sekigon-gonnoc
// SPDX-License-Identifier: GPL-2.0-or-later

#include "c1.h"
#include "ch.h"
#include "hal.h"
#include "hardware/sync.h"

#if defined(USB_PACKET_CAPTURE_DEBUG) && USB_PACKET_CAPTURE_DEBUG
#include "usb_capture.h"

#ifdef RP_DMA_REQUIRED
#error "Packet capture needs core1 DMA IRQ ownership"
#endif

static void (*capture_dma_handler)(void);

OSAL_IRQ_HANDLER(RP_DMA_IRQ_1_HANDLER) {
    OSAL_IRQ_PROLOGUE();
    capture_dma_handler();
    OSAL_IRQ_EPILOGUE();
}

void usb_capture_irq_init(void (*handler)(void)) {
    capture_dma_handler = handler;
    nvicEnableVector(RP_DMA_IRQ_1_NUMBER, 1);
}
#endif

// 1ms repeat timer for USB frame
extern void pio_usb_host_frame(void);

static virtual_timer_t vt;
static volatile bool   c1_stop_flag;
static volatile bool   c1_restart_flag;
static volatile bool   usb_clock_ready;
static bool            usb_host_started;

static uint32_t next_sof;

void c1_usb_clock_ready(void) {
    __dmb();
    usb_clock_ready = true;
}

static void __no_inline_not_in_flash_func(timer_cb)(virtual_timer_t *_vt, void *_) {
    static bool sync;

    // Adjust frame interval to 1ms
    uint32_t enter_time = st_lld_get_counter();
    if (enter_time >= next_sof) {
        sync = false;
    } else {
        while (st_lld_get_counter() <= next_sof) {
            continue;
        }
    }

    // Start USB frame
    pio_usb_host_frame();

    // Schedule next frame
    if (!sync) {
        sync = true;
        next_sof = enter_time + 1010;
    } else {
        next_sof += 1000;
    }
}

void c1_before_flash_operation(void) {
    c1_stop_flag = true;
    while (c1_stop_flag) {
        continue;
    }
}

void c1_after_flash_operation(void) {
    c1_restart_flag = true;
    while (c1_restart_flag) {
        continue;
    }
}

static void __no_inline_not_in_flash_func(c1_trap_for_flash_operation)(void) {
    if (c1_stop_flag) {
        if (usb_host_started) chVTReset(&vt);
        c1_stop_flag = false;
        while (!c1_restart_flag) {
            continue;
        }
        c1_restart_flag = false;
        if (usb_host_started) chVTSetContinuous(&vt, TIME_MS2I(1), timer_cb, NULL);
    }
}

static void c1_init_usb_host(void) {
    // USB host stack uses PIO and DMA
    hal_lld_peripheral_unreset(RESETS_ALLREG_PIO0);
    hal_lld_peripheral_unreset(RESETS_ALLREG_PIO1);
    hal_lld_peripheral_unreset(RESETS_ALLREG_DMA);

    // Initialize USB host stack
    c1_usbh();
    usb_host_started = true;
}

// Service flash requests during QMK setup, before the USB clock is ready.
static THD_WORKING_AREA(wa_c1_main_task_wrapper, 2048);
static THD_FUNCTION(c1_main_task_wrapper, arg) {
    while (1) {
        c1_trap_for_flash_operation();
        if (!usb_host_started && usb_clock_ready) {
            __dmb();
            c1_init_usb_host();
        }
        if (usb_host_started) {
            c1_main_task();
#if defined(USB_PACKET_CAPTURE_DEBUG) && USB_PACKET_CAPTURE_DEBUG
            usb_capture_task();
#endif
        }
        chThdSleepMicroseconds(125);
    }
}

// Entry point of core1
void c1_main(void) {
    chSysWaitSystemState(ch_sys_running);
    chInstanceObjectInit(&ch1, &ch_core1_cfg);
    chSysUnlock();

    // Start main task
    chThdCreateStatic(wa_c1_main_task_wrapper, sizeof(wa_c1_main_task_wrapper), NORMALPRIO + 1, c1_main_task_wrapper, NULL);
}

// Start 1ms timer
void c1_start_timer(void) {
    chVTObjectInit(&vt);
    chVTSetContinuous(&vt, TIME_MS2I(1), timer_cb, NULL);
    next_sof = st_lld_get_counter() + 1010;
}
