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

#ifdef CONFIG_MODULE_ROLE_CONTROL
/**
 * ESP-NOW message received callback (Main Unit)
 */
static void espnow_recv_callback_main(const uint8_t *sender_mac, const espnow_message_t *message)
{
    ESP_LOGI(TAG, "ESP-NOW message received from %02X:%02X:%02X:%02X:%02X:%02X",
             sender_mac[0], sender_mac[1], sender_mac[2], 
             sender_mac[3], sender_mac[4], sender_mac[5]);
    
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
    
    // Initialize OLED display
    ESP_LOGI(TAG, "  Initializing OLED Display (I2C SDA:%d SCL:%d)", 
             CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN);
    ESP_ERROR_CHECK(display_manager_init(CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN, CONFIG_I2C_FREQUENCY));
    display_set_screen(SCREEN_IDLE);
    
    // Initialize buttons (TODO: Implement button handler component)
    ESP_LOGI(TAG, "  [TODO] Initialize Buttons (GPIO %d, %d, %d, %d)",
             CONFIG_BUTTON1_PIN, CONFIG_BUTTON2_PIN, CONFIG_BUTTON3_PIN, CONFIG_BUTTON4_PIN);
    
    // Initialize buzzer (TODO: Implement buzzer component)
    ESP_LOGI(TAG, "  [TODO] Initialize Buzzer (GPIO %d)", CONFIG_BUZZER_PIN);
    
    // Initialize WiFi AP (TODO: Move to dedicated component)
    ESP_LOGI(TAG, "  [TODO] Initialize WiFi AP (SSID: %s, Channel: %d)",
             CONFIG_WIFI_SSID, CONFIG_WIFI_CHANNEL);
    
    // Initialize web server (TODO: Implement web server component)
    ESP_LOGI(TAG, "  [TODO] Initialize Web Server (http://192.168.4.1)");
    
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

/**
 * Pairing request timer callback
 */
static void pairing_timer_callback(void *arg)
{
    if (!is_paired) {
        ESP_LOGI(TAG, "Sending pairing request (not yet paired)...");
        espnow_broadcast_message(MSG_PAIRING_REQUEST, NULL, 0);
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
            break;
        case MSG_GAME_STOP:
            ESP_LOGI(TAG, "Game stop command received");
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
            ESP_LOGI(TAG, "Pairing response received - paired successfully!");
            is_paired = true;
            if (pairing_timer) {
                esp_timer_stop(pairing_timer);
                ESP_LOGI(TAG, "Pairing timer stopped");
            }
            gpio_set_level(CONFIG_LASER_STATUS_LED_PIN, 1);  // Status LED on when paired
            break;
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
        // TODO: Implement main logic based on module role
        // For now, just report status every 5 seconds
        ESP_LOGI(TAG, "Status: Running (%s Module ID:%d) - Free heap: %ld bytes",
                 MODULE_ROLE, CONFIG_MODULE_ID, esp_get_free_heap_size());
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
