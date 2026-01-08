/**
 * SSD1306 OLED Display Driver - Header
 * 
 * Low-level driver for SSD1306 128x64 OLED displays via I2C.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES (SSD1306_HEIGHT / 8)

/**
 * Initialize SSD1306 display
 * 
 * @param sda_pin GPIO pin for I2C SDA
 * @param scl_pin GPIO pin for I2C SCL
 * @param freq_hz I2C frequency in Hz
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ssd1306_init(gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t freq_hz);

/**
 * Clear the framebuffer
 * 
 * @return ESP_OK on success
 */
esp_err_t ssd1306_clear(void);

/**
 * Send framebuffer to display
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ssd1306_update(void);

/**
 * Draw a character to framebuffer
 * 
 * @param x X position (0-127)
 * @param page Page number (0-7)
 * @param c Character to draw
 */
void ssd1306_draw_char(uint8_t x, uint8_t page, char c);

/**
 * Draw a string to framebuffer
 * 
 * @param x X position (0-127)
 * @param page Page number (0-7)
 * @param str String to draw
 */
void ssd1306_draw_string(uint8_t x, uint8_t page, const char *str);

/**
 * Draw a large digit (3x size) to framebuffer
 * 
 * @param x X position
 * @param page Starting page
 * @param digit Digit character ('0'-'9')
 */
void ssd1306_draw_large_digit(uint8_t x, uint8_t page, char digit);

/**
 * Draw a horizontal line
 * 
 * @param page Page number
 * @param pattern Line pattern (0xFF for solid line)
 */
void ssd1306_draw_hline(uint8_t page, uint8_t pattern);

/**
 * Set display contrast
 * 
 * @param contrast Contrast value (0-255)
 * @return ESP_OK on success
 */
esp_err_t ssd1306_set_contrast(uint8_t contrast);

/**
 * Turn display on/off
 * 
 * @param on true to turn on, false to turn off
 * @return ESP_OK on success
 */
esp_err_t ssd1306_display_power(bool on);

/**
 * Get direct access to framebuffer (for advanced operations)
 * 
 * @return Pointer to framebuffer
 */
uint8_t* ssd1306_get_framebuffer(void);

#ifdef __cplusplus
}
#endif

#endif // SSD1306_H
