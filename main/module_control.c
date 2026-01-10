/**
 * CONTROL Module - Main Unit Implementation
 * 
 * Handles main unit initialization and event callbacks
 * 
 * @author ninharp
 * @date 2026-01-09
 */

#include "module_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include <dirent.h>
#include <sys/stat.h>

// Component includes
#include "display_manager.h"
#include "game_logic.h"
#include "espnow_manager.h"
#include "wifi_ap_manager.h"
#include "web_server.h"
#include "button_handler.h"
#include "buzzer.h"
#ifdef CONFIG_ENABLE_SOUND_MANAGER
#include "sound_manager.h"
#endif
#include "sd_card_manager.h"

static const char *TAG = "MODULE_CTRL";

// Display update task
static TaskHandle_t display_update_task_handle = NULL;

// Heartbeat timer for sending periodic heartbeats to laser units
static esp_timer_handle_t heartbeat_timer = NULL;

// Last countdown value for buzzer triggering
static int last_countdown_value = -1;

#ifdef CONFIG_ENABLE_SD_CARD
/**
 * List directory contents on SD card
 */
static void list_sd_directory(const char *path, int max_files)
{
    DIR *dir = opendir(path);
    if (!dir) {
        ESP_LOGD(TAG, "    Directory not found: %s", path);
        return;
    }
    
    ESP_LOGI(TAG, "    Contents of %s:", path);
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL && count < max_files) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[300];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                ESP_LOGI(TAG, "      [DIR]  %s", entry->d_name);
            } else {
                ESP_LOGI(TAG, "      [FILE] %s (%ld bytes)", entry->d_name, st.st_size);
            }
            count++;
        }
    }
    
    closedir(dir);
    
    if (count == 0) {
        ESP_LOGI(TAG, "      (empty)");
    } else if (count >= max_files) {
        ESP_LOGI(TAG, "      ... (showing first %d entries)", max_files);
    }
}

/**
 * List SD card directory structure
 */
static void list_sd_card_structure(void)
{
    ESP_LOGI(TAG, "  SD Card Directory Structure:");
    
    // List root directory
    list_sd_directory("/sdcard", 10);
    
    // List common directories if they exist
    const char *important_dirs[] = {
        "/sdcard/web",
        "/sdcard/sounds",
        "/sdcard/logs",
        "/sdcard/config"
    };
    
    for (int i = 0; i < sizeof(important_dirs) / sizeof(important_dirs[0]); i++) {
        list_sd_directory(important_dirs[i], 10);
    }
}
#endif

/**
 * Heartbeat timer callback (Main Unit)
 * Sends periodic heartbeat to all laser units to keep safety timers alive
 */
static void heartbeat_timer_callback(void *arg)
{
    // Send heartbeat broadcast to all units
    espnow_broadcast_message(MSG_HEARTBEAT, NULL, 0);
    ESP_LOGD(TAG, "Heartbeat broadcast sent to all units");
}

/**
 * Display update task - Updates the display based on game state
 */
