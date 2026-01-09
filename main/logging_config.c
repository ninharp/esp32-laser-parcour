/**
 * Logging Configuration - Implementation
 * 
 * Centralized logging level configuration for all components
 * Separates project-specific tags from ESP-IDF system tags
 * 
 * @author ninharp
 * @date 2026-01-09
 */

#include "logging_config.h"

void init_logging(void)
{
    // ========================================
    // DEFAULT: Disable all logs by default
    // ========================================
    esp_log_level_set("*", ESP_LOG_NONE);
    
    // ========================================
    // PROJECT-SPECIFIC COMPONENTS (DEBUG)
    // ========================================
    esp_log_level_set("LASER_PARCOUR", ESP_LOG_DEBUG);    // Main application
    esp_log_level_set("GAME_LOGIC", ESP_LOG_DEBUG);       // Game state & scoring
    esp_log_level_set("DISPLAY_MGR", ESP_LOG_INFO);      // Display manager
    esp_log_level_set("SSD1306", ESP_LOG_DEBUG);          // SSD1306 OLED driver
    esp_log_level_set("ESPNOW_MGR", ESP_LOG_DEBUG);       // ESP-NOW communication
    esp_log_level_set("BUTTON", ESP_LOG_DEBUG);           // Button handler
    esp_log_level_set("BUZZER", ESP_LOG_DEBUG);           // Buzzer/audio
    esp_log_level_set("LASER_CTRL", ESP_LOG_DEBUG);       // Laser control
    esp_log_level_set("SENSOR_MGR", ESP_LOG_DEBUG);       // Sensor manager
    esp_log_level_set("WEB_SERVER", ESP_LOG_DEBUG);       // HTTP web server
    esp_log_level_set("WIFI_AP_MGR", ESP_LOG_DEBUG);      // WiFi AP manager
    esp_log_level_set("SD_CARD_MANAGER", ESP_LOG_DEBUG);  // SD card manager
    esp_log_level_set("SOUND_MGR", ESP_LOG_DEBUG);        // Sound manager

    esp_log_level_set("MODULE_CTRL", ESP_LOG_DEBUG);    // Control module
    esp_log_level_set("MODULE_LASER", ESP_LOG_DEBUG);    // Laser module
    esp_log_level_set("MODULE_FINISH", ESP_LOG_DEBUG);   // Finish button module
    
    // ========================================
    // ESP-IDF SYSTEM COMPONENTS
    // ========================================
    // esp_log_level_set("wifi", ESP_LOG_WARN);              // WiFi stack
    // esp_log_level_set("wifi_init", ESP_LOG_WARN);         // WiFi initialization
    // esp_log_level_set("phy_init", ESP_LOG_WARN);          // PHY initialization
    // esp_log_level_set("nvs", ESP_LOG_WARN);               // NVS storage
    // esp_log_level_set("esp_netif_handlers", ESP_LOG_WARN); // Network interface
    // esp_log_level_set("esp_netif_lwip", ESP_LOG_WARN);    // LwIP integration
    // esp_log_level_set("httpd", ESP_LOG_WARN);             // HTTP daemon
    // esp_log_level_set("httpd_uri", ESP_LOG_WARN);         // HTTP URI handler
    // esp_log_level_set("httpd_txrx", ESP_LOG_WARN);        // HTTP TX/RX
    // esp_log_level_set("httpd_parse", ESP_LOG_WARN);       // HTTP parser
    esp_log_level_set("gpio", ESP_LOG_WARN);              // GPIO driver
    // esp_log_level_set("ledc", ESP_LOG_WARN);              // LEDC PWM driver
    // esp_log_level_set("i2c", ESP_LOG_WARN);               // I2C driver
    // esp_log_level_set("adc", ESP_LOG_WARN);               // ADC driver
    esp_log_level_set("spi", ESP_LOG_WARN);               // SPI driver
    esp_log_level_set("sdmmc_cmd", ESP_LOG_WARN);         // SD/MMC commands
    esp_log_level_set("sdmmc_common", ESP_LOG_WARN);      // SD/MMC common
    esp_log_level_set("sdmmc_io", ESP_LOG_WARN);          // SD/MMC I/O
    esp_log_level_set("vfs_fat", ESP_LOG_WARN);           // FAT filesystem VFS
    esp_log_level_set("ff", ESP_LOG_WARN);                // FatFs library
    
    // ========================================
    // VERBOSE LOGGING (Optional - uncomment for deep debugging)
    // ========================================
    // esp_log_level_set("ESPNOW_MGR", ESP_LOG_VERBOSE);
    // esp_log_level_set("GAME_LOGIC", ESP_LOG_VERBOSE);
    // esp_log_level_set("wifi", ESP_LOG_VERBOSE);
}
