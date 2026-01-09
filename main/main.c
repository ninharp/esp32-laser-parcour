/**
 * ESP32 Laser Obstacle Course - Main Application
 * 
 * This is a modular system that can be configured as Main Unit or Laser Unit
 * via menuconfig. Each module type has different initialization and behavior.
 * 
 * @author ninharp
 * @date 2025
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_chip_info.h"
#include "esp_timer.h"

// Component includes
#include "display_manager.h"
#include "game_logic.h"
#include "espnow_manager.h"
#include "laser_control.h"
#include "sensor_manager.h"
#include "wifi_ap_manager.h"
#include "button_handler.h"
#include "buzzer.h"
#include "web_server.h"

#ifdef CONFIG_ENABLE_SD_CARD
#include "sd_card_manager.h"
#endif

static const char *TAG = "LASER_PARCOUR";

// Module role from Kconfig
#ifdef CONFIG_MODULE_ROLE_CONTROL
    #define MODULE_ROLE "MAIN_UNIT"
    #define IS_CONTROL_MODULE 1
#elif defined(CONFIG_MODULE_ROLE_LASER)
    #define MODULE_ROLE "LASER_UNIT"
    #define IS_LASER_MODULE 1
#elif defined(CONFIG_MODULE_ROLE_FINISH)
    #define MODULE_ROLE "FINISH_BUTTON"
    #define IS_FINISH_MODULE 1
#else
    #error "Module role not defined! Please run 'idf.py menuconfig' and select a module role."
#endif

/**
 * Initialize NVS (Non-Volatile Storage)
 * Required for WiFi and configuration storage
 */