static void display_update_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Display update task started");
    
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t update_interval = pdMS_TO_TICKS(100); // Update every 100ms
    
    game_state_t last_state = GAME_STATE_IDLE;
    bool complete_screen_shown = false;
    uint32_t last_status_log = 0; // For periodic status logging
    
    while (1) {
        game_state_t state = game_get_state();
        player_data_t player_data;
        
        // Get laser units info
        laser_unit_info_t units[MAX_LASER_UNITS];
        size_t unit_count = 0;
        game_get_laser_units(units, MAX_LASER_UNITS, &unit_count);
        
        // Count online units
        size_t online_count = 0;
        for (size_t i = 0; i < unit_count; i++) {
            if (units[i].is_online) {
                online_count++;
            }
        }
        
        switch (state) {
            case GAME_STATE_IDLE:
                display_set_screen(SCREEN_IDLE);
                // Show welcome message with connected units
                display_clear();
                display_text("Laser Parcour", 0);
                display_text("Ready to Start", 2);
                char units_line[32];
                snprintf(units_line, sizeof(units_line), "Units: %d", online_count);
                display_text(units_line, 4);
                display_text("Start via Web", 6);
                display_update();
                complete_screen_shown = false;
                last_countdown_value = -1;  // Reset countdown tracking
                break;
                
            case GAME_STATE_COUNTDOWN:
                display_set_screen(SCREEN_GAME_COUNTDOWN);
                if (game_get_player_data(&player_data) == ESP_OK) {
                    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
                    uint32_t countdown_remaining = 0;
                    if (now < player_data.start_time) {
                        countdown_remaining = (player_data.start_time - now) / 1000;
                    }
                    display_countdown(countdown_remaining);
                    
                    // Play audio beep on each countdown tick
                    if (countdown_remaining > 0 && last_countdown_value != countdown_remaining) {
                        last_countdown_value = countdown_remaining;
                        sound_manager_play_event(SOUND_EVENT_COUNTDOWN, SOUND_MODE_ONCE);
                        ESP_LOGI(TAG, "Countdown beep: %lu", (unsigned long)countdown_remaining);
                    }
                }
                break;
                
            case GAME_STATE_RUNNING:
                display_set_screen(SCREEN_GAME_RUNNING);
                if (game_get_player_data(&player_data) == ESP_OK) {
                    display_game_status(player_data.elapsed_time, 
                                      player_data.beam_breaks);
                }
                break;
                
            case GAME_STATE_PENALTY:
                display_set_screen(SCREEN_GAME_PAUSED); // Reuse PAUSED screen for PENALTY
                if (game_get_player_data(&player_data) == ESP_OK) {
                    // Clear and show penalty message
                    display_clear();
                    display_text("*** PENALTY! ***", 0);
                    char time_str[32];
                    uint32_t minutes = player_data.elapsed_time / 60000;
                    uint32_t seconds = (player_data.elapsed_time % 60000) / 1000;
                    snprintf(time_str, sizeof(time_str), "Time: %02lu:%02lu", minutes, seconds);
                    display_text(time_str, 3);
                    char breaks_str[32];
                    snprintf(breaks_str, sizeof(breaks_str), "Breaks: %d", player_data.beam_breaks);
                    display_text(breaks_str, 5);
                    display_update();
                }
                break;
                
            case GAME_STATE_PAUSED:
                display_set_screen(SCREEN_GAME_PAUSED);
                if (game_get_player_data(&player_data) == ESP_OK) {
                    display_game_status(player_data.elapsed_time, 
                                      player_data.beam_breaks);
                }
                break;
                
            case GAME_STATE_COMPLETE:
                // Only display results once to avoid infinite logging
                if (!complete_screen_shown) {
                    display_set_screen(SCREEN_GAME_COMPLETE);
                    if (game_get_player_data(&player_data) == ESP_OK) {
                        display_game_results(player_data.elapsed_time,
                                           player_data.beam_breaks,
                                           player_data.completion);
                    }
                    complete_screen_shown = true;
                }
                break;
                
            default:
                break;
        }
        
        // Reset complete screen flag when leaving COMPLETE state
        if (last_state == GAME_STATE_COMPLETE && state != GAME_STATE_COMPLETE) {
            complete_screen_shown = false;
        }
        last_state = state;
        
        // Periodic status logging every 10 seconds during active game
        if (state == GAME_STATE_RUNNING || state == GAME_STATE_PENALTY || state == GAME_STATE_PAUSED) {
            uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
            if (now - last_status_log >= 10000) { // Every 10 seconds
                if (game_get_player_data(&player_data) == ESP_OK) {
                    uint32_t minutes = player_data.elapsed_time / 60000;
                    uint32_t seconds = (player_data.elapsed_time % 60000) / 1000;
                    const char* state_str = (state == GAME_STATE_RUNNING) ? "RUNNING" : 
                                           (state == GAME_STATE_PENALTY) ? "PENALTY" : "PAUSED";
                    ESP_LOGI(TAG, "=== GAME STATUS ===  State: %s | Time: %02lu:%02lu | Breaks: %d",
                            state_str, minutes, seconds, player_data.beam_breaks);
                }
                last_status_log = now;
            }
        } else {
            last_status_log = 0; // Reset when not in active game
        }
        
        // Wait for next update interval
        vTaskDelayUntil(&last_wake_time, update_interval);
    }
}

