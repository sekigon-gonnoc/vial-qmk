// Copyright 2023 sekigon-gonnoc
/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <stdint.h>

typedef enum {
    BMP_FILE_NONE,
    BMP_FILE_EEPROM,
    BMP_FILE_CONFIG,
} bmp_file_t;

typedef enum {
    BMP_FILE_CONTINUE,
    BMP_FILE_COMPLETE,
    BMP_FILE_ERROR,
} bmp_file_res_t;

bmp_file_t     detect_file_type(const uint8_t *data, uint16_t len);
bmp_file_res_t write_bmp_file(bmp_file_t file_type, const uint8_t *data, uint32_t offset, uint32_t len);
void           flush_bmp_file(void);