static void init_nvs(void)
{
    ESP_LOGI(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized successfully");
}

/**
 * Initialize networking stack
 * Required for WiFi and ESP-NOW
 */
static void init_network(void)
{
    ESP_LOGI(TAG, "Initializing network stack...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "Network stack initialized");
}

/**
 * Print system information
 */
static void print_system_info(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "   ESP32 Laser Obstacle Course System");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Module Role:    %s", MODULE_ROLE);
    ESP_LOGI(TAG, "Module ID:      %d", CONFIG_MODULE_ID);
    ESP_LOGI(TAG, "Device Name:    %s", CONFIG_DEVICE_NAME);
    ESP_LOGI(TAG, "ESP-IDF:        %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Chip:           ESP32-C3 (rev %d)", chip_info.revision);
    ESP_LOGI(TAG, "Cores:          %d", chip_info.cores);
    ESP_LOGI(TAG, "Flash:          %s", 
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    ESP_LOGI(TAG, "Free Heap:      %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "=================================================");
}

/**
 * Notify all ESP-NOW peers about channel change
 * Called by wifi_ap_manager before connecting to STA
 */
esp_err_t notify_channel_change(uint8_t new_channel)
{
    ESP_LOGI(TAG, "Notifying all ESP-NOW peers about channel change to %d", new_channel);
    
    // Broadcast channel change via ESP-NOW
    esp_err_t ret = espnow_broadcast_channel_change(new_channel, 2000); // 2 second timeout
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Channel change notification sent successfully");
    } else {
        ESP_LOGW(TAG, "Failed to send channel change notification: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

#ifdef CONFIG_MODULE_ROLE_CONTROL
// Display update task handle
static TaskHandle_t display_update_task_handle = NULL;

/**
 * Display update task (Main Unit)
 * Periodically updates the OLED display with current game status
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
        buzzer_play_pattern(BUZZER_PATTERN_BEEP);
        
        switch (button_id) {
            case 0:  // Button 1 - Start/Stop
                ESP_LOGI(TAG, "Start/Stop button pressed");
                {
                    game_state_t state = game_get_state();
                    if (state == GAME_STATE_IDLE || state == GAME_STATE_COMPLETE) {
                        // Start game
                        ESP_LOGI(TAG, "Starting game...");
                        esp_err_t ret = game_start(GAME_MODE_SINGLE_SPEEDRUN, "Player");
                        if (ret == ESP_OK) {
                            buzzer_play_pattern(BUZZER_PATTERN_GAME_START);
                            ESP_LOGI(TAG, "Game started successfully");
                        } else {
                            buzzer_play_pattern(BUZZER_PATTERN_ERROR);
                            ESP_LOGE(TAG, "Failed to start game: %s", esp_err_to_name(ret));
                        }
                    } else if (state == GAME_STATE_RUNNING || state == GAME_STATE_PENALTY) {
                        // Stop game
                        ESP_LOGI(TAG, "Stopping game...");
                        esp_err_t ret = game_stop();
                        if (ret == ESP_OK) {
                            buzzer_play_pattern(BUZZER_PATTERN_GAME_END);
                            ESP_LOGI(TAG, "Game stopped");
                        } else {
                            buzzer_play_pattern(BUZZER_PATTERN_ERROR);
                            ESP_LOGE(TAG, "Failed to stop game: %s", esp_err_to_name(ret));
                        }
                    } else if (state == GAME_STATE_PAUSED) {
                        // Resume game
                        ESP_LOGI(TAG, "Resuming game...");
                        esp_err_t ret = game_resume();
                        if (ret == ESP_OK) {
                            ESP_LOGI(TAG, "Game resumed");
                        } else {
                            buzzer_play_pattern(BUZZER_PATTERN_ERROR);
                            ESP_LOGE(TAG, "Failed to resume game: %s", esp_err_to_name(ret));
                        }
                    }
                }
                break;
            case 1:  // Button 2 - Pause/Resume
                ESP_LOGI(TAG, "Pause/Resume button pressed");
                {
                    game_state_t state = game_get_state();
                    if (state == GAME_STATE_RUNNING || state == GAME_STATE_PENALTY) {
                        ESP_LOGI(TAG, "Pausing game...");
                        esp_err_t ret = game_pause();
                        if (ret == ESP_OK) {
                            ESP_LOGI(TAG, "Game paused");
                        } else {
                            buzzer_play_pattern(BUZZER_PATTERN_ERROR);
                            ESP_LOGE(TAG, "Failed to pause game: %s", esp_err_to_name(ret));
                        }
                    } else if (state == GAME_STATE_PAUSED) {
                        ESP_LOGI(TAG, "Resuming game...");
                        esp_err_t ret = game_resume();
                        if (ret == ESP_OK) {
                            ESP_LOGI(TAG, "Game resumed");
                        } else {
                            buzzer_play_pattern(BUZZER_PATTERN_ERROR);
                            ESP_LOGE(TAG, "Failed to resume game: %s", esp_err_to_name(ret));
                        }
                    }
                }
                break;
            case 2:  // Button 3 - Reset/Stop
                ESP_LOGI(TAG, "Reset button pressed");
                {
                    game_state_t state = game_get_state();
                    if (state == GAME_STATE_RUNNING || state == GAME_STATE_PENALTY || state == GAME_STATE_PAUSED) {
                        ESP_LOGI(TAG, "Stopping game...");
                        esp_err_t ret = game_stop();
                        if (ret == ESP_OK) {
                            buzzer_play_pattern(BUZZER_PATTERN_GAME_END);
                            ESP_LOGI(TAG, "Game stopped and reset");
                        } else {
                            buzzer_play_pattern(BUZZER_PATTERN_ERROR);
                            ESP_LOGE(TAG, "Failed to stop game: %s", esp_err_to_name(ret));
                        }
                    }
                }
                break;
            case 3:  // Button 4 - Reserved for future use
                ESP_LOGI(TAG, "Button 4 pressed (no action assigned)");
                break;
        }
    } else if (event == BUTTON_EVENT_LONG_PRESS) {
        // Long press on Button 1: Turn all lasers ON/OFF
        if (button_id == 0) {
            ESP_LOGI(TAG, "Long press on Start/Stop button - toggling all lasers");
            buzzer_play_pattern(BUZZER_PATTERN_BEEP);
            
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
 * Web server game control callback (Main Unit)
 */
static esp_err_t game_control_callback(const char *command, const char *data)
{
    ESP_LOGI(TAG, "Game control from web: %s", command);
    
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    
    if (strcmp(command, "start") == 0) {
        buzzer_play_pattern(BUZZER_PATTERN_GAME_START);
        ret = game_start(GAME_MODE_SINGLE_SPEEDRUN, "Web Player");
    } else if (strcmp(command, "stop") == 0) {
        buzzer_play_pattern(BUZZER_PATTERN_GAME_END);
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
    
    return ESP_ERR_INVALID_ARG;
}

/**
 * ESP-NOW message received callback (Main Unit)
 */
static void espnow_recv_callback_main(const uint8_t *sender_mac, const espnow_message_t *message)
{
    ESP_LOGI(TAG, "ESP-NOW message received from %02X:%02X:%02X:%02X:%02X:%02X",
             sender_mac[0], sender_mac[1], sender_mac[2], 
             sender_mac[3], sender_mac[4], sender_mac[5]);
    
    // Update laser unit tracking
    game_update_laser_unit(message->module_id, sender_mac, -50); // RSSI not available in callback
    
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
            esp_err_t peer_ret = espnow_add_peer(sender_mac, message->module_id, 1);
            if (peer_ret == ESP_OK) {
                // Only log when peer is actually added (e.g., after main unit restart)
                ESP_LOGI(TAG, "Laser unit %d re-added as ESP-NOW peer", message->module_id);
            } else if (peer_ret != ESP_ERR_ESPNOW_EXIST) {
                ESP_LOGE(TAG, "Failed to add peer during heartbeat: %s", esp_err_to_name(peer_ret));
            }
            // Silent when peer already exists - no need to log every 3 seconds
            break;
        case MSG_STATUS_UPDATE:
            ESP_LOGD(TAG, "Status update from module %d", message->module_id);
            break;
        case MSG_PAIRING_REQUEST:
            ESP_LOGI(TAG, "Pairing request from module %d", message->module_id);
            
            // Extract role from message data (default to 1 for laser if not specified)
            // data[0] will be 1 (laser) or 2 (finish button), default to 1 for backward compatibility
            uint8_t peer_role = (message->data[0] == 2) ? 2 : 1;
            const char *role_name = (peer_role == 2) ? "Finish Button" : "Laser Unit";
            
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
 * Main Unit Initialization
 * Initializes display, buttons, buzzer, WiFi AP, web server, and ESP-NOW
 */
static void init_main_unit(void)
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
    button_config_t buttons[4] = {
        {.pin = CONFIG_BUTTON1_PIN, .debounce_time_ms = CONFIG_DEBOUNCE_TIME, .long_press_time_ms = 1000, .pull_up = true, .active_low = true},
        {.pin = CONFIG_BUTTON2_PIN, .debounce_time_ms = CONFIG_DEBOUNCE_TIME, .long_press_time_ms = 1000, .pull_up = true, .active_low = true},
        {.pin = CONFIG_BUTTON3_PIN, .debounce_time_ms = CONFIG_DEBOUNCE_TIME, .long_press_time_ms = 1000, .pull_up = true, .active_low = true},
        {.pin = CONFIG_BUTTON4_PIN, .debounce_time_ms = CONFIG_DEBOUNCE_TIME, .long_press_time_ms = 1000, .pull_up = true, .active_low = true}
    };
    
    // Count enabled buttons
    uint8_t num_buttons = 0;
    for (int i = 0; i < 4; i++) {
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
    // Initialize buzzer
    ESP_LOGI(TAG, "  Initializing Buzzer (GPIO %d)", CONFIG_BUZZER_PIN);
    ESP_ERROR_CHECK(buzzer_init(CONFIG_BUZZER_PIN));
    buzzer_set_volume(50); // 50% volume
    buzzer_play_pattern(BUZZER_PATTERN_SUCCESS); // Startup sound
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
        }
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
    } else {
        ESP_LOGI(TAG, "  Running in AP mode (Fallback): http://192.168.4.1");
    }
    
    // Initialize web server
    ESP_LOGI(TAG, "  Initializing Web Server (http://192.168.4.1)");
    ESP_ERROR_CHECK(web_server_init(NULL, game_control_callback));
    
    // Initialize ESP-NOW
    ESP_LOGI(TAG, "  Initializing ESP-NOW (Channel: %d)", CONFIG_ESPNOW_CHANNEL);
    ESP_ERROR_CHECK(espnow_manager_init(CONFIG_ESPNOW_CHANNEL, espnow_recv_callback_main));
    
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
    ESP_LOGI(TAG, "Buttons:        B1=GPIO%d, B2=GPIO%d, B3=GPIO%d, B4=GPIO%d",
             CONFIG_BUTTON1_PIN, CONFIG_BUTTON2_PIN, CONFIG_BUTTON3_PIN, CONFIG_BUTTON4_PIN);
#else
    ESP_LOGI(TAG, "Buttons:        Disabled (menuconfig)");
#endif
#ifdef CONFIG_ENABLE_BUZZER
    ESP_LOGI(TAG, "Buzzer:         GPIO%d", CONFIG_BUZZER_PIN);
#else
    ESP_LOGI(TAG, "Buzzer:         Disabled (menuconfig)");
#endif
    ESP_LOGI(TAG, "WiFi Channel:   %d", CONFIG_WIFI_CHANNEL);
    ESP_LOGI(TAG, "ESP-NOW Ch:     %d", CONFIG_ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "=================================================");
    
    ESP_LOGI(TAG, "Main Unit initialized - ready to coordinate game");
}
#endif

#ifdef CONFIG_MODULE_ROLE_LASER
// Pairing state
static bool is_paired = false;
static bool is_game_mode = false;  // Track if in game mode (vs manual laser control)
static esp_timer_handle_t pairing_timer = NULL;
static esp_timer_handle_t heartbeat_timer = NULL;
static esp_timer_handle_t led_blink_timer = NULL;
static esp_timer_handle_t safety_timer = NULL;  // Safety timer to monitor main unit heartbeat
static uint8_t main_unit_mac[6] = {0};  // MAC address of paired main unit

// Safety mechanism
static int64_t last_main_unit_heartbeat = 0;  // Timestamp of last heartbeat from main unit
static const int64_t HEARTBEAT_TIMEOUT_US = 10000000;  // 10 seconds in microseconds

// Channel scanning state
static uint8_t current_scan_channel = CONFIG_ESPNOW_CHANNEL;
static uint8_t scan_attempts_on_channel = 0;
static const uint8_t MAX_ATTEMPTS_PER_CHANNEL = 1;  // 1 pairing attempt per channel (fast scan)
static const uint8_t MAX_WIFI_CHANNEL = 13;          // WiFi channels 1-13
static uint8_t led_blink_state = 0;                  // For blinking status LED during scanning

/**
 * LED blink timer callback (Laser Unit)
 * Fast blink during pairing search
 */
static void led_blink_timer_callback(void *arg)
{
    if (!is_paired) {
        // Toggle LED for visual feedback during pairing
        led_blink_state = !led_blink_state;
        gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, led_blink_state);
    }
}

/**
 * Heartbeat timer callback (Laser Unit)
 * Sends periodic heartbeat to keep online status
 */
static void heartbeat_timer_callback(void *arg)
{
    if (is_paired) {
        // Send heartbeat message as unicast to main unit
        esp_err_t ret = espnow_send_message(main_unit_mac, MSG_HEARTBEAT, NULL, 0);
        ESP_LOGI(TAG, "Heartbeat sent to main unit: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGW(TAG, "Heartbeat timer fired but not paired!");
    }
}

/**
 * Safety timer callback (Laser Unit)
 * Monitors main unit heartbeat and turns off laser if no heartbeat received
 */
static void safety_timer_callback(void *arg)
{
    if (!is_paired) {
        return;  // Not paired, no safety check needed
    }
    
    // Get current time
    int64_t now = esp_timer_get_time();
    
    // Check if we haven't received a heartbeat from main unit in too long
    if (last_main_unit_heartbeat > 0) {
        int64_t time_since_heartbeat = now - last_main_unit_heartbeat;
        
        if (time_since_heartbeat > HEARTBEAT_TIMEOUT_US) {
            // No heartbeat for too long - check if laser is on
            laser_status_t laser_status = laser_get_status();
            
            if (laser_status == LASER_ON) {
                ESP_LOGW(TAG, "SAFETY: No heartbeat from main unit for %lld ms - turning off laser!",
                         time_since_heartbeat / 1000);
                
                // Turn off laser for safety
                laser_turn_off();
                
                // Turn off green LED, turn on red LED to indicate safety shutdown
                gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 0);
                gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 1);
                
                ESP_LOGE(TAG, "LASER SAFETY SHUTDOWN - No communication with main unit!");
            }
        }
    }
}

/**
 * Pairing request timer callback with channel scanning
 */
static void pairing_timer_callback(void *arg)
{
    if (!is_paired) {
        ESP_LOGI(TAG, "Sending pairing request on channel %d...", current_scan_channel);
        
        uint8_t role = 1;  // 1 = Laser Unit
        espnow_broadcast_message(MSG_PAIRING_REQUEST, &role, sizeof(role));
        scan_attempts_on_channel++;
        
        // After MAX_ATTEMPTS_PER_CHANNEL, switch to next channel
        if (scan_attempts_on_channel >= MAX_ATTEMPTS_PER_CHANNEL) {
            scan_attempts_on_channel = 0;
            current_scan_channel++;
            
            // Wrap back to channel 1 after channel 13
            if (current_scan_channel > MAX_WIFI_CHANNEL) {
                current_scan_channel = 1;
                ESP_LOGI(TAG, "Completed full channel scan, restarting from channel 1");
            }
            
            // Switch channel
            ESP_LOGI(TAG, "No response, switching to channel %d", current_scan_channel);
            esp_err_t ret = espnow_change_channel(current_scan_channel);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to switch channel: %s", esp_err_to_name(ret));
            }
        }
    }
}

/**
 * Initialize status LEDs
 */
static void init_status_leds(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    // Configure laser status LED
    io_conf.pin_bit_mask = (1ULL << CONFIG_LASER_STATUS_LED_PIN);
    gpio_config(&io_conf);
    gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 0);
    
    // Configure sensor green LED
    io_conf.pin_bit_mask = (1ULL << CONFIG_SENSOR_LED_GREEN_PIN);
    gpio_config(&io_conf);
    gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 0);
    
    // Configure sensor red LED
    io_conf.pin_bit_mask = (1ULL << CONFIG_SENSOR_LED_RED_PIN);
    gpio_config(&io_conf);
    gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 0);
    
    ESP_LOGI(TAG, "Status LEDs initialized (Status:%d, Green:%d, Red:%d)",
             CONFIG_LASER_STATUS_LED_PIN, CONFIG_SENSOR_LED_GREEN_PIN, CONFIG_SENSOR_LED_RED_PIN);
}