/**
 * Button event callback (Main Unit)
 */
#ifdef CONFIG_ENABLE_BUTTONS
static void button_event_callback(uint8_t button_id, button_event_t event)
{
    ESP_LOGI(TAG, "Button %d event: %d", button_id, event);
    
    if (event == BUTTON_EVENT_CLICK) {
        sound_manager_play_event(SOUND_EVENT_BUTTON_PRESS, SOUND_MODE_ONCE);
        
        switch (button_id) {
            case 0:  // Button 1 - Start/Stop
                ESP_LOGI(TAG, "Start/Stop button pressed");
                {
                    game_state_t state = game_get_state();
                    if (state == GAME_STATE_IDLE || state == GAME_STATE_COMPLETE) {
                        // Check for laser units first
                        if (!game_has_laser_units()) {
                            ESP_LOGW(TAG, "Cannot start game: No laser units connected");
                            display_clear();
                            display_text("ERROR:", 0);
                            display_text("No laser units", 2);
                            display_text("found!", 3);
                            display_text("Check units", 5);
                            display_update();
                            vTaskDelay(pdMS_TO_TICKS(5000));  // Show for 5 seconds
                            display_set_screen(SCREEN_IDLE);
                            display_update();
                            sound_manager_play_event(SOUND_EVENT_ERROR, SOUND_MODE_ONCE);
                            return;
                        }
                        
                        // Start game
                        ESP_LOGI(TAG, "Starting game...");
                        esp_err_t ret = game_start(GAME_MODE_SINGLE_SPEEDRUN, "Player");
                        if (ret == ESP_OK) {
                            sound_manager_play_event(SOUND_EVENT_GAME_START, SOUND_MODE_ONCE);
                            ESP_LOGI(TAG, "Game started successfully");
                        } else {
                            sound_manager_play_event(SOUND_EVENT_ERROR, SOUND_MODE_ONCE);
                            ESP_LOGE(TAG, "Failed to start game: %s", esp_err_to_name(ret));
                        }
                    } else if (state == GAME_STATE_RUNNING || state == GAME_STATE_PENALTY) {
                        // Stop game
                        ESP_LOGI(TAG, "Stopping game...");
                        esp_err_t ret = game_stop();
                        if (ret == ESP_OK) {
                            sound_manager_play_event(SOUND_EVENT_GAME_STOP, SOUND_MODE_ONCE);
                            ESP_LOGI(TAG, "Game stopped");
                        } else {
                            sound_manager_play_event(SOUND_EVENT_ERROR, SOUND_MODE_ONCE);
                            ESP_LOGE(TAG, "Failed to stop game: %s", esp_err_to_name(ret));
                        }
                    } else if (state == GAME_STATE_PAUSED) {
                        // Resume game
                        ESP_LOGI(TAG, "Resuming game...");
                        esp_err_t ret = game_resume();
                        if (ret == ESP_OK) {
                            ESP_LOGI(TAG, "Game resumed");
                        } else {
                            sound_manager_play_event(SOUND_EVENT_ERROR, SOUND_MODE_ONCE);
                            ESP_LOGE(TAG, "Failed to resume game: %s", esp_err_to_name(ret));
                        }
                    }
                }
                break;
                
            case 1:  // Button 2 - Reset/Stop
                ESP_LOGI(TAG, "Reset button pressed");
                {
                    game_state_t state = game_get_state();
                    if (state == GAME_STATE_RUNNING || state == GAME_STATE_PENALTY || state == GAME_STATE_PAUSED) {
                        ESP_LOGI(TAG, "Stopping game...");
                        esp_err_t ret = game_stop();
                        if (ret == ESP_OK) {
                            sound_manager_play_event(SOUND_EVENT_GAME_STOP, SOUND_MODE_ONCE);
                            ESP_LOGI(TAG, "Game stopped and reset");
                        } else {
                            sound_manager_play_event(SOUND_EVENT_ERROR, SOUND_MODE_ONCE);
                            ESP_LOGE(TAG, "Failed to stop game: %s", esp_err_to_name(ret));
                        }
                    }
                    // Always reset display to idle screen
                    display_set_screen(SCREEN_IDLE);
                    display_update();
                }
                break;
                
            case 2:  // Button 3 - Debug Finish Button
#ifdef CONFIG_ENABLE_BUTTON3_DEBUG_FINISH
                {
                    game_state_t state = game_get_state();
                    if (state == GAME_STATE_RUNNING || state == GAME_STATE_PENALTY) {
                        ESP_LOGI(TAG, "Debug Finish button pressed - triggering game finish");
                        esp_err_t ret = game_finish();
                        if (ret == ESP_OK) {
                            sound_manager_play_event(SOUND_EVENT_SUCCESS, SOUND_MODE_ONCE);
                            ESP_LOGI(TAG, "Game finished (debug)");
                        } else {
                            sound_manager_play_event(SOUND_EVENT_ERROR, SOUND_MODE_ONCE);
                            ESP_LOGE(TAG, "Failed to finish game: %s", esp_err_to_name(ret));
                        }
                    } else {
                        ESP_LOGW(TAG, "Debug Finish button pressed but game not running (state: %d)", state);
                    }
                }
#else
                ESP_LOGI(TAG, "Button 3 pressed (debug finish disabled)");
#endif
                break;
        }
    } else if (event == BUTTON_EVENT_LONG_PRESS) {
        // Long press on Button 1: Turn all lasers ON/OFF
        if (button_id == 0) {
            ESP_LOGI(TAG, "Long press on Start/Stop button - toggling all lasers");
            sound_manager_play_event(SOUND_EVENT_BUTTON_PRESS, SOUND_MODE_ONCE);
            
            // Get all laser units
            laser_unit_info_t units[MAX_LASER_UNITS];
            size_t unit_count = 0;
            game_get_laser_units(units, MAX_LASER_UNITS, &unit_count);
            
            // Toggle laser state (if any laser is on, turn all off; otherwise turn all on)
            bool any_laser_on = false;
            for (size_t i = 0; i < unit_count; i++) {
                if (units[i].laser_on && units[i].role == 1) {  // Only check laser units
                    any_laser_on = true;
                    break;
                }
            }
            
            // Send command to all laser units
            for (size_t i = 0; i < unit_count; i++) {
                if (units[i].role == 1) {  // Only control laser units
                    if (any_laser_on) {
                        game_control_laser(units[i].module_id, false, 0);  // Turn off
                    } else {
                        game_control_laser(units[i].module_id, true, 100);  // Turn on
                    }
                }
            }
            
            ESP_LOGI(TAG, "All lasers turned %s", any_laser_on ? "OFF" : "ON");
        }
    }
}
#endif

