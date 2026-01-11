/**
 * Button Handler Component - Header
 * 
 * Manages button inputs with debouncing and event callbacks.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BUTTONS 3

/**
 * Button event types
 */
typedef enum {
    BUTTON_EVENT_PRESSED = 0,       // Button pressed
    BUTTON_EVENT_RELEASED,          // Button released
    BUTTON_EVENT_CLICK,             // Short click
    BUTTON_EVENT_LONG_PRESS,        // Long press (>1 second)
    BUTTON_EVENT_DOUBLE_CLICK       // Double click
} button_event_t;

/**
 * Button callback function
 * 
 * @param button_id Button identifier (0-3)
 * @param event Event type
 */
typedef void (*button_callback_t)(uint8_t button_id, button_event_t event);

/**
 * Button configuration
 */
typedef struct {
    gpio_num_t pin;                 // GPIO pin (-1 to disable)
    uint32_t debounce_time_ms;      // Debounce time in milliseconds
    uint32_t long_press_time_ms;    // Long press threshold in milliseconds
    bool pull_up;                   // Enable internal pull-up
    bool active_low;                // true if button is active low
} button_config_t;

/**
 * Initialize button handler
 * 
 * @param buttons Array of button configurations (up to 4 buttons)
 * @param num_buttons Number of buttons
 * @param callback Event callback function
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_handler_init(const button_config_t *buttons, uint8_t num_buttons, 
                              button_callback_t callback);

/**
 * Deinitialize button handler
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_handler_deinit(void);

/**
 * Get button state
 * 
 * @param button_id Button identifier (0-3)
 * @param pressed Pointer to store pressed state
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_get_state(uint8_t button_id, bool *pressed);

#ifdef __cplusplus
}
#endif

#endif // BUTTON_HANDLER_H
