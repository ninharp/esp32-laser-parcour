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
    ESP_LOGI(TAG, "Flash:          %dMB %s", 
             spi_flash_get_chip_size() / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    ESP_LOGI(TAG, "Free Heap:      %ld bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "=================================================");
}

/**
 * Main Unit Initialization
 * Initializes display, buttons, buzzer, WiFi AP, web server, and ESP-NOW
 */
static void init_main_unit(void)
{
    ESP_LOGI(TAG, "Initializing Main Unit...");
    
    // TODO: Initialize I2C for OLED display
    ESP_LOGI(TAG, "  [TODO] Initialize OLED Display (I2C SDA:%d SCL:%d)", 
             CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN);
    
    // TODO: Initialize buttons
    ESP_LOGI(TAG, "  [TODO] Initialize Buttons (GPIO %d, %d, %d, %d)",
             CONFIG_BUTTON1_PIN, CONFIG_BUTTON2_PIN, CONFIG_BUTTON3_PIN, CONFIG_BUTTON4_PIN);
    
    // TODO: Initialize buzzer
    ESP_LOGI(TAG, "  [TODO] Initialize Buzzer (GPIO %d)", CONFIG_BUZZER_PIN);
    
    // TODO: Initialize WiFi AP
    ESP_LOGI(TAG, "  [TODO] Initialize WiFi AP (SSID: %s, Channel: %d)",
             CONFIG_WIFI_SSID, CONFIG_WIFI_CHANNEL);
    
    // TODO: Initialize web server
    ESP_LOGI(TAG, "  [TODO] Initialize Web Server (http://192.168.4.1)");
    
    // TODO: Initialize ESP-NOW
    ESP_LOGI(TAG, "  [TODO] Initialize ESP-NOW (Channel: %d)", CONFIG_ESPNOW_CHANNEL);
    
    // TODO: Initialize game logic
    ESP_LOGI(TAG, "  [TODO] Initialize Game Logic");
    
    ESP_LOGI(TAG, "Main Unit initialized - ready to coordinate game");
}

/**
 * Laser Unit Initialization
 * Initializes laser control (PWM), sensor (ADC), LEDs, safety monitoring, and ESP-NOW
 */
static void init_laser_unit(void)
{
    ESP_LOGI(TAG, "Initializing Laser Unit...");
    
    // TODO: Initialize laser PWM control
    ESP_LOGI(TAG, "  [TODO] Initialize Laser PWM (GPIO %d)", CONFIG_LASER_PIN);
    
    // TODO: Initialize ADC for photoresistor/sensor
    ESP_LOGI(TAG, "  [TODO] Initialize ADC Sensor (GPIO %d, Threshold: %d)", 
             CONFIG_SENSOR_PIN, CONFIG_SENSOR_THRESHOLD);
    
    // TODO: Initialize status LEDs
    ESP_LOGI(TAG, "  [TODO] Initialize Status LEDs (Status: GPIO %d, Green: GPIO %d, Red: GPIO %d)",
             CONFIG_LASER_STATUS_LED_PIN, CONFIG_SENSOR_LED_GREEN_PIN, CONFIG_SENSOR_LED_RED_PIN);
    
    // TODO: Initialize safety cutoff
    ESP_LOGI(TAG, "  [TODO] Initialize Safety Cutoff (timeout: 10 min)");
    
    // TODO: Initialize ESP-NOW
    ESP_LOGI(TAG, "  [TODO] Initialize ESP-NOW (Channel: %d)", CONFIG_ESPNOW_CHANNEL);
    
    // TODO: Start beam detection
    ESP_LOGI(TAG, "  [TODO] Start Beam Detection (debounce: %d ms)", CONFIG_DEBOUNCE_TIME);
    
    // TODO: Start pairing mode
    ESP_LOGI(TAG, "  [TODO] Start Pairing Mode - waiting for main unit");
    
    ESP_LOGI(TAG, "Laser Unit initialized - ready to emit beams and detect breaks");
}

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
