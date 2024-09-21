
#pragma once

#include <stdint.h>

#define BMP_SETTINGS_SIZE 64
#define BMP_SETTINGS_VERSION 1

typedef enum {
    BMP_KEY_OS_OVERRIDE_DISABLE,
    BMP_US_KEY_JP_OS_OVERRIDE,
    BMP_JP_KEY_US_OS_OVERRIDE,
} key_os_override_t;

typedef union {
    struct {
        uint8_t version;
        key_os_override_t key_os_override;
        struct {
            uint8_t enabled;
            int8_t  layer[8];
        } connection_setting;
    };

    uint8_t data[BMP_SETTINGS_SIZE];
} bmp_settings_t;

_Static_assert(sizeof(bmp_settings_t) == BMP_SETTINGS_SIZE, "Invalid bmp_settings_t");

void              bmp_settings_init(void);
void              bmp_set_key_os_override(key_os_override_t override);
key_os_override_t bmp_get_key_os_override(void);
void              bmp_settings_reset(void);
void              bmp_settings_read(bmp_settings_t *p_setting);
void              bmp_settings_write(bmp_settings_t const *p_setting);
void              bmp_settings_save(void);
int8_t            bmp_settings_get_connection_setting_layer(uint8_t id);
void              bmp_settings_set_connection_setting_layer(uint8_t id, uint8_t layer);
void              bmp_settings_set_connection_setting_enabled(uint8_t enabled);
void              via_custom_value_command_bmp(uint8_t *data, uint8_t length);