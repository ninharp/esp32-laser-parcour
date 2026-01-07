/**
 * WiFi Manager - Extended AP + STA Management
 * 
 * Supports Access Point, Station, and APSTA modes with:
 * - Automatic fallback to AP if STA connection fails
 * - NVS credential storage
 * - WiFi network scanning
 * - Web-based configuration
 * 
 * @author ninharp
 * @date 2025-01-07
 */

#include <string.h>
#include "wifi_ap_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "WIFI_MGR";

// NVS Configuration
#define NVS_NAMESPACE "wifi_config"
#define NVS_KEY_SSID "sta_ssid"
#define NVS_KEY_PASSWORD "sta_pass"

// Event group bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// State variables
static esp_netif_t *ap_netif = NULL;
static esp_netif_t *sta_netif = NULL;
static bool is_initialized = false;
static wifi_manager_mode_t current_mode = WIFI_MANAGER_MODE_AP;
static wifi_sta_status_t sta_status = WIFI_STATUS_DISCONNECTED;
static EventGroupHandle_t wifi_event_group;
static uint8_t connection_retries = 0;
static const uint8_t max_retries = 5;

/**
 * Save WiFi credentials to NVS
 */
static esp_err_t save_wifi_credentials(const char *ssid, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_str(nvs_handle, NVS_KEY_SSID, ssid);
    if (ret == ESP_OK) {
        ret = nvs_set_str(nvs_handle, NVS_KEY_PASSWORD, password);
    }
    
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }
    
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi credentials saved to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to save credentials: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/**
 * Load WiFi credentials from NVS
 */
static esp_err_t load_wifi_credentials(char *ssid, size_t ssid_len, char *password, size_t pass_len)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    size_t required_size = ssid_len;
    ret = nvs_get_str(nvs_handle, NVS_KEY_SSID, ssid, &required_size);
    
    if (ret == ESP_OK) {
        required_size = pass_len;
        ret = nvs_get_str(nvs_handle, NVS_KEY_PASSWORD, password, &required_size);
    }
    
    nvs_close(nvs_handle);
    return ret;
}

/**
 * WiFi event handler
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                        MAC2STR(event->mac), event->aid);
                break;
            }
            
            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d",
                        MAC2STR(event->mac), event->aid);
                break;
            }
            
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA started, attempting connection...");
                sta_status = WIFI_STATUS_CONNECTING;
                esp_wifi_connect();
                break;
            
            case WIFI_EVENT_STA_DISCONNECTED:
                if (connection_retries < max_retries) {
                    esp_wifi_connect();
                    connection_retries++;
                    ESP_LOGI(TAG, "Retry connection %d/%d", connection_retries, max_retries);
                } else {
                    xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
                    sta_status = WIFI_STATUS_FAILED;
                    ESP_LOGW(TAG, "Connection failed after %d retries", max_retries);
                }
                break;
                
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "STA got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            connection_retries = 0;
            sta_status = WIFI_STATUS_CONNECTED;
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

/**
 * Initialize WiFi Access Point
 */
esp_err_t wifi_ap_init(const laser_ap_config_t *config)
{
    if (!config) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing WiFi AP mode...");
    
    // Create WiFi event group if not exists
    if (!wifi_event_group) {
        wifi_event_group = xEventGroupCreate();
    }

    // Create default WiFi AP netif
    if (!ap_netif) {
        ap_netif = esp_netif_create_default_wifi_ap();
        if (!ap_netif) {
            ESP_LOGE(TAG, "Failed to create AP netif");
            return ESP_FAIL;
        }
    }

    // Initialize WiFi if not already done
    if (!is_initialized) {
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
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));
    }

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
    
    // Copy SSID and password
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

    // Check current WiFi mode to preserve APSTA if already set
    wifi_mode_t current_wifi_mode;
    esp_wifi_get_mode(&current_wifi_mode);
    
    // Only change mode if not already in APSTA mode (for ESP-NOW compatibility)
    if (current_wifi_mode != WIFI_MODE_APSTA) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    } else {
        ESP_LOGI(TAG, "Preserving APSTA mode for ESP-NOW");
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    
    // Only start WiFi if not already started
    if (!is_initialized) {
        ESP_ERROR_CHECK(esp_wifi_start());
    }

    ESP_LOGI(TAG, "WiFi AP started: SSID=%s, Channel=%d, Max connections=%d",
             config->ssid, config->channel, config->max_connection);

    is_initialized = true;
    current_mode = WIFI_MANAGER_MODE_AP;
    return ESP_OK;
}