/**
 * Beam break callback (Laser Unit)
 */
static void beam_break_callback(uint8_t sensor_id)
{
    ESP_LOGW(TAG, "Beam broken detected on sensor %d!", sensor_id);
    
    // Turn on red LED, turn off green LED (in game mode)
    gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 1);
    gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 0);
    
    // Send beam break to main unit (unicast)
    if (is_paired) {
        esp_err_t ret = espnow_send_message(main_unit_mac, MSG_BEAM_BROKEN, &sensor_id, sizeof(sensor_id));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send beam break: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "Beam break sent to main unit");
        }
    } else {
        ESP_LOGW(TAG, "Not paired, cannot send beam break");
    }
}

/**
 * Beam restore callback (Laser Unit)
 */
static void beam_restore_callback(uint8_t sensor_id)
{
    ESP_LOGI(TAG, "Beam restored on sensor %d", sensor_id);
    
    // Turn on green LED, turn off red LED (in game mode)
    gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 1);
    gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 0);
}

/**
 * ESP-NOW message received callback (Laser Unit)
 */
static void espnow_recv_callback_laser(const uint8_t *sender_mac, const espnow_message_t *message)
{
    ESP_LOGI(TAG, "ESP-NOW message received from %02X:%02X:%02X:%02X:%02X:%02X",
             sender_mac[0], sender_mac[1], sender_mac[2], 
             sender_mac[3], sender_mac[4], sender_mac[5]);
    
    switch (message->msg_type) {
        case MSG_GAME_START:
            ESP_LOGI(TAG, "Game start command received");
            is_game_mode = true;  // Enter game mode
            last_main_unit_heartbeat = esp_timer_get_time();  // Initialize safety timer
            laser_turn_on(100);  // Turn laser on at full intensity
            // Turn off status LED during game (status visible via green/red LEDs)
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 0);
            // Initialize game LEDs: green on (beam OK), red off
            gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 1);
            gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 0);
            // Start sensor monitoring
            sensor_start_monitoring();
            ESP_LOGI(TAG, "Sensor monitoring started (Game Mode) - Safety timer active");
            break;
        case MSG_GAME_STOP:
            ESP_LOGI(TAG, "Game stop command received");
            is_game_mode = false;  // Exit game mode
            // Stop sensor monitoring
            sensor_stop_monitoring();
            ESP_LOGI(TAG, "Sensor monitoring stopped");
            laser_turn_off();
            // Turn status LED back on (connected state)
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 1);
            // Turn off game LEDs
            gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 0);
            gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 0);
            break;
        case MSG_LASER_ON:
            ESP_LOGI(TAG, "Laser ON command (manual)");
            laser_turn_on(message->data[0]);  // Intensity in data[0]
            // Turn off status LED during manual laser operation
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 0);
            // Manual laser on: both green and red LEDs on (only if not in game mode)
            if (!is_game_mode) {
                gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 1);
                gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 1);
            }
            break;
        case MSG_LASER_OFF:
            ESP_LOGI(TAG, "Laser OFF command (manual)");
            laser_turn_off();
            // Turn status LED back on (connected state)
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 1);
            // Turn off manual laser LEDs (only if not in game mode)
            if (!is_game_mode) {
                gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 0);
                gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 0);
            }
            break;
        case MSG_HEARTBEAT:
            // Ignore heartbeat messages (we send them, don't need to process them)
            // But if it's from main unit, update safety timer
            if (memcmp(sender_mac, main_unit_mac, 6) == 0) {
                last_main_unit_heartbeat = esp_timer_get_time();
                ESP_LOGD(TAG, "Heartbeat from main unit received - safety timer updated");
            } else {
                ESP_LOGD(TAG, "Heartbeat received (ignoring - not from main unit)");
            }
            break;
        case MSG_PAIRING_RESPONSE:
            ESP_LOGI(TAG, "Pairing response received - paired successfully on channel %d!", current_scan_channel);
            
            // Store main unit MAC address for unicast heartbeats
            memcpy(main_unit_mac, sender_mac, 6);
            ESP_LOGI(TAG, "Main unit MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                     main_unit_mac[0], main_unit_mac[1], main_unit_mac[2],
                     main_unit_mac[3], main_unit_mac[4], main_unit_mac[5]);
            
            // Add main unit as peer for unicast communication
            esp_err_t peer_ret = espnow_add_peer(main_unit_mac, 0, 0); // module_id=0, role=0 for main
            if (peer_ret == ESP_OK) {
                ESP_LOGI(TAG, "Main unit added as peer");
            } else if (peer_ret == ESP_ERR_ESPNOW_EXIST) {
                ESP_LOGD(TAG, "Main unit peer already exists");
            } else {
                ESP_LOGE(TAG, "Failed to add main unit as peer: %s", esp_err_to_name(peer_ret));
            }
            
            is_paired = true;
            scan_attempts_on_channel = 0;  // Reset scan state
            led_blink_state = 0;           // Reset blink state
            
            // Status LED solid on = connected/paired
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 1);
            
            // Stop pairing timer
            if (pairing_timer) {
                esp_err_t ret = esp_timer_stop(pairing_timer);
                ESP_LOGI(TAG, "Pairing timer stopped: %s", esp_err_to_name(ret));
            }
            
            // Stop LED blink timer
            if (led_blink_timer && esp_timer_is_active(led_blink_timer)) {
                esp_timer_stop(led_blink_timer);
                ESP_LOGI(TAG, "LED blink timer stopped");
            }
            
            // Start heartbeat timer (3 seconds)
            if (heartbeat_timer) {
                if (!esp_timer_is_active(heartbeat_timer)) {
                    esp_err_t ret = esp_timer_start_periodic(heartbeat_timer, 3000000);  // 3 seconds
                    ESP_LOGI(TAG, "Heartbeat timer started: %s", esp_err_to_name(ret));
                } else {
                    ESP_LOGI(TAG, "Heartbeat timer already active");
                }
            } else {
                ESP_LOGE(TAG, "Heartbeat timer is NULL!");
            }
            
            break;
        case MSG_RESET:
            ESP_LOGI(TAG, "Reset command received");
            // Reset game mode
            is_game_mode = false;
            // Reset safety timer
            last_main_unit_heartbeat = 0;
            // Stop sensor monitoring
            sensor_stop_monitoring();
            // Turn off laser and all LEDs
            laser_turn_off();
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 0);
            gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 0);
            gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 0);
            
            // Reset pairing state
            is_paired = false;
            
            // Stop heartbeat timer
            if (heartbeat_timer && esp_timer_is_active(heartbeat_timer)) {
                esp_timer_stop(heartbeat_timer);
                ESP_LOGI(TAG, "Heartbeat timer stopped");
            }
            
            // Reset channel scan state
            current_scan_channel = CONFIG_ESPNOW_CHANNEL;  // Back to configured start channel
            scan_attempts_on_channel = 0;
            led_blink_state = 0;  // Reset blink state
            
            // Restart pairing timer
            if (pairing_timer) {
                esp_timer_start_periodic(pairing_timer, 1500000); // 1.5 seconds
                ESP_LOGI(TAG, "Pairing timer restarted, will scan from channel %d", current_scan_channel);
            }
            
            // Restart LED blink timer
            if (led_blink_timer) {
                esp_timer_start_periodic(led_blink_timer, 500000);  // 500ms
                ESP_LOGI(TAG, "LED blink timer restarted");
            }
            ESP_LOGI(TAG, "Module reset complete");
            break;
        case MSG_CHANNEL_CHANGE: {
            uint8_t new_channel = message->data[0];
            ESP_LOGI(TAG, "Channel change request to channel %d", new_channel);
            
            // Change channel
            esp_err_t ret = espnow_change_channel(new_channel);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Channel changed successfully to %d", new_channel);
                
                // Send ACK back to main unit
                espnow_broadcast_message(MSG_CHANNEL_ACK, NULL, 0);
            } else {
                ESP_LOGE(TAG, "Failed to change channel: %s", esp_err_to_name(ret));
            }
            break;
        }
        default:
            ESP_LOGW(TAG, "Unknown message type: 0x%02X", message->msg_type);
            break;
    }
}

