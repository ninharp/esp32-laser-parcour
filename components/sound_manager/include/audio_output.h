/**
 * Audio Output Manager - Unified interface for sound/buzzer output
 * 
 * Provides automatic fallback from sound manager to buzzer.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include "esp_err.h"
#include "sound_manager.h"
#include "buzzer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Audio output events (compatible with both sound and buzzer)
 */
typedef enum {
    AUDIO_EVENT_STARTUP = SOUND_EVENT_STARTUP,
    AUDIO_EVENT_BUTTON_PRESS = SOUND_EVENT_BUTTON_PRESS,
    AUDIO_EVENT_GAME_START = SOUND_EVENT_GAME_START,
    AUDIO_EVENT_COUNTDOWN = SOUND_EVENT_COUNTDOWN,
    AUDIO_EVENT_GAME_RUNNING = SOUND_EVENT_GAME_RUNNING,
    AUDIO_EVENT_BEAM_BREAK = SOUND_EVENT_BEAM_BREAK,
    AUDIO_EVENT_GAME_FINISH = SOUND_EVENT_GAME_FINISH,
    AUDIO_EVENT_GAME_STOP = SOUND_EVENT_GAME_STOP,
    AUDIO_EVENT_ERROR = SOUND_EVENT_ERROR,
    AUDIO_EVENT_SUCCESS = SOUND_EVENT_SUCCESS
} audio_event_t;

/**
 * Play audio for event
 * Uses sound manager if available, falls back to buzzer
 * 
 * @param event Audio event to play
 * @param loop True for looping playback (sound only)
 * @return ESP_OK on success
 */
esp_err_t audio_play_event(audio_event_t event, bool loop);

/**
 * Stop audio playback
 * 
 * @return ESP_OK on success
 */
esp_err_t audio_stop(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_OUTPUT_H
