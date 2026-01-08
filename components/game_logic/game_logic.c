/**
 * Game Logic Component - Implementation
 * 
 * Manages game state, scoring, timing, and game modes.
 * 
 * @author ninharp
 * @date 2025
 */

#include "game_logic.h"
#include "espnow_manager.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "GAME_LOGIC";

// Global game state
static game_state_t current_state = GAME_STATE_IDLE;
static player_data_t current_player = {0};
static game_stats_t statistics = {0};
static game_config_t configuration = {
    .mode = GAME_MODE_SINGLE_SPEEDRUN,
    .duration = 180,            // 3 minutes default
    .penalty_time = 5,          // 5 seconds penalty
    .countdown_time = 5,        // 5 second countdown
    .base_score = 1000,
    .time_bonus_mult = 10,
    .penalty_points = -50,
    .max_players = 8
};

// Mutex for thread-safe access
static SemaphoreHandle_t game_mutex = NULL;

/**
 * Initialize game logic component
 */
esp_err_t game_logic_init(void)
{
    ESP_LOGI(TAG, "Initializing game logic...");
    
    // Create mutex for thread-safe access
    game_mutex = xSemaphoreCreateMutex();
    if (game_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }
    
    // Initialize game state
    current_state = GAME_STATE_IDLE;
    memset(&current_player, 0, sizeof(player_data_t));
    
    // TODO: Load statistics from NVS
    
    ESP_LOGI(TAG, "Game logic initialized successfully");
    return ESP_OK;
}

/**
 * Start a new game
 */
esp_err_t game_start(game_mode_t mode, const char *player_name)
{
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return ESP_FAIL;
    }
    
    // Check if game is already running
    if (current_state == GAME_STATE_RUNNING || current_state == GAME_STATE_COUNTDOWN) {
        ESP_LOGW(TAG, "Game already running");
        xSemaphoreGive(game_mutex);
        return ESP_FAIL;
    }
    
    // Initialize player data
    memset(&current_player, 0, sizeof(player_data_t));
    current_player.player_id = 1;
    if (player_name) {
        strncpy(current_player.name, player_name, sizeof(current_player.name) - 1);
    } else {
        strcpy(current_player.name, "Player 1");
    }
    current_player.start_time = (uint32_t)(esp_timer_get_time() / 1000);
    current_player.is_active = true;
    
    // Set game mode
    configuration.mode = mode;
    
    // Change state to running (skip countdown for web interface)
    current_state = GAME_STATE_RUNNING;
    
    ESP_LOGI(TAG, "Game starting - Mode: %d, Player: %s", mode, current_player.name);
    
    xSemaphoreGive(game_mutex);
    
    // Send MSG_GAME_START to all registered laser units (unicast)
    ESP_LOGI(TAG, "Sending MSG_GAME_START to all laser units");
    
    // Get current laser units list
    laser_unit_info_t units[MAX_LASER_UNITS];
    size_t unit_count = 0;
    game_get_laser_units(units, MAX_LASER_UNITS, &unit_count);
    
    for (size_t i = 0; i < unit_count; i++) {
        esp_err_t ret = espnow_send_message(units[i].mac_addr, MSG_GAME_START, NULL, 0);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send game start to unit %d: %s", 
                     units[i].module_id, esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "Game start sent to laser unit %d", units[i].module_id);
        }
    }
    
    return ESP_OK;
}

/**
 * Stop the current game
 */
esp_err_t game_stop(void)
{
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return ESP_FAIL;
    }
    
    if (current_state == GAME_STATE_IDLE) {
        ESP_LOGW(TAG, "No game running");
        xSemaphoreGive(game_mutex);
        return ESP_FAIL;
    }
    
    // Record end time
    current_player.end_time = (uint32_t)(esp_timer_get_time() / 1000);
    current_player.elapsed_time = current_player.end_time - current_player.start_time;
    
    // Calculate final score
    current_player.score = game_calculate_score(
        current_player.elapsed_time,
        current_player.beam_breaks
    );
    
    // Update statistics
    statistics.total_games++;
    statistics.total_beam_breaks += current_player.beam_breaks;
    statistics.total_playtime += current_player.elapsed_time;
    
    if (statistics.best_time == 0 || current_player.elapsed_time < statistics.best_time) {
        statistics.best_time = current_player.elapsed_time;
    }
    if (current_player.elapsed_time > statistics.worst_time) {
        statistics.worst_time = current_player.elapsed_time;
    }
    statistics.avg_time = statistics.total_playtime / statistics.total_games;
    
    // Change state
    current_state = GAME_STATE_COMPLETE;
    current_player.is_active = false;
    
    ESP_LOGI(TAG, "Game stopped - Time: %lu ms, Beam Breaks: %d, Score: %ld",
             current_player.elapsed_time, current_player.beam_breaks, current_player.score);
    
    xSemaphoreGive(game_mutex);
    
    // Send MSG_GAME_STOP to all registered laser units (unicast)
    ESP_LOGI(TAG, "Sending MSG_GAME_STOP to all laser units");
    
    // Get current laser units list
    laser_unit_info_t units[MAX_LASER_UNITS];
    size_t unit_count = 0;
    game_get_laser_units(units, MAX_LASER_UNITS, &unit_count);
    
    for (size_t i = 0; i < unit_count; i++) {
        esp_err_t ret = espnow_send_message(units[i].mac_addr, MSG_GAME_STOP, NULL, 0);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send game stop to unit %d: %s", 
                     units[i].module_id, esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "Game stop sent to laser unit %d", units[i].module_id);
        }
    }
    
    // TODO: Save statistics to NVS
    // TODO: Display results on OLED
    
    return ESP_OK;
}

