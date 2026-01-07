/**
 * WiFi Access Point Manager Component - Header
 * 
 * Manages WiFi Access Point for Main Unit.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef WIFI_AP_MANAGER_H
#define WIFI_AP_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * WiFi operating modes
 */
typedef enum {
    WIFI_MANAGER_MODE_AP,      /**< Access Point only */
    WIFI_MANAGER_MODE_STA,     /**< Station only */
    WIFI_MANAGER_MODE_APSTA    /**< AP + STA simultaneous */
} wifi_manager_mode_t;

/**
 * WiFi connection status
 */
typedef enum {
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_FAILED
} wifi_sta_status_t;

/**
 * Scanned WiFi network info
 */
typedef struct {
    char ssid[33];
    int8_t rssi;
    wifi_auth_mode_t authmode;
    uint8_t channel;
} wifi_scan_result_t;

/**
 * WiFi AP configuration
 */
typedef struct {
    char ssid[32];              // AP SSID
    char password[64];          // AP Password (min 8 chars)
    uint8_t channel;            // WiFi channel (1-13)
    uint8_t max_connection;     // Max simultaneous connections
} laser_ap_config_t;

/**
 * Initialize WiFi Access Point
 * 
 * @param config AP configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_ap_init(const laser_ap_config_t *config);

/**
 * Deinitialize WiFi Access Point
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_ap_deinit(void);

/**
 * Scan for available WiFi networks
 * 
 * @param results Array to store scan results
 * @param max_results Maximum number of results
 * @param num_found Pointer to store actual number found
 * @return ESP_OK on success
 */
esp_err_t wifi_scan_networks(wifi_scan_result_t *results, size_t max_results, size_t *num_found);

/**
 * Connect to WiFi network as Station
 * 
 * @param ssid Network SSID
 * @param password Network password
 * @param save_to_nvs Save credentials to NVS for auto-reconnect
 * @return ESP_OK on success
 */
esp_err_t wifi_connect_sta(const char *ssid, const char *password, bool save_to_nvs);

/**
 * Disconnect from WiFi station
 * 
 * @return ESP_OK on success
 */
esp_err_t wifi_disconnect_sta(void);

/**
 * Get WiFi station connection status
 * 
 * @return Current connection status
 */
wifi_sta_status_t wifi_get_sta_status(void);

/**
 * Get station IP info
 * 
 * @param ip_info Pointer to store IP info
 * @return ESP_OK on success
 */
esp_err_t wifi_get_sta_ip(esp_netif_ip_info_t *ip_info);

/**
 * Switch WiFi mode (AP, STA, or APSTA)
 * 
 * @param mode Desired WiFi mode
 * @return ESP_OK on success
 */
esp_err_t wifi_set_mode(wifi_manager_mode_t mode);

/**
 * Try to connect to saved WiFi credentials from NVS
 * If connection fails, start AP mode as fallback
 * 
 * @param ap_config AP configuration for fallback
 * @param timeout_ms Connection timeout in milliseconds
 * @return ESP_OK if connected to STA, ESP_FAIL if fallback to AP
 */
esp_err_t wifi_connect_with_fallback(const laser_ap_config_t *ap_config, uint32_t timeout_ms);

/**
 * Erase saved WiFi credentials from NVS
 * 
 * @return ESP_OK on success
 */
esp_err_t wifi_erase_credentials(void);

/**
 * Check if WiFi credentials are saved in NVS
 * 
 * @return true if credentials exist
 */
bool wifi_has_saved_credentials(void);

/**
 * Get number of connected stations
 * 
 * @return Number of connected stations
 */
uint8_t wifi_ap_get_connected_stations(void);

/**
 * Get AP IP address
 * 
 * @param ip_info Pointer to store IP info
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_ap_get_ip_info(esp_netif_ip_info_t *ip_info);

#ifdef __cplusplus
}
#endif

#endif // WIFI_AP_MANAGER_H
