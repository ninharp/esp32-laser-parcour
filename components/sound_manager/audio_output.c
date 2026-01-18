/**
 * Audio Output Manager - Implementation
 * 
 * Unified interface with automatic fallback.
 */

#include "audio_output.h"
#include "esp_log.h"

static const char *TAG = "AUDIO_OUT";

/**
 * Map audio events to buzzer patterns
 */
static buzzer_pattern_t event_to_buzzer_pattern(audio_event_t event)
{
    switch (event) {
        case AUDIO_EVENT_STARTUP:
        case AUDIO_EVENT_SUCCESS:
            return BUZZER_PATTERN_SUCCESS;
        case AUDIO_EVENT_BUTTON_PRESS:
            return BUZZER_PATTERN_BEEP;
        case AUDIO_EVENT_GAME_START:
            return BUZZER_PATTERN_GAME_START;
        case AUDIO_EVENT_COUNTDOWN:
            return BUZZER_PATTERN_COUNTDOWN;
        case AUDIO_EVENT_BEAM_BREAK:
        case AUDIO_EVENT_ERROR:
            return BUZZER_PATTERN_ERROR;
        case AUDIO_EVENT_GAME_FINISH:
        case AUDIO_EVENT_GAME_STOP:
            return BUZZER_PATTERN_GAME_END;
        case AUDIO_EVENT_GAME_RUNNING:
            return BUZZER_PATTERN_BEEP; // Buzzer doesn't support background
        default:
            return BUZZER_PATTERN_BEEP;
    }
}

esp_err_t audio_play_event(audio_event_t event, bool loop)
{
    esp_err_t ret = ESP_FAIL;
    
#ifdef CONFIG_ENABLE_SOUND_MANAGER
    // Try sound manager first
    if (sound_manager_is_ready()) {
        sound_mode_t mode = loop ? SOUND_MODE_LOOP : SOUND_MODE_ONCE;
        ret = sound_manager_play_event((sound_event_t)event, mode);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Playing sound for event %d", event);
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "Sound playback failed: %s, falling back to buzzer", 
                     esp_err_to_name(ret));
        }
    }
#endif

#ifdef CONFIG_ENABLE_BUZZER
    // Fallback to buzzer (ignore loop for buzzer)
    buzzer_pattern_t pattern = event_to_buzzer_pattern(event);
    ret = buzzer_play_pattern(pattern);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Playing buzzer pattern for event %d", event);
    } else {
        ESP_LOGW(TAG, "Buzzer playback failed: %s", esp_err_to_name(ret));
    }
#endif

    return ret;
}

esp_err_t audio_stop(void)
{
#ifdef CONFIG_ENABLE_SOUND_MANAGER
    if (sound_manager_is_ready()) {
        return sound_manager_stop();
    }
#endif
    
    // Buzzer doesn't have explicit stop function
    return ESP_OK;
}
