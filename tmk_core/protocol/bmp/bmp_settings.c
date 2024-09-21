
#include "eeprom.h"
#include "via.h"

#include "bmp_settings.h"
#include "bmp_key_override.h"

#define BMP_SETTINGS_ADDR (TOTAL_EEPROM_BYTE_COUNT - BMP_SETTINGS_SIZE)
#define KEY_OS_OVERRIDE_ADDR (TOTAL_EEPROM_BYTE_COUNT - BMP_SETTINGS_SIZE + offsetof(bmp_settings_t, key_os_override))
#define BMP_SETTINGS_VERSION_ADDR (TOTAL_EEPROM_BYTE_COUNT - BMP_SETTINGS_SIZE + offsetof(bmp_settings_t, version))

static bmp_settings_t settings = {};
void bmp_settings_init(void) {
    eeprom_read_block(&settings, (void *)BMP_SETTINGS_ADDR, sizeof(bmp_settings_t));
    if (settings.version != BMP_SETTINGS_VERSION) {
        bmp_settings_reset();
    }

    bmp_set_key_os_override(bmp_get_key_os_override());
}

void bmp_settings_reset(void) {
    memset(&settings, 0, sizeof(settings));
    settings.version = BMP_SETTINGS_VERSION;
    bmp_settings_write(&settings);
    bmp_settings_save();
}

void bmp_set_key_os_override(key_os_override_t override) {
    switch (override) {
        case BMP_KEY_OS_OVERRIDE_DISABLE:
            remove_all_bmp_key_overrides();
            settings.key_os_override = BMP_KEY_OS_OVERRIDE_DISABLE;
            eeprom_update_byte((uint8_t *)KEY_OS_OVERRIDE_ADDR, BMP_KEY_OS_OVERRIDE_DISABLE);
            break;
        case BMP_US_KEY_JP_OS_OVERRIDE:
            register_us_key_on_jp_os_overrides();
            settings.key_os_override = BMP_US_KEY_JP_OS_OVERRIDE;
            eeprom_update_byte((uint8_t *)KEY_OS_OVERRIDE_ADDR, BMP_US_KEY_JP_OS_OVERRIDE);
            break;
        case BMP_JP_KEY_US_OS_OVERRIDE:
            register_jp_key_on_us_os_overrides();
            settings.key_os_override = BMP_JP_KEY_US_OS_OVERRIDE;
            eeprom_update_byte((uint8_t *)KEY_OS_OVERRIDE_ADDR, BMP_JP_KEY_US_OS_OVERRIDE);
            break;
    }
}

key_os_override_t bmp_get_key_os_override(void) {
    return settings.key_os_override;
}

void bmp_settings_read(bmp_settings_t *p_setting) {
    *p_setting = settings;
}

void bmp_settings_write(bmp_settings_t const *p_setting) {
    settings = *p_setting;
}

void bmp_settings_save(void) {
    eeprom_write_block(&settings, (void *)BMP_SETTINGS_ADDR, sizeof(bmp_settings_t));
}

int8_t bmp_settings_get_connection_setting_layer(uint8_t id) {
    if (!settings.connection_setting.enabled || id >= sizeof(settings.connection_setting.layer)) {
        return -1;
    }

    return settings.connection_setting.layer[id];
}

void bmp_settings_set_connection_setting_layer(uint8_t id, uint8_t layer) {
    if (id >= sizeof(settings.connection_setting.layer)) {
        return;
    }

    settings.connection_setting.layer[id] = layer;
}

void bmp_settings_set_connection_setting_enabled(uint8_t enabled) {
    settings.connection_setting.enabled = enabled;
}


typedef void (*eeconfig_bmp_hook)(int32_t);
typedef struct {
    uint32_t id;
    uint32_t offset;
    uint8_t size;
    uint8_t array_len;
    eeconfig_bmp_hook initialize;
} eeconfig_bmp_member_t;
#define GET_OFFSET(member) (offsetof(bmp_settings_t, member))

static const eeconfig_bmp_member_t eeconfig_bmp_members[] = {
    //
    {.id = 0xff, .offset = GET_OFFSET(connection_setting.enabled), .size = 1},               //
    {.id = 0xfe, .offset = GET_OFFSET(connection_setting.layer), .size = 1, .array_len = 8}, //
};

void via_custom_value_command_bmp(uint8_t *data, uint8_t length) {
    // data = [ command_id, channel_id, value_id, value_data ]
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);

    if (*command_id == id_eeprom_reset) {
        bmp_settings_reset();
        return;
    }

    if (*channel_id != id_custom_channel) {
        return;
    }

    const eeconfig_bmp_member_t *member = NULL;

    for (int idx = 0; idx < sizeof(eeconfig_bmp_members) / sizeof(eeconfig_bmp_members[0]); idx++) {
        if (eeconfig_bmp_members[idx].id == value_id_and_data[0]) {
            member = &eeconfig_bmp_members[idx];
            break;
        }
    }

    if (member == NULL) {
        return;
    }

    if (member->size == 0) {
        return;
    }

    if (*command_id == id_custom_get_value) {
        if (member->array_len == 0) {
            int32_t data = 0;
            memcpy(&data, settings.data + member->offset, member->size);
            memcpy(&value_id_and_data[1], &data, member->size);
        } else {
            if (value_id_and_data[1] < member->array_len) {
                int32_t data = 0;
                memcpy(&data, settings.data + member->offset + (member->size * value_id_and_data[1]), member->size);
                memcpy(&value_id_and_data[2], &data, member->size);
            }
        }
    } else if (*command_id == id_custom_set_value) {
        int32_t data = 0;
        if (member->array_len == 0) {
            data = ((value_id_and_data[1]) | ((int32_t)value_id_and_data[2] << 8) | ((int32_t)value_id_and_data[3] << 16) | ((int32_t)value_id_and_data[4] << 24));
            memcpy(settings.data + member->offset, &data, member->size);
        } else {
            if (value_id_and_data[1] < member->array_len) {
                data = ((value_id_and_data[2]) | ((int32_t)value_id_and_data[3] << 8) | ((int32_t)value_id_and_data[4] << 16) | ((int32_t)value_id_and_data[5] << 24));
                memcpy(settings.data + member->offset + (member->size * value_id_and_data[1]), &data, member->size);
            }
        }
        if (member->initialize) member->initialize(data);
    } else if (*command_id == id_custom_save) {
        bmp_settings_save();
    }
}