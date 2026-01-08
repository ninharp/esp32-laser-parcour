/**
 * Game Logic Component - Header
 * 
 * Manages game state, scoring, timing, and game modes for the laser parcour system.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LASER_UNITS 10

/**
 * Game states
 */
typedef enum {
    GAME_STATE_IDLE = 0,        // System idle, waiting for start
    GAME_STATE_READY,            // Ready to start, countdown not started
    GAME_STATE_COUNTDOWN,        // Pre-game countdown in progress
    GAME_STATE_RUNNING,          // Game actively running
    GAME_STATE_PENALTY,          // Beam broken, penalty mode
    GAME_STATE_PAUSED,           // Game paused
    GAME_STATE_COMPLETE,         // Game finished
    GAME_STATE_ERROR             // Error state
} game_state_t;

/**
 * Completion status - how the game ended
 */
typedef enum {
    COMPLETION_NONE = 0,         // Game not completed yet
    COMPLETION_SOLVED,           // Completed via finish button
    COMPLETION_ABORTED_TIME,     // Aborted due to max time
    COMPLETION_ABORTED_MANUAL    // Manually aborted via web interface
} completion_status_t;

/**
 * Game modes
 */
typedef enum {
    GAME_MODE_SINGLE_SPEEDRUN = 0,  // Single player speed run
    GAME_MODE_MULTIPLAYER,           // Multi-player challenge
    GAME_MODE_TRAINING,              // Training mode (no penalties)
    GAME_MODE_CUSTOM                 // Custom game settings
} game_mode_t;

/**
 * Player data structure
 */
typedef struct {
    uint8_t player_id;           // Player identifier
    char name[32];               // Player name
    uint32_t start_time;         // Game start timestamp (ms)
    uint32_t end_time;           // Game end timestamp (ms)
    uint32_t elapsed_time;       // Total elapsed time (ms) - counts UP from 0
    uint16_t beam_breaks;        // Number of beam breaks
    completion_status_t completion; // How the game ended
    bool is_active;              // Is this player currently active
} player_data_t;

/**
 * Game statistics
 */
typedef struct {
    uint32_t total_games;        // Total games played
    uint32_t best_time;          // Best completion time (ms)
    uint32_t worst_time;         // Worst completion time (ms)
    uint32_t avg_time;           // Average completion time (ms)
    uint32_t total_beam_breaks;  // Total beam breaks across all games
    uint32_t total_playtime;     // Total playtime (ms)
} game_stats_t;

/**
 * Game configuration
 */
typedef struct {
    game_mode_t mode;            // Current game mode
    uint32_t max_time;           // Maximum time in seconds (0 = no limit)
    uint32_t penalty_time;       // Penalty time per beam break (seconds)
    uint32_t countdown_time;     // Pre-game countdown (seconds)
    uint8_t max_players;         // Maximum players for multiplayer
} game_config_t;

/**
 * Initialize game logic component
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_logic_init(void);

/**
 * Start a new game
 * 
 * @param mode Game mode to start
 * @param player_name Name of the player (optional, can be NULL)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_start(game_mode_t mode, const char *player_name);

/**
 * Finish the current game via finish button (successful completion)
 * Sets completion status to COMPLETION_SOLVED
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_finish(void);

/**
 * Stop the current game (abort/cancel)
 * Sets completion status to ABORTED_MANUAL if not already set
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_stop(void);

/**
 * Pause the current game
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_pause(void);

/**
 * Resume a paused game
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_resume(void);

/**
 * Register a beam break event
 * 
 * @param sensor_id ID of the sensor that detected the break
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_beam_broken(uint8_t sensor_id);

/**
 * Get current game state
 * 
 * @return Current game state
 */
game_state_t game_get_state(void);

/**
 * Get current player data
 * 
 * @param player_data Pointer to store player data
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_get_player_data(player_data_t *player_data);

/**
 * Get game statistics
 * 
 * @param stats Pointer to store statistics
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_get_stats(game_stats_t *stats);

/**
 * Get game configuration
 * 
 * @param config Pointer to store configuration
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_get_config(game_config_t *config);

/**
 * Set game configuration
 * 
 * @param config Pointer to configuration to set
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_set_config(const game_config_t *config);

/**

 * Reset game statistics
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_reset_stats(void);

/**
 * Laser Unit information
 */
typedef struct {
    uint8_t module_id;           // Module ID
    uint8_t mac_addr[6];         // MAC address
    bool is_online;              // Is unit responding
    bool laser_on;               // Is laser currently on
    uint32_t last_seen;          // Last heartbeat timestamp (ms)
    int8_t rssi;                 // Signal strength
    char status[32];             // Status text
} laser_unit_info_t;

/**
 * Get list of all registered laser units
 * 
 * @param units Array to store unit information
 * @param max_units Maximum number of units to retrieve
 * @param unit_count Pointer to store actual number of units
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_get_laser_units(laser_unit_info_t *units, size_t max_units, size_t *unit_count);

/**
 * Control laser unit
 * 
 * @param module_id Module ID to control
 * @param laser_on true to turn laser on, false to turn off
 * @param intensity Laser intensity (0-100), only used if laser_on is true
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_control_laser(uint8_t module_id, bool laser_on, uint8_t intensity);

/**
 * Reset laser unit
 * 
 * @param module_id Module ID to reset
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_reset_laser_unit(uint8_t module_id);

/**
 * Update laser unit tracking (call from ESP-NOW message handler)
 * 
 * @param module_id Module ID
 * @param mac_addr MAC address of the unit
 * @param rssi Signal strength
 */
void game_update_laser_unit(uint8_t module_id, const uint8_t *mac_addr, int8_t rssi);

#ifdef __cplusplus
}
#endif

#endif // GAME_LOGIC_H