/**
 * Web server game control callback
 */
static esp_err_t game_control_callback(const char *command, const char *data)
{
    ESP_LOGI(TAG, "Game control from web: %s", command);
    
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    
    if (strcmp(command, "start") == 0) {
        ret = game_start(GAME_MODE_SINGLE_SPEEDRUN, "Web Player");
        if (ret == ESP_OK) {
            sound_manager_play_event(SOUND_EVENT_GAME_START, SOUND_MODE_ONCE);
        } else if (ret == ESP_ERR_INVALID_STATE) {
            // Show error on display if no laser units
            display_clear();
            display_text("ERROR:", 0);
            display_text("No laser units", 2);
            display_text("found!", 3);
            display_text("Check web UI", 5);
            display_update();
            vTaskDelay(pdMS_TO_TICKS(5000));  // Show for 5 seconds
            display_set_screen(SCREEN_IDLE);
            display_update();
            sound_manager_play_event(SOUND_EVENT_ERROR, SOUND_MODE_ONCE);
        }
    } else if (strcmp(command, "stop") == 0) {
        #ifdef CONFIG_ENABLE_SOUND_MANAGER
        sound_manager_play_event(SOUND_EVENT_GAME_STOP, SOUND_MODE_ONCE);
        #endif
        ret = game_stop();
    } else if (strcmp(command, "pause") == 0) {
        ret = game_pause();
    } else if (strcmp(command, "resume") == 0) {
        ret = game_resume();
    }
    
    // Update web interface with new game state
    if (ret == ESP_OK) {
        player_data_t player_data;
        if (game_get_player_data(&player_data) == ESP_OK) {
            // TODO: Update web status properly
        }
    }
    
    return ret;
}

