/**
 * Buzzer Component - Header
 * 
 * PWM-based buzzer/speaker control for audio feedback.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Predefined tones (frequencies in Hz)
 */
#define BUZZER_NOTE_C4  262
#define BUZZER_NOTE_D4  294
#define BUZZER_NOTE_E4  330
#define BUZZER_NOTE_F4  349
#define BUZZER_NOTE_G4  392
#define BUZZER_NOTE_A4  440
#define BUZZER_NOTE_B4  494
#define BUZZER_NOTE_C5  523

/**
 * Buzzer patterns
 */
typedef enum {
    BUZZER_PATTERN_BEEP = 0,        // Single beep
    BUZZER_PATTERN_DOUBLE_BEEP,     // Two quick beeps
    BUZZER_PATTERN_SUCCESS,         // Success melody
    BUZZER_PATTERN_ERROR,           // Error sound
    BUZZER_PATTERN_COUNTDOWN,       // Countdown tick
    BUZZER_PATTERN_GAME_START,      // Game start sound
    BUZZER_PATTERN_GAME_END         // Game end sound
} buzzer_pattern_t;

/**
 * Initialize buzzer
 * 
 * @param pin GPIO pin for buzzer (-1 to disable)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_init(gpio_num_t pin);

/**
 * Deinitialize buzzer
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_deinit(void);

/**
 * Play tone at specific frequency
 * 
 * @param frequency Frequency in Hz (0 to stop)
 * @param duration_ms Duration in milliseconds (0 for continuous)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_play_tone(uint32_t frequency, uint32_t duration_ms);

/**
 * Play predefined pattern
 * 
 * @param pattern Pattern to play
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_play_pattern(buzzer_pattern_t pattern);

/**
 * Stop buzzer
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_stop(void);

/**
 * Set buzzer volume
 * 
 * @param volume Volume level (0-100%)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_set_volume(uint8_t volume);

#ifdef __cplusplus
}
#endif

#endif // BUZZER_H