/**
 * Laser Unit Initialization
 * Initializes laser control (PWM), sensor (ADC), LEDs, safety monitoring, and ESP-NOW
 */
static void init_laser_unit(void)
{
    ESP_LOGI(TAG, "Initializing Laser Unit...");
    
    // Initialize laser PWM control
    ESP_LOGI(TAG, "  Initializing Laser PWM (GPIO %d)", CONFIG_LASER_PIN);
    ESP_ERROR_CHECK(laser_control_init(CONFIG_LASER_PIN));
    ESP_ERROR_CHECK(laser_set_safety_timeout(true));  // Enable safety timeout
    
    // Initialize ADC for photoresistor/sensor
    ESP_LOGI(TAG, "  Initializing ADC Sensor (GPIO %d, Threshold: %d)", 
             CONFIG_SENSOR_PIN, CONFIG_SENSOR_THRESHOLD);
    ESP_ERROR_CHECK(sensor_manager_init(CONFIG_SENSOR_PIN, CONFIG_SENSOR_THRESHOLD, CONFIG_DEBOUNCE_TIME));
    ESP_ERROR_CHECK(sensor_register_callback(beam_break_callback));
    ESP_ERROR_CHECK(sensor_register_restore_callback(beam_restore_callback));
    
    // Initialize status LEDs
    ESP_LOGI(TAG, "  Initializing Status LEDs (Status: GPIO %d, Green: GPIO %d, Red: GPIO %d)",
             CONFIG_LASER_STATUS_LED_PIN, CONFIG_SENSOR_LED_GREEN_PIN, CONFIG_SENSOR_LED_RED_PIN);
    init_status_leds();
    
    // Initialize WiFi (required for ESP-NOW)
    ESP_LOGI(TAG, "  Initializing WiFi for ESP-NOW");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Initialize ESP-NOW
    ESP_LOGI(TAG, "  Initializing ESP-NOW (Channel: %d)", CONFIG_ESPNOW_CHANNEL);
    ESP_ERROR_CHECK(espnow_manager_init(CONFIG_ESPNOW_CHANNEL, espnow_recv_callback_laser));
    
    // Set up periodic pairing request timer (every 1.5 seconds until paired)
    ESP_LOGI(TAG, "  Setting up pairing request timer");
    const esp_timer_create_args_t pairing_timer_args = {
        .callback = &pairing_timer_callback,
        .name = "pairing_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&pairing_timer_args, &pairing_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(pairing_timer, 1500000));  // 1.5 seconds
    
    // Set up LED blink timer for visual feedback during pairing
    ESP_LOGI(TAG, "  Setting up LED blink timer");
    const esp_timer_create_args_t led_blink_timer_args = {
        .callback = &led_blink_timer_callback,
        .name = "led_blink_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&led_blink_timer_args, &led_blink_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(led_blink_timer, 500000));  // 500ms (fast blink)
    
    // Set up heartbeat timer (starts after successful pairing)
    ESP_LOGI(TAG, "  Setting up heartbeat timer");
    const esp_timer_create_args_t heartbeat_timer_args = {
        .callback = &heartbeat_timer_callback,
        .name = "heartbeat_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&heartbeat_timer_args, &heartbeat_timer));
    
    // Set up safety timer to monitor main unit heartbeat
    ESP_LOGI(TAG, "  Setting up laser safety timer");
    const esp_timer_create_args_t safety_timer_args = {
        .callback = &safety_timer_callback,
        .name = "safety_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&safety_timer_args, &safety_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(safety_timer, 2000000));  // Check every 2 seconds
    
    // Send initial pairing request
    ESP_LOGI(TAG, "  Sending initial pairing request to main unit");
    espnow_broadcast_message(MSG_PAIRING_REQUEST, NULL, 0);
    
    // Print GPIO configuration summary
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "   Laser Unit - GPIO Configuration");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Laser Diode:    GPIO%d (PWM)", CONFIG_LASER_PIN);
    ESP_LOGI(TAG, "Sensor ADC:     GPIO%d (Channel %d)", CONFIG_SENSOR_PIN, CONFIG_SENSOR_PIN);
    ESP_LOGI(TAG, "Threshold:      %d (ADC units)", CONFIG_SENSOR_THRESHOLD);
    ESP_LOGI(TAG, "Status LED:     GPIO%d", CONFIG_LASER_STATUS_LED_PIN);
    ESP_LOGI(TAG, "Green LED:      GPIO%d", CONFIG_SENSOR_LED_GREEN_PIN);
    ESP_LOGI(TAG, "Red LED:        GPIO%d", CONFIG_SENSOR_LED_RED_PIN);
    ESP_LOGI(TAG, "ESP-NOW Ch:     %d (scanning)", CONFIG_ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "=================================================");
    
    ESP_LOGI(TAG, "Laser Unit initialized - ready to emit beams and detect breaks");
}
#endif

/******************************************************************************
 * FINISH BUTTON MODULE - Finish Line Button Device
 ******************************************************************************/
#ifdef IS_FINISH_MODULE

// Finish Button Module State
static bool is_paired = false;
static uint8_t main_unit_mac[6] = {0};
static esp_timer_handle_t pairing_timer = NULL;
static esp_timer_handle_t led_blink_timer = NULL;
static esp_timer_handle_t heartbeat_timer = NULL;
static bool status_led_state = false;
static bool button_led_on = true;  // Button illumination LED starts ON
static volatile bool button_pressed = false;

// Channel scanning state for pairing
static bool channel_scanning = true;
static uint8_t scanning_channels[] = {1, 6, 11};
static uint8_t scanning_channel_index = 0;

/**
 * Initialize status LED for finish button
 */
static void init_finish_button_leds(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    // Configure status LED (connection/pairing indicator)
    io_conf.pin_bit_mask = (1ULL << CONFIG_FINISH_STATUS_LED_PIN);
    gpio_config(&io_conf);
    gpio_set_level(CONFIG_FINISH_STATUS_LED_PIN, 0);
    
    // Configure button illumination LED
    io_conf.pin_bit_mask = (1ULL << CONFIG_FINISH_BUTTON_LED_PIN);
    gpio_config(&io_conf);
    gpio_set_level(CONFIG_FINISH_BUTTON_LED_PIN, 1);  // Initially ON
    
    ESP_LOGI(TAG, "Finish Button LEDs initialized (Status:%d, Button Light:%d)",
             CONFIG_FINISH_STATUS_LED_PIN, CONFIG_FINISH_BUTTON_LED_PIN);
}

/**
 * LED blink timer callback for finish button
 */
static void led_blink_timer_callback(void *arg)
{
    if (!is_paired) {
        // Blink status LED when not paired
        status_led_state = !status_led_state;
        gpio_set_level(CONFIG_FINISH_STATUS_LED_PIN, status_led_state ? 1 : 0);
    } else {
        // Solid on when paired
        gpio_set_level(CONFIG_FINISH_STATUS_LED_PIN, 1);
    }
}

/**
 * Heartbeat timer callback (sends periodic heartbeat to main unit)
 */
static void heartbeat_timer_callback(void *arg)
{
    if (is_paired) {
        espnow_broadcast_message(MSG_HEARTBEAT, NULL, 0);
    }
}

/**
 * Pairing timer callback with multi-channel scanning
 */
static void pairing_timer_callback(void *arg)
{
    if (!is_paired) {
        if (channel_scanning) {
            // Cycle through common channels
            uint8_t target_channel = scanning_channels[scanning_channel_index];
            ESP_LOGI(TAG, "Scanning channel %d for main unit...", target_channel);
            
            // Change to next scanning channel
            espnow_change_channel(target_channel);
            
            // Toggle status LED while scanning (visual feedback)
            status_led_state = !status_led_state;
            gpio_set_level(CONFIG_FINISH_STATUS_LED_PIN, status_led_state ? 1 : 0);
            
            // Move to next channel for next attempt
            scanning_channel_index = (scanning_channel_index + 1) % (sizeof(scanning_channels) / sizeof(scanning_channels[0]));
        }
        
        // Send pairing request (broadcast)
        ESP_LOGI(TAG, "Sending pairing request (Module ID: %d)...", CONFIG_MODULE_ID);
        uint8_t role = 2;  // 2 = Finish Button
        espnow_broadcast_message(MSG_PAIRING_REQUEST, &role, sizeof(role));
    }
}

/**
 * Button interrupt handler (active low)
 */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    button_pressed = true;
}

/**
 * Button handler task - processes button press and sends finish message
 */
static void button_handler_task(void *arg)
{
    ESP_LOGI(TAG, "Button handler task started");
    
    while (1) {
        if (button_pressed) {
            button_pressed = false;
            
            // Debounce delay
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // Check if button is still pressed (active low)
            if (gpio_get_level(CONFIG_FINISH_BUTTON_PIN) == 0) {
                ESP_LOGI(TAG, "Finish button pressed!");
                
                // Turn OFF button illumination LED when pressed
                gpio_set_level(CONFIG_FINISH_BUTTON_LED_PIN, 0);
                button_led_on = false;
                
                // Send finish message to main unit
                if (is_paired) {
                    esp_err_t ret = espnow_send_message(main_unit_mac, MSG_FINISH_PRESSED, NULL, 0);
                    if (ret != ESP_OK) {
                        ESP_LOGE(TAG, "Failed to send finish message: %s", esp_err_to_name(ret));
                    } else {
                        ESP_LOGI(TAG, "Finish message sent to main unit successfully!");
                    }
                } else {
                    ESP_LOGW(TAG, "Not paired, cannot send finish message");
                }
                
                // Wait for button release
                while (gpio_get_level(CONFIG_FINISH_BUTTON_PIN) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                
                // Turn button illumination LED back ON after release
                gpio_set_level(CONFIG_FINISH_BUTTON_LED_PIN, 1);
                button_led_on = true;
                ESP_LOGI(TAG, "Button released");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * ESP-NOW message received callback (Finish Button Unit)
 */
static void espnow_recv_callback_finish(const uint8_t *sender_mac, const espnow_message_t *message)
{
    ESP_LOGI(TAG, "ESP-NOW message received from %02X:%02X:%02X:%02X:%02X:%02X",
             sender_mac[0], sender_mac[1], sender_mac[2], 
             sender_mac[3], sender_mac[4], sender_mac[5]);
    
    switch (message->msg_type) {
        case MSG_PAIRING_RESPONSE:
            ESP_LOGI(TAG, "Pairing response received!");
            if (!is_paired) {
                is_paired = true;
                memcpy(main_unit_mac, sender_mac, 6);
                ESP_LOGI(TAG, "Successfully paired with main unit: %02X:%02X:%02X:%02X:%02X:%02X",
                         main_unit_mac[0], main_unit_mac[1], main_unit_mac[2],
                         main_unit_mac[3], main_unit_mac[4], main_unit_mac[5]);
                
                // Stop channel scanning
                channel_scanning = false;
                
                // Turn on status LED solid
                gpio_set_level(CONFIG_FINISH_STATUS_LED_PIN, 1);
                
                // Start heartbeat timer (every 3 seconds)
                esp_timer_start_periodic(heartbeat_timer, 3000000);
            }
            break;
            
        case MSG_HEARTBEAT:
            // Ignore heartbeat broadcasts from other units
            break;
            
        case MSG_RESET:
            ESP_LOGI(TAG, "Reset command received - resetting pairing state");
            is_paired = false;
            channel_scanning = true;
            scanning_channel_index = 0;
            
            // Turn off button illumination LED
            gpio_set_level(CONFIG_FINISH_BUTTON_LED_PIN, 1);  // Reset to ON
            button_led_on = true;
            
            // Stop heartbeat timer
            esp_timer_stop(heartbeat_timer);
            break;
            
        case MSG_CHANNEL_CHANGE: {
            uint8_t new_channel = message->data[0];
            ESP_LOGI(TAG, "Channel change requested: %d", new_channel);
            
            // Stop channel scanning when main unit sets specific channel
            channel_scanning = false;
            
            // Change channel
            esp_err_t ret = espnow_change_channel(new_channel);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Channel changed successfully to %d", new_channel);
                
                // Send ACK back to main unit
                espnow_broadcast_message(MSG_CHANNEL_ACK, NULL, 0);
            } else {
                ESP_LOGE(TAG, "Failed to change channel: %s", esp_err_to_name(ret));
            }
            break;
        }
            
        default:
            ESP_LOGW(TAG, "Unknown message type: 0x%02X", message->msg_type);
            break;
    }
}

/**
 * Finish Button Unit Initialization
 */
static void init_finish_button_unit(void)
{
    ESP_LOGI(TAG, "Initializing Finish Button Unit...");
    
    // Initialize button GPIO (active low with pull-up)
    ESP_LOGI(TAG, "  Initializing Button (GPIO %d)", CONFIG_FINISH_BUTTON_PIN);
    gpio_config_t button_conf = {
        .pin_bit_mask = (1ULL << CONFIG_FINISH_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,  // Trigger on falling edge (button press)
    };
    gpio_config(&button_conf);
    
    // Install GPIO ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(CONFIG_FINISH_BUTTON_PIN, button_isr_handler, NULL);
    
    // Initialize LEDs
    ESP_LOGI(TAG, "  Initializing LEDs (Status: GPIO %d, Button Light: GPIO %d)",
             CONFIG_FINISH_STATUS_LED_PIN, CONFIG_FINISH_BUTTON_LED_PIN);
    init_finish_button_leds();
    
    // Initialize WiFi (required for ESP-NOW)
    ESP_LOGI(TAG, "  Initializing WiFi for ESP-NOW");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Initialize ESP-NOW
    ESP_LOGI(TAG, "  Initializing ESP-NOW (Channel: %d)", CONFIG_ESPNOW_CHANNEL);
    ESP_ERROR_CHECK(espnow_manager_init(CONFIG_ESPNOW_CHANNEL, espnow_recv_callback_finish));
    
    // Set up periodic pairing request timer (every 1.5 seconds until paired)
    ESP_LOGI(TAG, "  Setting up pairing request timer");
    const esp_timer_create_args_t pairing_timer_args = {
        .callback = &pairing_timer_callback,
        .name = "pairing_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&pairing_timer_args, &pairing_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(pairing_timer, 1500000));  // 1.5 seconds
    
    // Set up LED blink timer for visual feedback during pairing
    ESP_LOGI(TAG, "  Setting up LED blink timer");
    const esp_timer_create_args_t led_blink_timer_args = {
        .callback = &led_blink_timer_callback,
        .name = "led_blink_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&led_blink_timer_args, &led_blink_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(led_blink_timer, 500000));  // 500ms (fast blink)
    
    // Set up heartbeat timer (starts after successful pairing)
    ESP_LOGI(TAG, "  Setting up heartbeat timer");
    const esp_timer_create_args_t heartbeat_timer_args = {
        .callback = &heartbeat_timer_callback,
        .name = "heartbeat_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&heartbeat_timer_args, &heartbeat_timer));
    
    // Create button handler task
    ESP_LOGI(TAG, "  Creating button handler task");
    xTaskCreate(button_handler_task, "button_handler", 4096, NULL, 5, NULL);
    
    // Send initial pairing request
    ESP_LOGI(TAG, "  Sending initial pairing request to main unit");
    espnow_broadcast_message(MSG_PAIRING_REQUEST, NULL, 0);
    
    // Print GPIO configuration summary
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "   Finish Button - GPIO Configuration");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Button GPIO:    GPIO%d (Active Low)", CONFIG_FINISH_BUTTON_PIN);
    ESP_LOGI(TAG, "Status LED:     GPIO%d (Pairing Status)", CONFIG_FINISH_STATUS_LED_PIN);
    ESP_LOGI(TAG, "Button LED:     GPIO%d (Illumination)", CONFIG_FINISH_BUTTON_LED_PIN);
    ESP_LOGI(TAG, "ESP-NOW Ch:     %d (scanning)", CONFIG_ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "=================================================");
    
    ESP_LOGI(TAG, "Finish Button Unit initialized - ready to complete games!");
}
#endif

/**
 * Main application entry point
 */
void app_main(void)
{
    esp_log_level_set("SSD1306", ESP_LOG_DEBUG);  // Enable debug logs for SSD1306 driver
    // Print system information
    print_system_info();
    
    // Initialize NVS
    init_nvs();
    
    // Initialize network stack
    init_network();
    
    // Initialize module based on role
#ifdef IS_CONTROL_MODULE
    init_main_unit();
#elif defined(IS_LASER_MODULE)
    init_laser_unit();
#elif defined(IS_FINISH_MODULE)
    init_finish_button_unit();
#endif
    
    ESP_LOGI(TAG, "Initialization complete!");
    ESP_LOGI(TAG, "System is running - Module ID: %d, Role: %s", 
             CONFIG_MODULE_ID, MODULE_ROLE);
    
    // Main loop
    while (1) {
#ifdef CONFIG_MODULE_ROLE_CONTROL
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
        
        ESP_LOGI(TAG, "Status: Running (%s Module ID:%d) - Free heap: %ld bytes%s",
                 MODULE_ROLE, CONFIG_MODULE_ID, esp_get_free_heap_size(), units_str);
#else
        ESP_LOGI(TAG, "Status: Running (%s Module ID:%d) - Free heap: %ld bytes",
                 MODULE_ROLE, CONFIG_MODULE_ID, esp_get_free_heap_size());
#endif
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