/**
 * ESP-NOW message received callback (Main Unit)
 */
static void espnow_recv_callback_main(const uint8_t *sender_mac, const espnow_message_t *message)
{
    ESP_LOGI(TAG, "ESP-NOW message received from %02X:%02X:%02X:%02X:%02X:%02X",
             sender_mac[0], sender_mac[1], sender_mac[2], 
             sender_mac[3], sender_mac[4], sender_mac[5]);
    
    // Update laser unit tracking (role=0 means "keep existing role")
    game_update_laser_unit(message->module_id, sender_mac, -50, 0);
    
    switch (message->msg_type) {
        case MSG_BEAM_BROKEN:
            ESP_LOGW(TAG, "Beam broken on module %d!", message->module_id);
            game_beam_broken(message->module_id);
            break;
        case MSG_FINISH_PRESSED:
            ESP_LOGI(TAG, "Finish button pressed on module %d - completing game!", message->module_id);
            game_finish();  // Successful completion via finish button
            break;
        case MSG_HEARTBEAT:
            // Ensure the laser unit is in the ESP-NOW peer list
            {
                esp_err_t peer_ret = espnow_add_peer(sender_mac, message->module_id, 1);
                if (peer_ret == ESP_OK) {
                    // Only log when peer is actually added (e.g., after main unit restart)
                    ESP_LOGI(TAG, "Laser unit %d re-added as ESP-NOW peer", message->module_id);
                } else if (peer_ret != ESP_ERR_ESPNOW_EXIST) {
                    ESP_LOGE(TAG, "Failed to add peer during heartbeat: %s", esp_err_to_name(peer_ret));
                }
                // Silent when peer already exists - no need to log every 3 seconds
            }
            break;
        case MSG_STATUS_UPDATE:
            ESP_LOGD(TAG, "Status update from module %d", message->module_id);
            break;
        case MSG_PAIRING_REQUEST:
            ESP_LOGI(TAG, "Pairing request from module %d", message->module_id);
            
            // Extract role from message data (default to 1 for laser if not specified)
            // data[0] will be 1 (laser) or 2 (finish button), default to 1 for backward compatibility
            {
                uint8_t peer_role = (message->data[0] == 2) ? 2 : 1;
                const char *role_name = (peer_role == 2) ? "Finish Button" : "Laser Unit";
                
                // Update laser unit tracking with role information
                game_update_laser_unit(message->module_id, sender_mac, -50, peer_role);
                
                // Add the unit as an ESP-NOW peer
                esp_err_t ret = espnow_add_peer(sender_mac, message->module_id, peer_role);
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "%s %d added as peer", role_name, message->module_id);
                } else if (ret == ESP_ERR_ESPNOW_EXIST) {
                    ESP_LOGD(TAG, "Peer already exists for module %d", message->module_id);
                } else {
                    ESP_LOGE(TAG, "Failed to add peer: %s", esp_err_to_name(ret));
                }
                
                // Send pairing response back to the unit
                ret = espnow_send_message(sender_mac, MSG_PAIRING_RESPONSE, NULL, 0);
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "Pairing response sent to %s %d", role_name, message->module_id);
                } else {
                    ESP_LOGE(TAG, "Failed to send pairing response: %s", esp_err_to_name(ret));
                }
            }
            break;
        case MSG_CHANNEL_ACK:
            ESP_LOGD(TAG, "Channel change ACK from module %d", message->module_id);
            break;
        default:
            ESP_LOGW(TAG, "Unknown message type: 0x%02X", message->msg_type);
            break;
    }
}

/**
 * Initialize the main control unit
 */
