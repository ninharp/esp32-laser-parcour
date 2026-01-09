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
    .max_time = 0,              // No time limit (0 = unlimited)
    .penalty_time = 15,         // 15 seconds penalty per beam break
    .countdown_time = 5,        // 5 second countdown
    .max_players = 8
};

// Penalty tracking
static uint32_t penalty_start_time = 0;
static uint32_t total_penalty_time = 0;  // Accumulated penalty time in ms
#define PENALTY_DISPLAY_TIME_MS 3000  // Show PENALTY state for 3 seconds

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
    
    // Check if at least one laser unit is online
    if (!game_has_laser_units()) {
        ESP_LOGW(TAG, "Cannot start game: No laser units online");
        xSemaphoreGive(game_mutex);
        return ESP_ERR_INVALID_STATE;
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
    current_player.completion = COMPLETION_NONE;
    
    // Reset penalty tracking
    penalty_start_time = 0;
    total_penalty_time = 0;  // Total penalty seconds ADDED to elapsed time
    
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
 * Finish game via finish button (successful completion)
 */
esp_err_t game_finish(void)
{
    if (xSemaphoreTake(game_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return ESP_FAIL;
    }
    
    if (current_state != GAME_STATE_RUNNING && current_state != GAME_STATE_PENALTY) {
        ESP_LOGW(TAG, "Cannot finish game - not running (state: %d)", current_state);
        xSemaphoreGive(game_mutex);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Finishing game via finish button...");
    
    // Record end time
    current_player.end_time = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t raw_elapsed = current_player.end_time - current_player.start_time;
    
    // ADD accumulated penalty time to final elapsed time (wurde bereits bei Beam-Breaks addiert)
    current_player.elapsed_time = raw_elapsed + total_penalty_time;
    
    // Set completion status to SOLVED (finished via button)
    current_player.completion = COMPLETION_SOLVED;
    
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
    penalty_start_time = 0;
    
    ESP_LOGI(TAG, "Game finished successfully! Time: %lu ms, Breaks: %d, Completion: SOLVED",
             current_player.elapsed_time, current_player.beam_breaks);
    
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
    
    return ESP_OK;
}

/**
 * Stop the current game (abort/cancel)
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
    uint32_t raw_elapsed = current_player.end_time - current_player.start_time;
    
    // ADD accumulated penalty time to final elapsed time (wurde bereits bei Beam-Breaks addiert)
    current_player.elapsed_time = raw_elapsed + total_penalty_time;
    
    // Set completion status if not already set
    if (current_player.completion == COMPLETION_NONE) {
        current_player.completion = COMPLETION_ABORTED_MANUAL;
    }
    
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
    
    ESP_LOGI(TAG, "Game stopped - Time: %lu ms, Beam Breaks: %d, Completion: %d",
             current_player.elapsed_time, current_player.beam_breaks, current_player.completion);
    
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
    
    // Only accept beam breaks in RUNNING state (not during PENALTY display)
    if (current_state != GAME_STATE_RUNNING) {
        xSemaphoreGive(game_mutex);
        return ESP_FAIL;
    }
    
    // Increment beam break counter
    current_player.beam_breaks++;
    
    // Enter penalty display state (unless in training mode)
    if (configuration.mode != GAME_MODE_TRAINING) {
        current_state = GAME_STATE_PENALTY;
        penalty_start_time = (uint32_t)(esp_timer_get_time() / 1000);  // Start penalty display timer
        
        // SOFORT die volle Penalty-Zeit zur Gesamtzeit addieren
        uint32_t penalty_duration_ms = configuration.penalty_time * 1000;
        total_penalty_time += penalty_duration_ms;
        
        ESP_LOGI(TAG, "Beam broken! Sensor: %d, Total breaks: %d, Penalty: %lu seconds (added immediately, showing penalty for 3s)", 
                 sensor_id, current_player.beam_breaks, configuration.penalty_time);
    } else {
        ESP_LOGI(TAG, "Beam broken! Sensor: %d, Total breaks: %d (Training mode - no penalty)", 
                 sensor_id, current_player.beam_breaks);
    }
    
    xSemaphoreGive(game_mutex);
    
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
    
    // Check if penalty display period has expired and return to RUNNING
    if (current_state == GAME_STATE_PENALTY && penalty_start_time > 0) {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
        uint32_t penalty_elapsed = now - penalty_start_time;
        
        if (penalty_elapsed >= PENALTY_DISPLAY_TIME_MS) {
            // Penalty display over (3s), return to RUNNING (Zeit wurde bereits bei Beam-Break addiert)
            penalty_start_time = 0;
            current_state = GAME_STATE_RUNNING;
            ESP_LOGI(TAG, "Penalty display ended, returning to RUNNING state");
        }
    }
    
    memcpy(player_data, &current_player, sizeof(player_data_t));
    
    // Calculate elapsed time for running games
    if (current_state == GAME_STATE_RUNNING || current_state == GAME_STATE_PENALTY) {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
        // ADD penalty times to elapsed time (penalty wurde bereits sofort bei Beam-Break addiert)
        uint32_t raw_elapsed = now - current_player.start_time;
        player_data->elapsed_time = raw_elapsed + total_penalty_time;
        
        // Check for max time limit (if configured) and auto-stop
        if (configuration.max_time > 0 && player_data->elapsed_time >= (configuration.max_time * 1000)) {
            // Max time exceeded - abort game automatically
            ESP_LOGW(TAG, "Max time limit reached (%lu seconds) - auto-stopping game", configuration.max_time);
            
            // Set completion status BEFORE releasing mutex and calling game_stop
            current_player.completion = COMPLETION_ABORTED_TIME;
            
            // Release mutex before calling game_stop (which will acquire it again)
            xSemaphoreGive(game_mutex);
            
            // Auto-stop the game
            game_stop();
            
            // Return early - game_stop already set final times
            return ESP_OK;
        }
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
static void update_laser_unit(uint8_t module_id, const uint8_t *mac_addr, int8_t rssi, uint8_t role)
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
            // Update role if provided (non-zero)
            if (role != 0) {
                laser_units[i].role = role;
                ESP_LOGD(TAG, "Updated role for unit %d to %d", module_id, role);
            }
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
        
        // If role is 0 (unknown), assume it's a laser unit (role=1)
        // This happens when heartbeat arrives before pairing request
        if (role == 0) {
            laser_units[laser_unit_count].role = 1;  // Default to laser unit
            ESP_LOGW(TAG, "New unit %d added with default role=1 (laser)", module_id);
        } else {
            laser_units[laser_unit_count].role = role;
        }
        
        snprintf(laser_units[laser_unit_count].status, sizeof(laser_units[laser_unit_count].status), "Active");
        laser_unit_count++;
        const char *role_name = (role == 2) ? "Finish Button" : (role == 1 || role == 0) ? "Laser Unit" : "Unknown";
        ESP_LOGI(TAG, "New %s registered: ID %d (role=%d)", role_name, module_id, laser_units[laser_unit_count-1].role);
    }
}

/**
 * Check if any laser units are online
 */
bool game_has_laser_units(void)
{
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    
    ESP_LOGI(TAG, "Checking for laser units: total units=%zu", laser_unit_count);
    
    // Check if at least one laser unit (role=1) is online
    for (size_t i = 0; i < laser_unit_count; i++) {
        uint32_t time_since_last_seen = now - laser_units[i].last_seen;
        
        ESP_LOGI(TAG, "Unit %d: role=%d, last_seen=%lums ago, online=%d", 
                 laser_units[i].module_id, 
                 laser_units[i].role,
                 time_since_last_seen,
                 laser_units[i].is_online);
        
        // Unit is online if seen within last 15 seconds AND is a laser unit (role=1)
        if (time_since_last_seen <= 15000 && laser_units[i].role == 1) {
            ESP_LOGI(TAG, "Found online laser unit %d", laser_units[i].module_id);
            return true;
        }
    }
    
    ESP_LOGW(TAG, "No online laser units found!");
    return false;
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
void game_update_laser_unit(uint8_t module_id, const uint8_t *mac_addr, int8_t rssi, uint8_t role)
{
    update_laser_unit(module_id, mac_addr, rssi, role);
}

