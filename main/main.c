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

static const char *TAG = "LASER_PARCOUR";

// Module role from Kconfig
#ifdef CONFIG_MODULE_ROLE_CONTROL
    #define MODULE_ROLE "MAIN_UNIT"
    #define IS_CONTROL_MODULE 1
#elif defined(CONFIG_MODULE_ROLE_LASER)
    #define MODULE_ROLE "LASER_UNIT"
    #define IS_LASER_MODULE 1
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
                // TODO: Toggle game start/stop
                break;
            case 1:  // Button 2 - Mode Select
                ESP_LOGI(TAG, "Mode select button pressed");
                // TODO: Cycle through game modes
                break;
            case 2:  // Button 3 - Reset
                ESP_LOGI(TAG, "Reset button pressed");
                // TODO: Reset game
                break;
            case 3:  // Button 4 - Confirm
                ESP_LOGI(TAG, "Confirm button pressed");
                // TODO: Confirm selection
                break;
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
        case MSG_STATUS_UPDATE:
            ESP_LOGD(TAG, "Status update from module %d", message->module_id);
            break;
        case MSG_PAIRING_REQUEST:
            ESP_LOGI(TAG, "Pairing request from module %d", message->module_id);
            
            // Add the laser unit as an ESP-NOW peer
            esp_err_t ret = espnow_add_peer(sender_mac, message->module_id, 1); // role = 1 for laser unit
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Laser unit %d added as peer", message->module_id);
            } else if (ret == ESP_ERR_ESPNOW_EXIST) {
                ESP_LOGD(TAG, "Peer already exists for module %d", message->module_id);
            } else {
                ESP_LOGE(TAG, "Failed to add peer: %s", esp_err_to_name(ret));
            }
            
            // Send pairing response back to the laser unit
            ret = espnow_send_message(sender_mac, MSG_PAIRING_RESPONSE, NULL, 0);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Pairing response sent to module %d", message->module_id);
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
        ESP_ERROR_CHECK(display_manager_init(CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN, CONFIG_I2C_FREQUENCY));
        display_set_screen(SCREEN_IDLE);
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
    
    // Initialize game logic
    ESP_LOGI(TAG, "  Initializing Game Logic");
    ESP_ERROR_CHECK(game_logic_init());
    
    ESP_LOGI(TAG, "Main Unit initialized - ready to coordinate game");
}
#endif

#ifdef CONFIG_MODULE_ROLE_LASER
// Pairing state
static bool is_paired = false;
static esp_timer_handle_t pairing_timer = NULL;

// Channel scanning state
static uint8_t current_scan_channel = CONFIG_ESPNOW_CHANNEL;
static uint8_t scan_attempts_on_channel = 0;
static const uint8_t MAX_ATTEMPTS_PER_CHANNEL = 3;  // 3 pairing attempts per channel
static const uint8_t MAX_WIFI_CHANNEL = 13;          // WiFi channels 1-13

/**
 * Pairing request timer callback with channel scanning
 */
static void pairing_timer_callback(void *arg)
{
    if (!is_paired) {
        ESP_LOGI(TAG, "Sending pairing request on channel %d (attempt %d/%d)...", 
                 current_scan_channel, scan_attempts_on_channel + 1, MAX_ATTEMPTS_PER_CHANNEL);
        
        espnow_broadcast_message(MSG_PAIRING_REQUEST, NULL, 0);
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
    
    // Turn on red LED, turn off green LED
    gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 1);
    gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 0);
    
    // Broadcast beam break to main unit
    espnow_broadcast_message(MSG_BEAM_BROKEN, &sensor_id, sizeof(sensor_id));
    
    // Restore green LED after short delay (handled by sensor manager debounce)
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
            laser_turn_on(100);  // Turn laser on at full intensity
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 1);
            gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 1);
            // Start sensor monitoring
            sensor_start_monitoring();
            ESP_LOGI(TAG, "Sensor monitoring started");
            break;
        case MSG_GAME_STOP:
            ESP_LOGI(TAG, "Game stop command received");
            // Stop sensor monitoring
            sensor_stop_monitoring();
            ESP_LOGI(TAG, "Sensor monitoring stopped");
            laser_turn_off();
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 0);
            gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 0);
            gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 0);
            break;
        case MSG_LASER_ON:
            ESP_LOGI(TAG, "Laser ON command");
            laser_turn_on(message->data[0]);  // Intensity in data[0]
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 1);
            break;
        case MSG_LASER_OFF:
            ESP_LOGI(TAG, "Laser OFF command");
            laser_turn_off();
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 0);
            break;
        case MSG_PAIRING_RESPONSE:
            ESP_LOGI(TAG, "Pairing response received - paired successfully on channel %d!", current_scan_channel);
            is_paired = true;
            scan_attempts_on_channel = 0;  // Reset scan state
            if (pairing_timer) {
                esp_timer_stop(pairing_timer);
                ESP_LOGI(TAG, "Pairing timer stopped");
            }
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 1);  // Status LED on when paired
            break;
        case MSG_RESET:
            ESP_LOGI(TAG, "Reset command received");
            // Stop sensor monitoring
            sensor_stop_monitoring();
            // Turn off laser and all LEDs
            laser_turn_off();
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 0);
            gpio_set_level(CONFIG_SENSOR_LED_GREEN_PIN, 0);
            gpio_set_level(CONFIG_SENSOR_LED_RED_PIN, 0);
            
            // Reset pairing state
            is_paired = false;
            
            // Reset channel scan state
            current_scan_channel = CONFIG_ESPNOW_CHANNEL;  // Back to configured start channel
            scan_attempts_on_channel = 0;
            
            // Restart pairing timer
            if (pairing_timer) {
                esp_timer_start_periodic(pairing_timer, 5000000); // 5 seconds
                ESP_LOGI(TAG, "Pairing timer restarted, will scan from channel %d", current_scan_channel);
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
    
    // Set up periodic pairing request timer (every 5 seconds until paired)
    ESP_LOGI(TAG, "  Setting up pairing request timer");
    const esp_timer_create_args_t pairing_timer_args = {
        .callback = &pairing_timer_callback,
        .name = "pairing_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&pairing_timer_args, &pairing_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(pairing_timer, 5000000));  // 5 seconds in microseconds
    
    // Send initial pairing request
    ESP_LOGI(TAG, "  Sending initial pairing request to main unit");
    espnow_broadcast_message(MSG_PAIRING_REQUEST, NULL, 0);
    
    ESP_LOGI(TAG, "Laser Unit initialized - ready to emit beams and detect breaks");
}
#endif

/**
 * Main application entry point
 */
void app_main(void)
{
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
