
#pragma once

enum {
    KEY_OS_OVERRIDE_DISABLE,
    US_KEY_JP_OS_OVERRIDE_DISABLE,
    JP_KEY_US_OS_OVERRIDE_DISABLE,
};

typedef union {
    uint32_t raw;
    struct {
        uint8_t key_os_override : 2;
    };
} user_config_t;
extern user_config_t user_config;