/**
 * Pause the current game
 */
esp_err_t game_pause(void)
{
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    if (current_state != GAME_STATE_RUNNING) {
        xSemaphoreGive(game_mutex);
        return ESP_FAIL;
    }
    
    current_state = GAME_STATE_PAUSED;
    ESP_LOGI(TAG, "Game paused");
    
    xSemaphoreGive(game_mutex);
    return ESP_OK;
}

/**
 * Resume a paused game
 */
esp_err_t game_resume(void)
{
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    if (current_state != GAME_STATE_PAUSED) {
        xSemaphoreGive(game_mutex);
        return ESP_FAIL;
    }
    
    current_state = GAME_STATE_RUNNING;
    ESP_LOGI(TAG, "Game resumed");
    
    xSemaphoreGive(game_mutex);
    return ESP_OK;
}

/**
 * Register a beam break event
 */
esp_err_t game_beam_broken(uint8_t sensor_id)
{
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    if (current_state != GAME_STATE_RUNNING) {
        xSemaphoreGive(game_mutex);
        return ESP_FAIL;
    }
    
    // Increment beam break counter
    current_player.beam_breaks++;
    
    // Enter penalty state (unless in training mode)
    if (configuration.mode != GAME_MODE_TRAINING) {
        current_state = GAME_STATE_PENALTY;
    }
    
    ESP_LOGI(TAG, "Beam broken! Sensor: %d, Total breaks: %d", 
             sensor_id, current_player.beam_breaks);
    
    xSemaphoreGive(game_mutex);
    
    // TODO: Trigger penalty sound/display
    // TODO: Apply time penalty if applicable
    
    return ESP_OK;
}

/**
 * Get current game state
 */
game_state_t game_get_state(void)
{
    return current_state;
}

/**
 * Get current player data
 */
esp_err_t game_get_player_data(player_data_t *player_data)
{
    if (!player_data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    memcpy(player_data, &current_player, sizeof(player_data_t));
    
    // Calculate elapsed time for running games
    if (current_state == GAME_STATE_RUNNING || current_state == GAME_STATE_PENALTY) {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
        player_data->elapsed_time = now - current_player.start_time;
    }
    
    xSemaphoreGive(game_mutex);
    return ESP_OK;
}

/**
 * Get game statistics
 */
esp_err_t game_get_stats(game_stats_t *stats)
{
    if (!stats) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    memcpy(stats, &statistics, sizeof(game_stats_t));
    
    xSemaphoreGive(game_mutex);
    return ESP_OK;
}

/**
 * Get game configuration
 */
esp_err_t game_get_config(game_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    memcpy(config, &configuration, sizeof(game_config_t));
    
    xSemaphoreGive(game_mutex);
    return ESP_OK;
}

/**
 * Set game configuration
 */
esp_err_t game_set_config(const game_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    memcpy(&configuration, config, sizeof(game_config_t));
    
    ESP_LOGI(TAG, "Configuration updated");
    
    xSemaphoreGive(game_mutex);
    return ESP_OK;
}

/**
 * Calculate final score
 */
int32_t game_calculate_score(uint32_t elapsed_time, uint16_t beam_breaks)
{
    int32_t score = configuration.base_score;
    
    // Add time bonus (faster completion = higher score)
    uint32_t time_seconds = elapsed_time / 1000;
    uint32_t time_remaining = (configuration.duration > time_seconds) ? 
                               (configuration.duration - time_seconds) : 0;
    score += (int32_t)(time_remaining * configuration.time_bonus_mult);
    
    // Subtract penalty points
    score += (int32_t)(beam_breaks * configuration.penalty_points);
    
    // Ensure score doesn't go negative
    if (score < 0) {
        score = 0;
    }
    
    return score;
}

/**
 * Reset game statistics
 */
esp_err_t game_reset_stats(void)
{
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_FAIL;
    }
    
    memset(&statistics, 0, sizeof(game_stats_t));
    
    ESP_LOGI(TAG, "Statistics reset");
    
    xSemaphoreGive(game_mutex);
    
    // TODO: Clear NVS statistics
    
    return ESP_OK;
}

// Laser unit tracking

static laser_unit_info_t laser_units[MAX_LASER_UNITS] = {0};
static size_t laser_unit_count = 0;

