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
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"

// Logging configuration
#include "logging_config.h"

// ESP-NOW manager for channel change notification
#ifdef CONFIG_MODULE_ROLE_CONTROL
#include "espnow_manager.h"
#endif

// Module-specific includes
#ifdef CONFIG_MODULE_ROLE_CONTROL
    #include "module_control.h"
    #define MODULE_ROLE "MAIN_UNIT"
#elif defined(CONFIG_MODULE_ROLE_LASER)
    #include "module_laser.h"
    #define MODULE_ROLE "LASER_UNIT"
#elif defined(CONFIG_MODULE_ROLE_FINISH)
    #include "module_finish.h"
    #define MODULE_ROLE "FINISH_BUTTON"
#else
    #error "Module role not defined! Please run 'idf.py menuconfig' and select a module role."
#endif

static const char *TAG = "LASER_PARCOUR";

/**
 * Notify all ESP-NOW peers about channel change
 * Called by wifi_ap_manager before connecting to STA
 */
esp_err_t notify_channel_change(uint8_t new_channel)
{
#ifdef CONFIG_MODULE_ROLE_CONTROL
    // Only control module needs to notify peers
    ESP_LOGI(TAG, "Notifying all ESP-NOW peers about channel change to %d", new_channel);
    
    // Broadcast channel change via ESP-NOW
    esp_err_t ret = espnow_broadcast_channel_change(new_channel, 2000); // 2 second timeout
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Channel change notification sent successfully");
    } else {
        ESP_LOGW(TAG, "Failed to send channel change notification: %s", esp_err_to_name(ret));
    }
    
    return ret;
#else
    // Laser and finish modules don't notify others
    ESP_LOGD(TAG, "Channel change to %d (no notification needed)", new_channel);
    return ESP_OK;
#endif
}

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
 * Main application entry point
 */
void app_main(void)
{
    // Step 1: Initialize logging configuration FIRST
    init_logging();
    
    // Step 2: Print system information
    print_system_info();
    
    // Step 3: Initialize core system components
    init_nvs();
    init_network();
    
    // Step 4: Initialize module based on role
#ifdef CONFIG_MODULE_ROLE_CONTROL
    module_control_init();
#elif defined(CONFIG_MODULE_ROLE_LASER)
    module_laser_init();
#elif defined(CONFIG_MODULE_ROLE_FINISH)
    module_finish_init();
#endif
    
    ESP_LOGI(TAG, "Initialization complete!");
    ESP_LOGI(TAG, "System is running - Module ID: %d, Role: %s", 
             CONFIG_MODULE_ID, MODULE_ROLE);
    
    // Step 5: Main loop - delegate to module-specific implementation
#ifdef CONFIG_MODULE_ROLE_CONTROL
    module_control_run();
#elif defined(CONFIG_MODULE_ROLE_LASER)
    module_laser_run();
#elif defined(CONFIG_MODULE_ROLE_FINISH)
    module_finish_run();
#endif
}
