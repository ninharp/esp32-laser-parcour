/**
 * WiFi Access Point Manager Component
 * 
 * Manages WiFi Access Point for Main Unit.
 * 
 * @author ninharp
 * @date 2025
 */

#include <string.h>
#include "wifi_ap_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

static const char *TAG = "WIFI_AP";

static esp_netif_t *ap_netif = NULL;
static bool is_initialized = false;

/**
 * WiFi event handler
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

/**
 * Initialize WiFi Access Point
 */
esp_err_t wifi_ap_init(const laser_ap_config_t *config)
{
    if (is_initialized) {
        ESP_LOGW(TAG, "WiFi AP already initialized");
        return ESP_OK;
    }

    if (!config) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing WiFi Access Point...");

    // Create default WiFi AP netif
    ap_netif = esp_netif_create_default_wifi_ap();
    if (!ap_netif) {
        ESP_LOGE(TAG, "Failed to create netif");
        return ESP_FAIL;
    }

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    // Configure WiFi AP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(config->ssid),
            .channel = config->channel,
            .max_connection = config->max_connection,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    
    // Copy SSID and password (explicit null termination to avoid strncpy warnings)
    memset(wifi_config.ap.ssid, 0, sizeof(wifi_config.ap.ssid));
    memset(wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy((char*)wifi_config.ap.ssid, config->ssid, sizeof(wifi_config.ap.ssid) - 1);
    strncpy((char*)wifi_config.ap.password, config->password, sizeof(wifi_config.ap.password) - 1);
    #pragma GCC diagnostic pop
    
    if (strlen(config->password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started: SSID=%s, Channel=%d, Max connections=%d",
             config->ssid, config->channel, config->max_connection);

    is_initialized = true;
    return ESP_OK;
}

/**
 * Deinitialize WiFi Access Point
 */
esp_err_t wifi_ap_deinit(void)
{
    if (!is_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing WiFi AP...");
    
    esp_wifi_stop();
    esp_wifi_deinit();
    
    if (ap_netif) {
        esp_netif_destroy(ap_netif);
        ap_netif = NULL;
    }

    is_initialized = false;
    return ESP_OK;
}

/**
 * Get number of connected stations
 */
esp_err_t wifi_ap_get_connected_stations(uint8_t *count)
{
    if (!is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!count) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_sta_list_t sta_list;
    esp_err_t ret = esp_wifi_ap_get_sta_list(&sta_list);
    if (ret == ESP_OK) {
        *count = sta_list.num;
    }

    return ret;
}

/**
 * Get AP IP address
 */
esp_err_t wifi_ap_get_ip_info(esp_netif_ip_info_t *ip_info)
{
    if (!is_initialized || !ap_netif) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!ip_info) {
        return ESP_ERR_INVALID_ARG;
    }

    return esp_netif_get_ip_info(ap_netif, ip_info);
}
