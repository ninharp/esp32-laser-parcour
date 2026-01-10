/*
 * Custom Audio Board Header - Minimal Stub
 * This provides the board.h header required by ESP-ADF components
 * without implementing full audio board support (not needed for this project)
 */

#ifndef _AUDIO_BOARD_H_
#define _AUDIO_BOARD_H_

#include "esp_err.h"
#include "audio_hal.h"
#include "board_pins_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio board handle type (stub - not used)
 */
typedef void* audio_board_handle_t;

/**
 * @brief Audio board configuration (stub values)
 */
#define AUDIO_CODEC_DEFAULT_CONFIG(){                   \
        .adc_input  = AUDIO_HAL_ADC_INPUT_LINE1,        \
        .dac_output = AUDIO_HAL_DAC_OUTPUT_ALL,         \
        .codec_mode = AUDIO_HAL_CODEC_MODE_BOTH,        \
        .i2s_iface = {                                  \
            .mode = AUDIO_HAL_MODE_SLAVE,               \
            .fmt = AUDIO_HAL_I2S_NORMAL,                \
            .samples = AUDIO_HAL_48K_SAMPLES,           \
            .bits = AUDIO_HAL_BIT_LENGTH_16BITS,        \
        },                                              \
};

/**
 * @brief Board PA gain stub (not used)
 */
#define BOARD_PA_GAIN           0

/**
 * @brief Get I2C pins (stub - returns error)
 */
esp_err_t get_i2c_pins(int port, void *config);

/**
 * @brief Get PA enable GPIO (stub - returns invalid)
 */
int get_pa_enable_gpio(void);

/**
 * @brief Initialize audio board (stub - does nothing)
 */
esp_err_t audio_board_init(void);

/**
 * @brief Query audio_board_handle (stub - returns NULL)
 */
audio_board_handle_t audio_board_get_handle(void);

/**
 * @brief Uninitialize audio board (stub - does nothing)
 */
esp_err_t audio_board_deinit(void);

#ifdef __cplusplus
}
#endif

#endif  // _AUDIO_BOARD_H_