void module_control_init(void)
{
    ESP_LOGI(TAG, "Initializing Main Unit...");
    
#ifdef CONFIG_ENABLE_DISPLAY
    // Initialize OLED display
    if (CONFIG_I2C_SDA_PIN != -1 && CONFIG_I2C_SCL_PIN != -1) {
        ESP_LOGI(TAG, "  Initializing OLED Display (I2C SDA:%d SCL:%d)", 
                 CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN);
        esp_err_t display_ret = display_manager_init(CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN, CONFIG_I2C_FREQUENCY);
        if (display_ret == ESP_OK) {
            display_set_screen(SCREEN_IDLE);
        } else {
            ESP_LOGW(TAG, "  Display initialization failed, continuing without display");
        }
    } else {
        ESP_LOGI(TAG, "  Display disabled (pin = -1)");
    }
#else
    ESP_LOGI(TAG, "  Display disabled in menuconfig");
#endif

#ifdef CONFIG_ENABLE_BUTTONS
    // Initialize buttons
    button_config_t buttons[3] = {
        {.pin = CONFIG_BUTTON1_PIN, .debounce_time_ms = CONFIG_DEBOUNCE_TIME, .long_press_time_ms = 1000, .pull_up = true, .active_low = true},
        {.pin = CONFIG_BUTTON2_PIN, .debounce_time_ms = CONFIG_DEBOUNCE_TIME, .long_press_time_ms = 1000, .pull_up = true, .active_low = true},
        {.pin = CONFIG_BUTTON3_PIN, .debounce_time_ms = CONFIG_DEBOUNCE_TIME, .long_press_time_ms = 1000, .pull_up = true, .active_low = true}
    };
    
    // Count enabled buttons
    uint8_t num_buttons = 0;
    for (int i = 0; i < 3; i++) {
        if (buttons[i].pin != -1) num_buttons++;
    }
    
    if (num_buttons > 0) {
        ESP_LOGI(TAG, "  Initializing %d buttons", num_buttons);
        ESP_ERROR_CHECK(button_handler_init(buttons, 4, button_event_callback));
    } else {
        ESP_LOGI(TAG, "  Buttons disabled (all pins = -1)");
    }
#else
    ESP_LOGI(TAG, "  Buttons disabled in menuconfig");
#endif

#ifdef CONFIG_ENABLE_BUZZER
    // Initialize buzzer (optional, errors are non-fatal)
    ESP_LOGI(TAG, "  Initializing Buzzer (GPIO %d)", CONFIG_BUZZER_PIN);
    esp_err_t buzz_ret = buzzer_init(CONFIG_BUZZER_PIN);
    if (buzz_ret == ESP_OK) {
        ESP_LOGI(TAG, "  Buzzer initialized successfully");
        buzzer_set_volume(50); // 50% volume
        sound_manager_play_event(SOUND_EVENT_SUCCESS, SOUND_MODE_ONCE); // Startup sound
    } else {
        ESP_LOGW(TAG, "  Buzzer initialization failed (continuing without buzzer)");
    }
#else
    ESP_LOGI(TAG, "  Buzzer disabled in menuconfig");
#endif

#ifdef CONFIG_ENABLE_SD_CARD
    // Initialize SD Card (optional, errors are non-fatal)
    ESP_LOGI(TAG, "  Initializing SD Card...");
    esp_err_t sd_ret = sd_card_manager_init(NULL);  // NULL = use menuconfig pins
    if (sd_ret == ESP_OK) {
        sd_card_info_t sd_info;
        if (sd_card_get_info(&sd_info) == ESP_OK) {
            ESP_LOGI(TAG, "  SD Card mounted: %llu MB total, %llu MB free",
                     sd_info.total_bytes / (1024*1024),
                     sd_info.free_bytes / (1024*1024));
            ESP_LOGI(TAG, "  Card Type: %s", sd_info.card_type);
            
            if (sd_info.web_dir_available) {
                ESP_LOGI(TAG, "  Web interface available on SD card");
            } else {
                ESP_LOGI(TAG, "  No /web directory on SD card, using internal interface");
            }
            
            // List SD card directory structure
            list_sd_card_structure();
        }
        
#ifdef CONFIG_ENABLE_SOUND_MANAGER
        // Initialize Sound Manager (requires SD card for audio files)
        ESP_LOGI(TAG, "  Initializing Sound Manager (I2S Audio)...");
        esp_err_t sound_ret = sound_manager_init(NULL);  // NULL = use menuconfig settings
        if (sound_ret == ESP_OK) {
            ESP_LOGI(TAG, "  Sound Manager initialized - audio playback enabled");
            //sound_manager_play_event(SOUND_EVENT_STARTUP, SOUND_MODE_ONCE);
        } else {
            ESP_LOGW(TAG, "  Sound Manager initialization failed, using buzzer fallback");
        }
#endif
    } else {
        ESP_LOGW(TAG, "  SD Card initialization failed (continuing without SD card)");
    }
#else
    ESP_LOGI(TAG, "  SD Card support disabled in menuconfig");
#endif
    
    // Initialize WiFi (required for ESP-NOW and web server)
    ESP_LOGI(TAG, "  Initializing WiFi in APSTA mode");
    ESP_ERROR_CHECK(wifi_apsta_init());
    ESP_LOGI(TAG, "  WiFi started in APSTA mode with STA and AP netif");
    
    // Initialize WiFi with automatic fallback
    // Try to connect to saved WiFi, if fails -> start AP mode
    ESP_LOGI(TAG, "  Initializing WiFi with automatic fallback...");
    laser_ap_config_t ap_config = {
        .channel = CONFIG_WIFI_CHANNEL,
        .max_connection = CONFIG_MAX_STA_CONN
    };
    strncpy(ap_config.ssid, CONFIG_WIFI_SSID, sizeof(ap_config.ssid) - 1);
    strncpy(ap_config.password, CONFIG_WIFI_PASSWORD, sizeof(ap_config.password) - 1);
    
    esp_err_t wifi_ret = wifi_connect_with_fallback(&ap_config, 10000);
    if (wifi_ret == ESP_OK) {
        ESP_LOGI(TAG, "  Connected to saved WiFi network");
        esp_netif_ip_info_t ip_info;
        if (wifi_get_sta_ip(&ip_info) == ESP_OK) {
            ESP_LOGI(TAG, "  WiFi STA IP: " IPSTR, IP2STR(&ip_info.ip));
        }
        
        #ifdef CONFIG_ENABLE_SOUND_MANAGER
        // Start HTTP streaming now that WiFi is connected
        ESP_LOGI(TAG, "  Starting audio streaming (WiFi connected)...");
        esp_err_t stream_ret = sound_manager_start_streaming();
        if (stream_ret == ESP_OK) {
            ESP_LOGI(TAG, "  Audio streaming started");
        } else {
            ESP_LOGW(TAG, "  Failed to start audio streaming: %s", esp_err_to_name(stream_ret));
        }
        #endif
    } else {
        ESP_LOGI(TAG, "  Running in AP mode (Fallback): http://192.168.4.1");
    }
    
    // Initialize web server
    ESP_LOGI(TAG, "  Initializing Web Server (http://192.168.4.1)");
    ESP_ERROR_CHECK(web_server_init(NULL, game_control_callback));
    
    // Initialize ESP-NOW
    ESP_LOGI(TAG, "  Initializing ESP-NOW (Channel: %d)", CONFIG_ESPNOW_CHANNEL);
    ESP_ERROR_CHECK(espnow_manager_init(CONFIG_ESPNOW_CHANNEL, espnow_recv_callback_main));
    
    // Set up heartbeat timer (5 seconds - to keep laser unit safety timers alive)
    ESP_LOGI(TAG, "  Setting up heartbeat timer");
    const esp_timer_create_args_t heartbeat_timer_args = {
        .callback = &heartbeat_timer_callback,
        .name = "heartbeat_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&heartbeat_timer_args, &heartbeat_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(heartbeat_timer, 5000000));  // 5 seconds
    
    // Update all peers with current WiFi channel (in case we connected to external WiFi)
    uint8_t actual_channel;
    wifi_second_chan_t second;
    esp_wifi_get_channel(&actual_channel, &second);
    if (actual_channel != CONFIG_ESPNOW_CHANNEL) {
        ESP_LOGI(TAG, "  Updating existing peers from channel %d to %d", CONFIG_ESPNOW_CHANNEL, actual_channel);
        espnow_update_all_peers_channel(actual_channel);
    }
    
    // Initialize game logic
    ESP_LOGI(TAG, "  Initializing Game Logic");
    ESP_ERROR_CHECK(game_logic_init());
    
#ifdef CONFIG_ENABLE_DISPLAY
    // Start display update task
    if (CONFIG_I2C_SDA_PIN != -1 && CONFIG_I2C_SCL_PIN != -1) {
        ESP_LOGI(TAG, "  Starting display update task");
        xTaskCreate(display_update_task, "display_update", 4096, NULL, 5, &display_update_task_handle);
    }
#endif
    
    // Broadcast reset to trigger re-pairing of existing laser units
    ESP_LOGI(TAG, "  Broadcasting reset to all units for re-pairing");
    vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second for ESP-NOW to be fully ready
    espnow_broadcast_message(MSG_RESET, NULL, 0);
    
    // Print GPIO configuration summary
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "   Main Unit - GPIO Configuration");
    ESP_LOGI(TAG, "=================================================");
#ifdef CONFIG_ENABLE_DISPLAY
    if (CONFIG_I2C_SDA_PIN != -1 && CONFIG_I2C_SCL_PIN != -1) {
        ESP_LOGI(TAG, "Display I2C:    SDA=GPIO%d, SCL=GPIO%d", CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN);
    } else {
        ESP_LOGI(TAG, "Display:        Disabled");
    }
#else
    ESP_LOGI(TAG, "Display:        Disabled (menuconfig)");
#endif
#ifdef CONFIG_ENABLE_BUTTONS
    ESP_LOGI(TAG, "Buttons:        B1=GPIO%d, B2=GPIO%d, B3=GPIO%d",
             CONFIG_BUTTON1_PIN, CONFIG_BUTTON2_PIN, CONFIG_BUTTON3_PIN);
#else
    ESP_LOGI(TAG, "Buttons:        Disabled (menuconfig)");
#endif
#ifdef CONFIG_ENABLE_BUZZER
    ESP_LOGI(TAG, "Buzzer:         GPIO%d", CONFIG_BUZZER_PIN);
#else
    ESP_LOGI(TAG, "Buzzer:         Disabled (menuconfig)");
#endif
#ifdef CONFIG_ENABLE_SOUND_MANAGER
    ESP_LOGI(TAG, "I2S Audio:      BCK=GPIO%d, WS=GPIO%d, DOUT=GPIO%d",
             CONFIG_I2S_BCK_PIN, CONFIG_I2S_WS_PIN, CONFIG_I2S_DATA_OUT_PIN);
#else
    ESP_LOGI(TAG, "I2S Audio:      Disabled (menuconfig)");
#endif
    ESP_LOGI(TAG, "WiFi Channel:   %d", CONFIG_WIFI_CHANNEL);
    ESP_LOGI(TAG, "ESP-NOW Ch:     %d", CONFIG_ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "=================================================");
    
    ESP_LOGI(TAG, "Main Unit initialized - ready to coordinate game");
}