/**
 * Scan for available WiFi networks
 */
esp_err_t wifi_scan_networks(wifi_scan_result_t *results, size_t max_results, size_t *num_found)
{
    if (!results || !num_found) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Starting WiFi scan...");
    
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 300
            }
        }
    };
    
    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    
    if (ap_count == 0) {
        *num_found = 0;
        return ESP_OK;
    }
    
    wifi_ap_record_t *ap_info = calloc(ap_count, sizeof(wifi_ap_record_t));
    if (!ap_info) {
        return ESP_ERR_NO_MEM;
    }
    
    ret = esp_wifi_scan_get_ap_records(&ap_count, ap_info);
    if (ret != ESP_OK) {
        free(ap_info);
        return ret;
    }
    
    size_t count = ap_count < max_results ? ap_count : max_results;
    for (size_t i = 0; i < count; i++) {
        strncpy(results[i].ssid, (char*)ap_info[i].ssid, 32);
        results[i].ssid[32] = '\0';
        results[i].rssi = ap_info[i].rssi;
        results[i].authmode = ap_info[i].authmode;
        results[i].channel = ap_info[i].primary;
    }
    
    *num_found = count;
    free(ap_info);
    
    ESP_LOGI(TAG, "Scan complete, found %d networks", count);
    return ESP_OK;
}

/**
 * Connect to WiFi network as Station
 */
esp_err_t wifi_connect_sta(const char *ssid, const char *password, bool save_to_nvs)
{
    if (!ssid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);
    
    // First, scan to find the target network's channel
    uint8_t target_channel = 0;
    wifi_scan_result_t scan_results[20];
    size_t num_found = 0;
    
    esp_err_t scan_ret = wifi_scan_networks(scan_results, 20, &num_found);
    if (scan_ret == ESP_OK) {
        for (size_t i = 0; i < num_found; i++) {
            if (strcmp(scan_results[i].ssid, ssid) == 0) {
                target_channel = scan_results[i].channel;
                ESP_LOGI(TAG, "Target WiFi '%s' found on channel %d", ssid, target_channel);
                break;
            }
        }
    }
    
    // If target channel found and different from current, broadcast channel change
    if (target_channel > 0) {
        uint8_t current_channel;
        wifi_second_chan_t second;
        esp_wifi_get_channel(&current_channel, &second);
        
        if (current_channel != target_channel) {
            ESP_LOGI(TAG, "Channel change required: %d -> %d", current_channel, target_channel);
            ESP_LOGI(TAG, "Broadcasting channel change to all ESP-NOW peers...");
            
            // Broadcast channel change via ESP-NOW (needs to be called from main.c)
            // For now, we'll add a callback mechanism
            extern esp_err_t notify_channel_change(uint8_t new_channel);
            notify_channel_change(target_channel);
        }
    }
    
    // Create STA netif if not exists
    if (!sta_netif) {
        sta_netif = esp_netif_create_default_wifi_sta();
        if (!sta_netif) {
            ESP_LOGE(TAG, "Failed to create STA netif");
            return ESP_FAIL;
        }
    }
    
    // Configure WiFi STA
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    // Set WiFi mode based on current state
    wifi_mode_t mode = (current_mode == WIFI_MANAGER_MODE_AP) ? WIFI_MODE_APSTA : WIFI_MODE_STA;
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // Reset connection state
    connection_retries = 0;
    sta_status = WIFI_STATUS_CONNECTING;
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    
    // Start or reconnect
    if (!is_initialized) {
        ESP_ERROR_CHECK(esp_wifi_start());
        is_initialized = true;
    } else {
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    
    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(10000));
    
    esp_err_t ret;
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to WiFi: %s", ssid);
        current_mode = (mode == WIFI_MODE_APSTA) ? WIFI_MANAGER_MODE_APSTA : WIFI_MANAGER_MODE_STA;
        
        if (save_to_nvs) {
            save_wifi_credentials(ssid, password ? password : "");
        }
        ret = ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to connect to WiFi: %s", ssid);
        ret = ESP_FAIL;
    }
    
    return ret;
}

