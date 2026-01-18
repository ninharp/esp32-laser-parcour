/**
 * Sound Manager Component - Header
 * 
 * I2S audio playback for WAV/MP3 files from SD card using ESP-ADF.
 * Replaces buzzer with high-quality audio feedback.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sound events - maps to sound files
 */
typedef enum {
    SOUND_EVENT_STARTUP = 0,        // System startup
    SOUND_EVENT_BUTTON_PRESS,       // Button press beep
    SOUND_EVENT_GAME_START,         // Game starting
    SOUND_EVENT_COUNTDOWN,          // Countdown tick
    SOUND_EVENT_GAME_RUNNING,       // Background music during game
    SOUND_EVENT_BEAM_BREAK,         // Laser beam broken (penalty)
    SOUND_EVENT_GAME_FINISH,        // Successful game completion
    SOUND_EVENT_GAME_STOP,          // Game stopped/aborted
    SOUND_EVENT_ERROR,              // Error sound
    SOUND_EVENT_SUCCESS,            // Success/confirmation sound
    SOUND_EVENT_MAX                 // Sentinel value
} sound_event_t;

/**
 * Sound playback modes
 */
typedef enum {
    SOUND_MODE_ONCE = 0,            // Play once and stop
    SOUND_MODE_LOOP                 // Loop continuously (for background music)
} sound_mode_t;

/**
 * Sound configuration structure
 */
typedef struct {
    int bck_io_num;                 // I2S BCK pin
    int ws_io_num;                  // I2S WS pin
    int data_out_num;               // I2S Data Out pin
    const char *sound_dir;          // Directory with sound files
    uint8_t default_volume;         // Default volume (0-100)
} sound_config_t;

/**
 * Initialize sound manager
 * 
 * @param config Sound configuration (NULL for defaults from Kconfig)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sound_manager_init(const sound_config_t *config);

/**
 * Deinitialize sound manager
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sound_manager_deinit(void);

/**
 * Play sound for specific event
 * 
 * @param event Sound event to play
 * @param mode Playback mode (once or loop)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sound_manager_play_event(sound_event_t event, sound_mode_t mode);

/**
 * Play sound file by name
 * 
 * @param filename Name of sound file (without path)
 * @param mode Playback mode (once or loop)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sound_manager_play_file(const char *filename, sound_mode_t mode);

/**
 * Stop currently playing sound
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sound_manager_stop(void);

/**
 * Set volume level
 * 
 * @param volume Volume level (0-100)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sound_manager_set_volume(uint8_t volume);

/**
 * Get current volume level
 * 
 * @return Current volume (0-100), or -1 on error
 */
int sound_manager_get_volume(void);

/**
 * Check if sound manager is initialized and ready
 * 
 * @return true if ready, false otherwise
 */
bool sound_manager_is_ready(void);

/**
 * Start streaming pipeline (HTTP/webradio)
 * Call this AFTER WiFi is connected!
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sound_manager_start_streaming(void);

/**
 * Set sound file mapping for event
 * 
 * @param event Sound event
 * @param filename Sound file name (NULL to use default)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sound_manager_set_event_file(sound_event_t event, const char *filename);

/**
 * Get sound file mapping for event
 * 
 * @param event Sound event
 * @return Filename or NULL if not set
 */
const char* sound_manager_get_event_file(sound_event_t event);

/**
 * Save current sound configuration to NVS
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sound_manager_save_config(void);

/**
 * Load sound configuration from NVS
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sound_manager_load_config(void);

#ifdef __cplusplus
}
#endif

#endif // SOUND_MANAGER_H
