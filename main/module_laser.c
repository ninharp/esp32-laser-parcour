/**
 * LASER Module - Laser Unit Implementation
 * 
 * Handles laser unit initialization and event callbacks
 * 
 * @author ninharp
 * @date 2026-01-09
 */

#include "module_laser.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "string.h"

// Component includes
#include "laser_control.h"
#include "sensor_manager.h"
#include "espnow_manager.h"

static const char *TAG = "MODULE_LASER";

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
static const int64_t HEARTBEAT_TIMEOUT_US = 30000000;  // 30 seconds in microseconds

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
            
            // Remove main unit from ESP-NOW peers if paired
            if (is_paired) {
                esp_err_t err = espnow_remove_peer(main_unit_mac);
                if (err != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to remove peer: %s", esp_err_to_name(err));
                } else {
                    ESP_LOGI(TAG, "Main unit removed from ESP-NOW peers");
                }
            }
            
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
 * Initialize the laser unit
 */
void module_laser_init(void)
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

/**
 * Run the laser unit loop
 */
void module_laser_run(void)
{
    // Main loop for laser module
    while (1) {
        ESP_LOGI(TAG, "Status: Running - Free heap: %ld bytes - Paired: %s",
                 esp_get_free_heap_size(), is_paired ? "Yes" : "No");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
