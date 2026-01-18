/**
 * Button Handler Component
 * 
 * Manages button inputs with debouncing and event callbacks.
 * 
 * @author ninharp
 * @date 2025
 */

#include "button_handler.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "BUTTON";

#define BUTTON_TASK_STACK_SIZE 4096  // Increased from 2048 to prevent stack overflow
#define BUTTON_TASK_PRIORITY 5
#define DOUBLE_CLICK_TIME_MS 300

typedef struct {
    button_config_t config;
    uint8_t id;
    bool current_state;
    bool last_state;
    uint64_t press_time;
    uint64_t release_time;
    bool long_press_triggered;
    uint8_t click_count;
} button_state_t;

static button_state_t buttons[MAX_BUTTONS];
static uint8_t num_buttons_active = 0;
static button_callback_t event_callback = NULL;
static TaskHandle_t button_task_handle = NULL;
static bool is_initialized = false;

/**
 * Button polling task
 */
static void button_task(void *arg)
{
    while (1) {
        uint64_t now = esp_timer_get_time() / 1000; // Convert to milliseconds

        for (uint8_t i = 0; i < num_buttons_active; i++) {
            if (buttons[i].config.pin == -1) {
                continue;
            }

            // Read current button state
            int level = gpio_get_level(buttons[i].config.pin);
            bool pressed = buttons[i].config.active_low ? (level == 0) : (level == 1);

            // Detect state change with debouncing
            if (pressed != buttons[i].last_state) {
                vTaskDelay(pdMS_TO_TICKS(buttons[i].config.debounce_time_ms));
                level = gpio_get_level(buttons[i].config.pin);
                pressed = buttons[i].config.active_low ? (level == 0) : (level == 1);

                if (pressed != buttons[i].last_state) {
                    buttons[i].current_state = pressed;
                    buttons[i].last_state = pressed;

                    if (pressed) {
                        // Button pressed
                        buttons[i].press_time = now;
                        buttons[i].long_press_triggered = false;
                        if (event_callback) {
                            event_callback(buttons[i].id, BUTTON_EVENT_PRESSED);
                        }
                    } else {
                        // Button released
                        buttons[i].release_time = now;
                        uint32_t press_duration = now - buttons[i].press_time;

                        if (event_callback) {
                            event_callback(buttons[i].id, BUTTON_EVENT_RELEASED);

                            if (!buttons[i].long_press_triggered) {
                                if (press_duration < buttons[i].config.long_press_time_ms) {
                                    // Click event
                                    buttons[i].click_count++;
                                    
                                    // Wait to see if double click
                                    vTaskDelay(pdMS_TO_TICKS(DOUBLE_CLICK_TIME_MS));
                                    
                                    if (buttons[i].click_count == 1) {
                                        event_callback(buttons[i].id, BUTTON_EVENT_CLICK);
                                    } else if (buttons[i].click_count >= 2) {
                                        event_callback(buttons[i].id, BUTTON_EVENT_DOUBLE_CLICK);
                                    }
                                    buttons[i].click_count = 0;
                                }
                            }
                        }
                    }
                }
            }

            // Check for long press
            if (buttons[i].current_state && !buttons[i].long_press_triggered) {
                uint32_t press_duration = now - buttons[i].press_time;
                if (press_duration >= buttons[i].config.long_press_time_ms) {
                    buttons[i].long_press_triggered = true;
                    if (event_callback) {
                        event_callback(buttons[i].id, BUTTON_EVENT_LONG_PRESS);
                    }
                }
            }

            // Reset double-click counter if timeout
            if (buttons[i].click_count > 0 && 
                (now - buttons[i].release_time) > DOUBLE_CLICK_TIME_MS) {
                buttons[i].click_count = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Poll every 10ms
    }
}

/**
 * Initialize button handler
 */
esp_err_t button_handler_init(const button_config_t *button_configs, uint8_t num_buttons,
                              button_callback_t callback)
{
    if (is_initialized) {
        ESP_LOGW(TAG, "Button handler already initialized");
        return ESP_OK;
    }

    if (!button_configs || num_buttons == 0 || num_buttons > MAX_BUTTONS) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing button handler with %d buttons...", num_buttons);

    num_buttons_active = num_buttons;
    event_callback = callback;

    // Configure each button
    for (uint8_t i = 0; i < num_buttons; i++) {
        if (button_configs[i].pin == -1) {
            ESP_LOGI(TAG, "Button %d disabled (pin = -1)", i);
            buttons[i].config.pin = -1;
            continue;
        }

        buttons[i].config = button_configs[i];
        buttons[i].id = i;
        buttons[i].current_state = false;
        buttons[i].last_state = false;
        buttons[i].press_time = 0;
        buttons[i].release_time = 0;
        buttons[i].long_press_triggered = false;
        buttons[i].click_count = 0;

        // Configure GPIO
        gpio_config_t io_conf = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_INPUT,
            .pin_bit_mask = (1ULL << button_configs[i].pin),
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = button_configs[i].pull_up ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE
        };
        gpio_config(&io_conf);

        ESP_LOGI(TAG, "Button %d configured on GPIO %d", i, button_configs[i].pin);
    }

    // Create button polling task
    xTaskCreate(button_task, "button_task", BUTTON_TASK_STACK_SIZE, NULL,
                BUTTON_TASK_PRIORITY, &button_task_handle);

    is_initialized = true;
    ESP_LOGI(TAG, "Button handler initialized");
    return ESP_OK;
}

/**
 * Deinitialize button handler
 */
esp_err_t button_handler_deinit(void)
{
    if (!is_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing button handler...");

    if (button_task_handle) {
        vTaskDelete(button_task_handle);
        button_task_handle = NULL;
    }

    num_buttons_active = 0;
    event_callback = NULL;
    is_initialized = false;

    return ESP_OK;
}

/**
 * Get button state
 */
esp_err_t button_get_state(uint8_t button_id, bool *pressed)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (button_id >= num_buttons_active || !pressed) {
        return ESP_ERR_INVALID_ARG;
    }

    *pressed = buttons[button_id].current_state;
    return ESP_OK;
}
