/**
 * Sensor Manager Component - Header
 * 
 * Manages photoresistor/photodiode sensors for beam detection.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sensor status
 */
typedef enum {
    SENSOR_BEAM_DETECTED = 0,    // Beam is present
    SENSOR_BEAM_BROKEN,          // Beam is broken
    SENSOR_ERROR                  // Sensor error
} sensor_status_t;

/**
 * Beam break callback
 * 
 * @param sensor_id Sensor identifier
 */
typedef void (*beam_break_callback_t)(uint8_t sensor_id);

/**
 * Beam restore callback
 * 
 * @param sensor_id Sensor identifier
 */
typedef void (*beam_restore_callback_t)(uint8_t sensor_id);

/**
 * Initialize sensor manager
 * 
 * @param adc_channel ADC channel for sensor
 * @param threshold Detection threshold (0-4095)
 * @param debounce_ms Debounce time in milliseconds
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_manager_init(uint8_t adc_channel, uint16_t threshold, uint32_t debounce_ms);

/**
 * Register beam break callback
 * 
 * @param callback Callback function
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_register_callback(beam_break_callback_t callback);

/**
 * Register beam restore callback
 * 
 * @param callback Callback function
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_register_restore_callback(beam_restore_callback_t callback);

/**
 * Read current ADC value
 * 
 * @param value Pointer to store ADC value
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_read_value(uint16_t *value);

/**
 * Get current sensor status
 * 
 * @return Current sensor status
 */
sensor_status_t sensor_get_status(void);

/**
 * Set detection threshold
 * 
 * @param threshold New threshold value (0-4095)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_set_threshold(uint16_t threshold);

/**
 * Calibrate sensor (set threshold based on current reading)
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_calibrate(void);

/**
 * Start sensor monitoring
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_start_monitoring(void);

/**
 * Stop sensor monitoring
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sensor_stop_monitoring(void);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_MANAGER_H