/**
 * Disconnect from WiFi station
 */
esp_err_t wifi_disconnect_sta(void)
{
    ESP_LOGI(TAG, "Disconnecting from WiFi station");
    sta_status = WIFI_STATUS_DISCONNECTED;
    
    esp_err_t ret = esp_wifi_disconnect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Disconnect failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/**
 * Get WiFi station connection status
 */
wifi_sta_status_t wifi_get_sta_status(void)
{
    return sta_status;
}

/**
 * Get station IP info
 */
esp_err_t wifi_get_sta_ip(esp_netif_ip_info_t *ip_info)
{
    if (!ip_info || !sta_netif) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return esp_netif_get_ip_info(sta_netif, ip_info);
}

/**
 * Get AP connected stations count
 */
uint8_t wifi_ap_get_connected_stations(void)
{
    wifi_sta_list_t sta_list;
    esp_wifi_ap_get_sta_list(&sta_list);
    return sta_list.num;
}

/**
 * Get AP IP info
 */
esp_err_t wifi_ap_get_ip_info(esp_netif_ip_info_t *ip_info)
{
    if (!ip_info || !ap_netif) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return esp_netif_get_ip_info(ap_netif, ip_info);
}

/**
 * Connect with automatic fallback to AP
 */
esp_err_t wifi_connect_with_fallback(const laser_ap_config_t *ap_config, uint32_t timeout_ms)
{
    if (!ap_config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Attempting WiFi connection with fallback...");
    
    // Try to load saved credentials
    char saved_ssid[33] = {0};
    char saved_password[64] = {0};
    
    esp_err_t ret = load_wifi_credentials(saved_ssid, sizeof(saved_ssid), 
                                         saved_password, sizeof(saved_password));
    
    if (ret == ESP_OK && strlen(saved_ssid) > 0) {
        ESP_LOGI(TAG, "Found saved WiFi credentials for: %s", saved_ssid);
        
        // Try to connect to saved network
        ret = wifi_connect_sta(saved_ssid, saved_password, false);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Successfully connected to saved WiFi");
            return ESP_OK;
        }
        
        ESP_LOGW(TAG, "Failed to connect to saved WiFi, falling back to AP mode");
    } else {
        ESP_LOGI(TAG, "No saved WiFi credentials found");
    }
    
    // Fallback: Start AP mode
    ESP_LOGI(TAG, "Starting AP mode as fallback");
    ret = wifi_ap_init(ap_config);
    
    return (ret == ESP_OK) ? ESP_FAIL : ret; // ESP_FAIL indicates fallback active
}

/**
 * Erase saved WiFi credentials
 */
esp_err_t wifi_erase_credentials(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    nvs_erase_key(nvs_handle, NVS_KEY_SSID);
    nvs_erase_key(nvs_handle, NVS_KEY_PASSWORD);
    ret = nvs_commit(nvs_handle);
    
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "WiFi credentials erased");
    return ret;
}

/**
 * Check if credentials are saved
 */
bool wifi_has_saved_credentials(void)
{
    char ssid[33] = {0};
    char password[64] = {0};
    
    esp_err_t ret = load_wifi_credentials(ssid, sizeof(ssid), password, sizeof(password));
    return (ret == ESP_OK && strlen(ssid) > 0);
}

/**
 * Deinitialize WiFi
 */
esp_err_t wifi_ap_deinit(void)
{
    if (!is_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing WiFi");
    
    esp_wifi_stop();
    esp_wifi_deinit();
    
    if (ap_netif) {
        esp_netif_destroy(ap_netif);
        ap_netif = NULL;
    }
    
    if (sta_netif) {
        esp_netif_destroy(sta_netif);
        sta_netif = NULL;
    }
    
    is_initialized = false;
    sta_status = WIFI_STATUS_DISCONNECTED;
    
    return ESP_OK;
}
