/**
 * Buzzer Component
 * 
 * PWM-based buzzer/speaker control for audio feedback.
 * 
 * @author ninharp
 * @date 2025
 */

#include "buzzer.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUZZER";

#define BUZZER_LEDC_TIMER          LEDC_TIMER_1
#define BUZZER_LEDC_MODE           LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_CHANNEL        LEDC_CHANNEL_1
#define BUZZER_LEDC_DUTY_RES       LEDC_TIMER_10_BIT
#define BUZZER_MAX_DUTY            ((1 << BUZZER_LEDC_DUTY_RES) - 1)

static gpio_num_t buzzer_pin = -1;
static bool is_initialized = false;
static uint8_t current_volume = 50; // 0-100%

/**
 * Initialize buzzer
 */
esp_err_t buzzer_init(gpio_num_t pin)
{
    if (pin == -1) {
        ESP_LOGI(TAG, "Buzzer disabled (pin = -1)");
        return ESP_OK;
    }

    if (is_initialized) {
        ESP_LOGW(TAG, "Buzzer already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing buzzer on GPIO %d...", pin);

    buzzer_pin = pin;

    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = BUZZER_LEDC_MODE,
        .timer_num        = BUZZER_LEDC_TIMER,
        .duty_resolution  = BUZZER_LEDC_DUTY_RES,
        .freq_hz          = 1000,  // Default frequency
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = BUZZER_LEDC_MODE,
        .channel        = BUZZER_LEDC_CHANNEL,
        .timer_sel      = BUZZER_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = pin,
        .duty           = 0,
        .hpoint         = 0
    };
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(ret));
        return ret;
    }

    is_initialized = true;
    ESP_LOGI(TAG, "Buzzer initialized");
    return ESP_OK;
}

/**
 * Deinitialize buzzer
 */
esp_err_t buzzer_deinit(void)
{
    if (!is_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing buzzer...");
    buzzer_stop();
    
    ledc_stop(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0);
    
    is_initialized = false;
    buzzer_pin = -1;
    return ESP_OK;
}

/**
 * Play tone at specific frequency
 */
esp_err_t buzzer_play_tone(uint32_t frequency, uint32_t duration_ms)
{
    if (!is_initialized || buzzer_pin == -1) {
        return ESP_ERR_INVALID_STATE;
    }

    if (frequency == 0) {
        return buzzer_stop();
    }

    ESP_LOGD(TAG, "Playing tone: %lu Hz for %lu ms", frequency, duration_ms);

    // Set frequency
    esp_err_t ret = ledc_set_freq(BUZZER_LEDC_MODE, BUZZER_LEDC_TIMER, frequency);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set frequency: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set duty cycle based on volume (50% duty for square wave)
    uint32_t duty = (BUZZER_MAX_DUTY * current_volume * 50) / (100 * 100);
    ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, duty);
    ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);

    // If duration specified, stop after duration
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        buzzer_stop();
    }

    return ESP_OK;
}

/**
 * Play predefined pattern
 */
esp_err_t buzzer_play_pattern(buzzer_pattern_t pattern)
{
    if (!is_initialized || buzzer_pin == -1) {
        return ESP_ERR_INVALID_STATE;
    }

    switch (pattern) {
        case BUZZER_PATTERN_BEEP:
            buzzer_play_tone(BUZZER_NOTE_A4, 100);
            break;

        case BUZZER_PATTERN_DOUBLE_BEEP:
            buzzer_play_tone(BUZZER_NOTE_A4, 100);
            vTaskDelay(pdMS_TO_TICKS(100));
            buzzer_play_tone(BUZZER_NOTE_A4, 100);
            break;

        case BUZZER_PATTERN_SUCCESS:
            buzzer_play_tone(BUZZER_NOTE_C4, 150);
            buzzer_play_tone(BUZZER_NOTE_E4, 150);
            buzzer_play_tone(BUZZER_NOTE_G4, 200);
            break;

        case BUZZER_PATTERN_ERROR:
            buzzer_play_tone(BUZZER_NOTE_C4, 300);
            vTaskDelay(pdMS_TO_TICKS(50));
            buzzer_play_tone(BUZZER_NOTE_C4, 300);
            break;

        case BUZZER_PATTERN_COUNTDOWN:
            buzzer_play_tone(BUZZER_NOTE_C4, 100);
            break;

        case BUZZER_PATTERN_GAME_START:
            buzzer_play_tone(BUZZER_NOTE_E4, 100);
            buzzer_play_tone(BUZZER_NOTE_G4, 100);
            buzzer_play_tone(BUZZER_NOTE_C5, 200);
            break;

        case BUZZER_PATTERN_GAME_END:
            buzzer_play_tone(BUZZER_NOTE_C5, 150);
            buzzer_play_tone(BUZZER_NOTE_A4, 150);
            buzzer_play_tone(BUZZER_NOTE_F4, 200);
            break;

        default:
            return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

/**
 * Stop buzzer
 */
esp_err_t buzzer_stop(void)
{
    if (!is_initialized || buzzer_pin == -1) {
        return ESP_ERR_INVALID_STATE;
    }

    ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0);
    ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);

    return ESP_OK;
}

/**
 * Set buzzer volume
 */
esp_err_t buzzer_set_volume(uint8_t volume)
{
    if (volume > 100) {
        return ESP_ERR_INVALID_ARG;
    }

    current_volume = volume;
    ESP_LOGI(TAG, "Volume set to %d%%", volume);
    return ESP_OK;
}
