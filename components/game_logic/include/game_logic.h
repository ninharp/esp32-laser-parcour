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
    uint32_t elapsed_time;       // Total elapsed time (ms)
    uint16_t beam_breaks;        // Number of beam breaks
    int32_t score;               // Final score
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
    uint32_t duration;           // Game duration in seconds
    uint32_t penalty_time;       // Penalty time per beam break (seconds)
    uint32_t countdown_time;     // Pre-game countdown (seconds)
    int32_t base_score;          // Starting score
    int32_t time_bonus_mult;     // Time bonus multiplier
    int32_t penalty_points;      // Points deducted per beam break
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
 * Stop the current game
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
 * Calculate final score based on time and penalties
 * 
 * @param elapsed_time Time elapsed in milliseconds
 * @param beam_breaks Number of beam breaks
 * @return Calculated score
 */
int32_t game_calculate_score(uint32_t elapsed_time, uint16_t beam_breaks);

/**
 * Reset game statistics
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t game_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif // GAME_LOGIC_H
