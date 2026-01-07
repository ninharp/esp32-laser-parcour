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

#ifdef __cplusplus
extern "C" {
#endif

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
 * Get number of connected stations
 * 
 * @param count Pointer to store station count
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_ap_get_connected_stations(uint8_t *count);

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
