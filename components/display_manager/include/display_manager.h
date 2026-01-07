/**
 * Display Manager Component - Header
 * 
 * Manages OLED display (SSD1306/SH1106) and UI rendering.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Display screen types
 */
typedef enum {
    SCREEN_IDLE = 0,            // Idle/welcome screen
    SCREEN_MENU,                // Main menu
    SCREEN_GAME_COUNTDOWN,      // Pre-game countdown
    SCREEN_GAME_RUNNING,        // Active game display
    SCREEN_GAME_PAUSED,         // Paused game
    SCREEN_GAME_COMPLETE,       // Game results
    SCREEN_SETTINGS,            // Settings menu
    SCREEN_STATS                // Statistics display
} display_screen_t;

/**
 * Initialize display manager
 * 
 * @param sda_pin GPIO pin for I2C SDA
 * @param scl_pin GPIO pin for I2C SCL
 * @param freq_hz I2C frequency in Hz
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_manager_init(gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t freq_hz);

/**
 * Clear the display
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_clear(void);

/**
 * Update display (refresh screen)
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_update(void);

/**
 * Set current screen
 * 
 * @param screen Screen type to display
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_set_screen(display_screen_t screen);

/**
 * Display game time and score
 * 
 * @param elapsed_time Elapsed time in milliseconds
 * @param beam_breaks Number of beam breaks
 * @param score Current score
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_game_status(uint32_t elapsed_time, uint16_t beam_breaks, int32_t score);

/**
 * Display countdown
 * 
 * @param seconds Seconds remaining
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_countdown(uint8_t seconds);

/**
 * Display text message
 * 
 * @param message Text to display
 * @param line Line number (0-7 for 128x64 display)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_text(const char *message, uint8_t line);

/**
 * Display game results
 * 
 * @param final_time Final time in milliseconds
 * @param beam_breaks Number of beam breaks
 * @param final_score Final score
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_game_results(uint32_t final_time, uint16_t beam_breaks, int32_t final_score);

/**
 * Set display contrast
 * 
 * @param contrast Contrast value (0-255)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_set_contrast(uint8_t contrast);

/**
 * Turn display on/off
 * 
 * @param on true to turn on, false to turn off
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_power(bool on);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_MANAGER_H