/**
 * Update or add laser unit to tracking list
 * This should be called when receiving ESP-NOW messages from units
 */
static void update_laser_unit(uint8_t module_id, const uint8_t *mac_addr, int8_t rssi)
{
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    
    // Find existing unit or add new one
    for (size_t i = 0; i < laser_unit_count; i++) {
        if (laser_units[i].module_id == module_id) {
            // Update existing
            memcpy(laser_units[i].mac_addr, mac_addr, 6);
            laser_units[i].last_seen = now;
            laser_units[i].rssi = rssi;
            laser_units[i].is_online = true;
            return;
        }
    }
    
    // Add new unit if space available
    if (laser_unit_count < MAX_LASER_UNITS) {
        laser_units[laser_unit_count].module_id = module_id;
        memcpy(laser_units[laser_unit_count].mac_addr, mac_addr, 6);
        laser_units[laser_unit_count].last_seen = now;
        laser_units[laser_unit_count].rssi = rssi;
        laser_units[laser_unit_count].is_online = true;
        laser_units[laser_unit_count].laser_on = false;
        snprintf(laser_units[laser_unit_count].status, sizeof(laser_units[laser_unit_count].status), "Active");
        laser_unit_count++;
        ESP_LOGI(TAG, "New laser unit registered: ID %d", module_id);
    }
}

/**
 * Get list of all registered laser units
 */
esp_err_t game_get_laser_units(laser_unit_info_t *units, size_t max_units, size_t *unit_count)
{
    if (!units || !unit_count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    
    // Update online status and remove dead units
    // Online timeout: 15 seconds (5x heartbeat interval for stability)
    // Removal timeout: 60 seconds (units that haven't sent anything)
    size_t active_count = 0;
    for (size_t i = 0; i < laser_unit_count; i++) {
        uint32_t time_since_last_seen = now - laser_units[i].last_seen;
        
        // Remove units that haven't sent anything in 60 seconds
        if (time_since_last_seen > 60000) {
            ESP_LOGI(TAG, "Removing inactive laser unit %d (offline for %lu seconds)",
                     laser_units[i].module_id, time_since_last_seen / 1000);
            
            // Also remove from ESP-NOW peers
            extern esp_err_t espnow_remove_peer(const uint8_t *mac_addr);
            espnow_remove_peer(laser_units[i].mac_addr);
            
            continue;  // Skip this unit (don't copy to active list)
        }
        
        // Update online status (15 second timeout)
        if (time_since_last_seen > 15000) {
            laser_units[i].is_online = false;
            snprintf(laser_units[i].status, sizeof(laser_units[i].status), "Offline");
        } else {
            laser_units[i].is_online = true;
            snprintf(laser_units[i].status, sizeof(laser_units[i].status), "Online");
        }
        
        // Copy to compacted array if not at same position
        if (active_count != i) {
            laser_units[active_count] = laser_units[i];
        }
        active_count++;
    }
    
    // Update count to reflect removed units
    if (active_count < laser_unit_count) {
        ESP_LOGI(TAG, "Removed %zu inactive laser units, %zu remaining",
                 laser_unit_count - active_count, active_count);
        laser_unit_count = active_count;
    }
    
    size_t copy_count = (laser_unit_count < max_units) ? laser_unit_count : max_units;
    memcpy(units, laser_units, copy_count * sizeof(laser_unit_info_t));
    *unit_count = copy_count;
    
    return ESP_OK;
}

/**
 * Control laser unit
 */
esp_err_t game_control_laser(uint8_t module_id, bool laser_on, uint8_t intensity)
{
    ESP_LOGI(TAG, "Controlling laser unit %d: %s (intensity: %d)", 
             module_id, laser_on ? "ON" : "OFF", intensity);
    
    // Find unit and update state
    uint8_t *target_mac = NULL;
    for (size_t i = 0; i < laser_unit_count; i++) {
        if (laser_units[i].module_id == module_id) {
            laser_units[i].laser_on = laser_on;
            target_mac = laser_units[i].mac_addr;
            break;
        }
    }
    
    if (!target_mac) {
        ESP_LOGE(TAG, "Laser unit %d not found", module_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    // Send unicast message to specific laser unit
    if (laser_on) {
        uint8_t data[1] = {intensity};
        return espnow_send_message(target_mac, MSG_LASER_ON, data, sizeof(data));
    } else {
        return espnow_send_message(target_mac, MSG_LASER_OFF, NULL, 0);
    }
}

/**
 * Reset laser unit
 */
esp_err_t game_reset_laser_unit(uint8_t module_id)
{
    ESP_LOGI(TAG, "Resetting laser unit %d", module_id);
    
    return espnow_broadcast_message(MSG_RESET, NULL, 0);
}

/**
 * Public function to update laser unit tracking (call from ESP-NOW callback)
 */
void game_update_laser_unit(uint8_t module_id, const uint8_t *mac_addr, int8_t rssi)
{
    update_laser_unit(module_id, mac_addr, rssi);
}

