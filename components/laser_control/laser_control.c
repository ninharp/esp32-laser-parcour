/**
 * Laser Control Component - Implementation
 * 
 * Controls laser diode with PWM and safety features.
 * 
 * @author ninharp
 * @date 2025
 */

#include "laser_control.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "LASER_CTRL";

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY          (5000) // 5 kHz

static gpio_num_t laser_gpio = GPIO_NUM_NC;
static laser_status_t current_status = LASER_OFF;
static bool safety_timeout_enabled = true;
static esp_timer_handle_t safety_timer = NULL;

/**
 * Safety timeout callback
 */
static void safety_timeout_cb(void *arg)
{
    ESP_LOGW(TAG, "Safety timeout triggered - turning off laser");
    laser_turn_off();
}

/**
 * Initialize laser control
 */
esp_err_t laser_control_init(gpio_num_t laser_pin)
{
    ESP_LOGI(TAG, "Initializing laser control on GPIO %d...", laser_pin);
    
    laser_gpio = laser_pin;
    
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    
    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num = laser_gpio,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    
    // Create safety timeout timer (10 minutes)
    const esp_timer_create_args_t timer_args = {
        .callback = &safety_timeout_cb,
        .name = "laser_safety"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &safety_timer));
    
    current_status = LASER_OFF;
    
    ESP_LOGI(TAG, "Laser control initialized");
    
    return ESP_OK;
}

/**
 * Turn laser on
 */
esp_err_t laser_turn_on(uint8_t intensity)
{
    if (intensity > 100) {
        intensity = 100;
    }
    
    // Set PWM duty cycle (0-255 for 8-bit resolution)
    uint32_t duty = (intensity * 255) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    
    current_status = LASER_ON;
    
    // Start safety timeout if enabled
    if (safety_timeout_enabled && safety_timer) {
        esp_timer_start_once(safety_timer, 10 * 60 * 1000000ULL); // 10 minutes
    }
    
    ESP_LOGI(TAG, "Laser turned ON (intensity: %d%%)", intensity);
    
    return ESP_OK;
}

/**
 * Turn laser off
 */
esp_err_t laser_turn_off(void)
{
    // Set PWM duty to 0
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    
    current_status = LASER_OFF;
    
    // Stop safety timer
    if (safety_timer && esp_timer_is_active(safety_timer)) {
        esp_timer_stop(safety_timer);
    }
    
    ESP_LOGI(TAG, "Laser turned OFF");
    
    return ESP_OK;
}

/**
 * Set laser intensity
 */
esp_err_t laser_set_intensity(uint8_t intensity)
{
    if (current_status != LASER_ON) {
        ESP_LOGW(TAG, "Laser is not on");
        return ESP_FAIL;
    }
    
    return laser_turn_on(intensity);
}

/**
 * Get laser status
 */
laser_status_t laser_get_status(void)
{
    return current_status;
}

/**
 * Enable/disable safety timeout
 */
esp_err_t laser_set_safety_timeout(bool enable)
{
    safety_timeout_enabled = enable;
    
    ESP_LOGI(TAG, "Safety timeout %s", enable ? "enabled" : "disabled");
    
    return ESP_OK;
}
