#include QMK_KEYBOARD_H

/* 
 * Keyboard Quantizer Mini 
 * CapsLock LED Indicator Custom Firmware
 */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_TRNS
    )
};

// CapsLockの状態を検知して本体のLED(GP25ピン)を制御する関数
bool led_update_user(led_t led_state) {
    if (led_state.caps_lock) {
        writePinLow(GP25); // 点灯
    } else {
        writePinHigh(GP25); // 消灯
    }
    return true;
}
