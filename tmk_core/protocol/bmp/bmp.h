
#pragma once
#include <stdint.h>
#include "apidef.h"
#include "action.h"
#include "quantum_keycodes.h"

typedef struct {
    uint32_t encoder_eeprom_addr;
    uint32_t encoder_size;
    uint32_t vial_bmp_dynamic_keymap_magic_addr;
    uint32_t vial_qmk_setting_eeprom_addr;
    uint32_t vial_tap_dance_eeprom_addr;
    uint32_t vial_combo_eeprom_addr;
    uint32_t vial_key_override_eeprom_addr;
    uint32_t vial_macro_eeprom_addr;
    uint32_t vial_macro_eeprom_size;
    uint32_t bmp_settings_addr;
    uint8_t  matrix_rows;
    uint8_t  matrix_cols;
    uint8_t  layer;
} dynamic_keymap_config_t;
#define VIAL_BMP_DYNAMIC_KEYMAP_MAGIC 0xB9FE

bool bmp_config_overwrite(bmp_api_config_t const *const config_on_storage,
                          bmp_api_config_t *const       keyboard_config);
void bmp_mode_transition_check(void);
void bmp_keyboard_task(void);
void bmp_init(void);
int  bmp_dynamic_keymap_init(void);
int  bmp_dynamic_keymap_calc_offset(const bmp_api_config_t *config, dynamic_keymap_config_t *keymap_config);

extern const dynamic_keymap_config_t *p_dynamic_keymap_config;
extern const bmp_api_config_t default_config;
extern const bmp_api_config_t *bmp_config;

const char *bmp_get_version_info(void);
bool        is_safe_mode(void);

void bmp_via_receive_cb(uint8_t *data, uint8_t length,
                        int (*raw_hid_send)(const uint8_t *data,
                                            uint8_t        length));

bool is_safe_mode(void);
void bmp_post_keyboard_task(void);
bool process_record_bmp(uint16_t keycode, keyrecord_t *record);
void protocol_post_task_bmp(void);

extern int sleep_enter_counter;
extern int reset_counter;
void bmp_enter_sleep(void);
void bmp_before_sleep(void);

extern const uint8_t MAINTASK_INTERVAL;