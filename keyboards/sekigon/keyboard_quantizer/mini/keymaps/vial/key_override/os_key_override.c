// Copyright 2023 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "os_key_override.h"

#include "vial.h"

#define OS_KEY_OVERRIDE_ENTRIES 25

extern const key_override_t **key_overrides;
static key_override_t         os_override[OS_KEY_OVERRIDE_ENTRIES]                                   = {0};
static uint8_t                os_override_cnt                                                        = 0;
static const key_override_t  *override_ptrs[VIAL_KEY_OVERRIDE_ENTRIES + OS_KEY_OVERRIDE_ENTRIES + 1] = {0};
void                          os_key_override_init(void) {
    // copy vial override
    for (size_t i = 0; i < VIAL_KEY_OVERRIDE_ENTRIES; ++i) {
        override_ptrs[i] = key_overrides[i];
    }

    // regisiter os override
    for (size_t i = 0; i < OS_KEY_OVERRIDE_ENTRIES; ++i) {
        override_ptrs[i + VIAL_KEY_OVERRIDE_ENTRIES] = &os_override[i];
    }

    // swap reference
    key_overrides = override_ptrs;
}

int register_os_key_override(const key_override_t *override) {
    if (os_override_cnt >= OS_KEY_OVERRIDE_ENTRIES) {
        return -1;
    }

    override_ptrs[VIAL_KEY_OVERRIDE_ENTRIES + os_override_cnt++] = override;
    return 0;
}

void remove_all_os_key_overrides(void) {
    for (int i = 0; i < OS_KEY_OVERRIDE_ENTRIES; i++) {
        override_ptrs[VIAL_KEY_OVERRIDE_ENTRIES + i] = NULL;
    }
    os_override_cnt = 0;
}
