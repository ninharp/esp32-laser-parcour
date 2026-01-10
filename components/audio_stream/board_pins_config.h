// Stub board_pins_config.h for custom ESP32-C3 board without ESP-ADF audio board
// This file is required by ESP-ADF audio_stream components that expect board headers
// Since we're using direct I2S without an audio board, provide stub types and functions

#ifndef _BOARD_PINS_CONFIG_H_
#define _BOARD_PINS_CONFIG_H_

#include "driver/i2s_std.h"

// Stub board_i2s_pin_t structure - not used since we configure pins in sound_manager.c
typedef struct {
    int bck_io_num;
    int ws_io_num;
    int data_out_num;
    int data_in_num;
    int mck_io_num;
} board_i2s_pin_t;

// Stub function - does nothing since we configure I2S directly
static inline void get_i2s_pins(i2s_port_t port, board_i2s_pin_t *i2s_config)
{
    (void)port;
    (void)i2s_config;
    // No-op: I2S pins are configured directly in sound_manager.c via i2s_cfg.std_cfg.gpio_cfg
}

#endif // _BOARD_PINS_CONFIG_H_