/**
 * Run the main control unit loop
 */
void module_control_run(void)
{
    // Main loop for control module
    while (1) {
        // Get connected laser units count
        laser_unit_info_t units[MAX_LASER_UNITS];
        size_t unit_count = 0;
        game_get_laser_units(units, MAX_LASER_UNITS, &unit_count);
        
        // Count online units
        size_t online_count = 0;
        for (size_t i = 0; i < unit_count; i++) {
            if (units[i].is_online) {
                online_count++;
            }
        }
        
        // Build connected units string
        char units_str[128] = {0};
        if (online_count > 0) {
            char *p = units_str;
            p += snprintf(p, sizeof(units_str), " | Connected units: %d [", online_count);
            bool first = true;
            for (size_t i = 0; i < unit_count; i++) {
                if (units[i].is_online) {
                    if (!first) {
                        p += snprintf(p, sizeof(units_str) - (p - units_str), ", ");
                    }
                    p += snprintf(p, sizeof(units_str) - (p - units_str), "%d", units[i].module_id);
                    first = false;
                }
            }
            snprintf(p, sizeof(units_str) - (p - units_str), "]");
        } else {
            snprintf(units_str, sizeof(units_str), " | No units connected");
        }
        
        ESP_LOGI(TAG, "Status: Running - Free heap: %ld bytes%s",
                 esp_get_free_heap_size(), units_str);
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
