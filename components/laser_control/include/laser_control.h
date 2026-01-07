/**
 * Laser Control Component - Header
 * 
 * Controls laser diode module with PWM and safety features.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef LASER_CONTROL_H
#define LASER_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Laser status
 */
typedef enum {
    LASER_OFF = 0,
    LASER_ON,
    LASER_STANDBY,
    LASER_ERROR
} laser_status_t;

/**
 * Initialize laser control
 * 
 * @param laser_pin GPIO pin for laser control
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t laser_control_init(gpio_num_t laser_pin);

/**
 * Turn laser on
 * 
 * @param intensity Intensity (0-100%)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t laser_turn_on(uint8_t intensity);

/**
 * Turn laser off
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t laser_turn_off(void);

/**
 * Set laser intensity
 * 
 * @param intensity Intensity (0-100%)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t laser_set_intensity(uint8_t intensity);

/**
 * Get laser status
 * 
 * @return Current laser status
 */
laser_status_t laser_get_status(void);

/**
 * Enable/disable safety timeout (auto-off after 10 minutes)
 * 
 * @param enable true to enable, false to disable
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t laser_set_safety_timeout(bool enable);

#ifdef __cplusplus
}
#endif

#endif // LASER_CONTROL_H
