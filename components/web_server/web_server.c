/**
 * Web Server Component - Extended with WiFi Management
 * 
 * Provides HTTP server for game control and WiFi configuration
 * 
 * @author ninharp
 * @date 2025-01-07
 */

#include "web_server.h"
#include "wifi_ap_manager.h"
#include "game_logic.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include "cJSON.h"

static const char *TAG = "WEB_SERVER";

static httpd_handle_t server = NULL;
static game_control_callback_t game_callback = NULL;
static char cached_status[512] = "";

// Embedded HTML file
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

/**
 * Root handler - serve HTML page
 */
static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    const size_t index_html_len = index_html_end - index_html_start;
    return httpd_resp_send(req, (const char *)index_html_start, index_html_len);
}

/**
 * Status handler - return game status as JSON
 */
static esp_err_t status_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    
    // Get current game state and player data
    player_data_t player_data;
    game_state_t state = game_get_state();
    
    const char *state_str = "IDLE";
    switch (state) {
        case GAME_STATE_IDLE: state_str = "IDLE"; break;
        case GAME_STATE_READY: state_str = "READY"; break;
        case GAME_STATE_COUNTDOWN: state_str = "COUNTDOWN"; break;
        case GAME_STATE_RUNNING: state_str = "RUNNING"; break;
        case GAME_STATE_PENALTY: state_str = "PENALTY"; break;
        case GAME_STATE_PAUSED: state_str = "PAUSED"; break;
        case GAME_STATE_COMPLETE: state_str = "COMPLETE"; break;
        case GAME_STATE_ERROR: state_str = "ERROR"; break;
    }
    
    if (game_get_player_data(&player_data) == ESP_OK && 
        (state == GAME_STATE_RUNNING || state == GAME_STATE_PAUSED || state == GAME_STATE_PENALTY)) {
        // Calculate time remaining only during active game states
        uint32_t elapsed = (player_data.end_time > 0) ? 
            (player_data.end_time - player_data.start_time) : 
            ((uint32_t)(esp_timer_get_time() / 1000) - player_data.start_time);
        uint32_t time_remaining = (180000 > elapsed) ? (180000 - elapsed) / 1000 : 0;
        
        snprintf(cached_status, sizeof(cached_status),
                 "{\"state\":\"%s\",\"lives\":3,\"score\":%ld,\"time_remaining\":%lu,\"current_level\":1,\"beam_breaks\":%d}",
                 state_str, player_data.score, time_remaining, player_data.beam_breaks);
    } else if (game_get_player_data(&player_data) == ESP_OK) {
        // Show score but time_remaining = 0 when not running
        snprintf(cached_status, sizeof(cached_status),
                 "{\"state\":\"%s\",\"lives\":3,\"score\":%ld,\"time_remaining\":0,\"current_level\":1,\"beam_breaks\":%d}",
                 state_str, player_data.score, player_data.beam_breaks);
    } else {
        snprintf(cached_status, sizeof(cached_status),
                 "{\"state\":\"%s\",\"lives\":3,\"score\":0,\"time_remaining\":0,\"current_level\":1,\"beam_breaks\":0}",
                 state_str);
    }
    
    httpd_resp_send(req, cached_status, strlen(cached_status));
    return ESP_OK;
}

/**
 * WiFi scan handler
 */
static esp_err_t wifi_scan_handler(httpd_req_t *req)
{
    wifi_scan_result_t results[20];
    size_t num_found = 0;
    
    esp_err_t ret = wifi_scan_networks(results, 20, &num_found);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Scan failed");
        return ESP_FAIL;
    }
    
    cJSON *root = cJSON_CreateObject();
    cJSON *networks = cJSON_CreateArray();
    
    for (size_t i = 0; i < num_found; i++) {
        cJSON *network = cJSON_CreateObject();
        cJSON_AddStringToObject(network, "ssid", results[i].ssid);
        cJSON_AddNumberToObject(network, "rssi", results[i].rssi);
        cJSON_AddNumberToObject(network, "authmode", results[i].authmode);
        cJSON_AddNumberToObject(network, "channel", results[i].channel);
        cJSON_AddItemToArray(networks, network);
    }
    
    cJSON_AddItemToObject(root, "networks", networks);
    cJSON_AddNumberToObject(root, "count", num_found);
    
    char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    cJSON_free(json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

/**
 * WiFi connect handler
 */
static esp_err_t wifi_connect_handler(httpd_req_t *req)
{
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request");
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *ssid_json = cJSON_GetObjectItem(root, "ssid");
    cJSON *password_json = cJSON_GetObjectItem(root, "password");
    cJSON *save_json = cJSON_GetObjectItem(root, "save");
    
    if (!ssid_json || !cJSON_IsString(ssid_json)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing SSID");
        return ESP_FAIL;
    }
    
    const char *ssid = cJSON_GetStringValue(ssid_json);
    const char *password = password_json ? cJSON_GetStringValue(password_json) : NULL;
    bool save = save_json ? cJSON_IsTrue(save_json) : false;
    
    ESP_LOGI(TAG, "Connecting to WiFi: %s (save=%d)", ssid, save);
    
    esp_err_t connect_ret = wifi_connect_sta(ssid, password, save);
    
    cJSON *response = cJSON_CreateObject();
    if (connect_ret == ESP_OK) {
        cJSON_AddStringToObject(response, "message", "Connected successfully");
        cJSON_AddBoolToObject(response, "success", true);
    } else {
        cJSON_AddStringToObject(response, "message", "Connection failed");
        cJSON_AddBoolToObject(response, "success", false);
    }
    
    char *json_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    cJSON_free(json_str);
    cJSON_Delete(response);
    cJSON_Delete(root);
    
    return ESP_OK;
}

/**
 * WiFi status handler
 */
static esp_err_t wifi_status_handler(httpd_req_t *req)
{
    wifi_sta_status_t status = wifi_get_sta_status();
    esp_netif_ip_info_t ip_info;
    
    cJSON *root = cJSON_CreateObject();
    
    const char *status_str;
    switch (status) {
        case WIFI_STATUS_CONNECTED:
            status_str = "Connected";
            break;
        case WIFI_STATUS_CONNECTING:
            status_str = "Connecting";
            break;
        case WIFI_STATUS_FAILED:
            status_str = "Failed";
            break;
        default:
            status_str = "Disconnected";
    }
    
    cJSON_AddStringToObject(root, "status", status_str);
    cJSON_AddBoolToObject(root, "connected", status == WIFI_STATUS_CONNECTED);
    
    if (status == WIFI_STATUS_CONNECTED && wifi_get_sta_ip(&ip_info) == ESP_OK) {
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
        cJSON_AddStringToObject(root, "ip", ip_str);
        // Note: SSID would need to be stored separately
        cJSON_AddStringToObject(root, "ssid", "Connected Network");
    }
    
    char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    cJSON_free(json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

/**
 * WiFi disconnect handler
 */
static esp_err_t wifi_disconnect_handler(httpd_req_t *req)
{
    esp_err_t ret = wifi_disconnect_sta();
    
    cJSON *root = cJSON_CreateObject();
    if (ret == ESP_OK) {
        cJSON_AddStringToObject(root, "message", "Disconnected");
        cJSON_AddBoolToObject(root, "success", true);
    } else {
        cJSON_AddStringToObject(root, "message", "Disconnect failed");
        cJSON_AddBoolToObject(root, "success", false);
    }
    
    char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    cJSON_free(json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

/**
 * Game control handler
 */
static esp_err_t game_control_handler(httpd_req_t *req)
{
    // Extract command from URI
    const char *uri = req->uri;
    const char *command = strrchr(uri, '/');
    if (command) {
        command++; // Skip '/'
    }
    
    ESP_LOGI(TAG, "Game control: %s", command);
    
    if (game_callback) {
        esp_err_t ret = game_callback(command, NULL);
        if (ret == ESP_OK) {
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"message\":\"OK\"}", HTTPD_RESP_USE_STRLEN);
        } else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Command failed");
        }
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No callback registered");
    }

    return ESP_OK;
}

/**
 * Laser units list handler - GET /api/units
 */
static esp_err_t units_list_handler(httpd_req_t *req)
{
    extern esp_err_t game_get_laser_units(laser_unit_info_t *units, size_t max_units, size_t *unit_count);
    
    laser_unit_info_t units[10];
    size_t unit_count = 0;
    
    esp_err_t ret = game_get_laser_units(units, 10, &unit_count);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get units");
        return ESP_OK;
    }
    
    cJSON *root = cJSON_CreateObject();
    cJSON *units_array = cJSON_CreateArray();
    
    for (size_t i = 0; i < unit_count; i++) {
        cJSON *unit = cJSON_CreateObject();
        cJSON_AddNumberToObject(unit, "id", units[i].module_id);
        
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 units[i].mac_addr[0], units[i].mac_addr[1], units[i].mac_addr[2],
                 units[i].mac_addr[3], units[i].mac_addr[4], units[i].mac_addr[5]);
        cJSON_AddStringToObject(unit, "mac", mac_str);
        
        cJSON_AddBoolToObject(unit, "online", units[i].is_online);
        cJSON_AddBoolToObject(unit, "laser_on", units[i].laser_on);
        cJSON_AddNumberToObject(unit, "rssi", units[i].rssi);
        cJSON_AddStringToObject(unit, "status", units[i].status);
        cJSON_AddNumberToObject(unit, "last_seen", units[i].last_seen);
        
        cJSON_AddItemToArray(units_array, unit);
    }
    
    cJSON_AddItemToObject(root, "units", units_array);
    cJSON_AddNumberToObject(root, "count", unit_count);
    
    char *json_string = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, HTTPD_RESP_USE_STRLEN);
    
    cJSON_free(json_string);
    cJSON_Delete(root);
    
    return ESP_OK;
}

/**
 * Laser unit control handler - POST /api/units/control
 */
static esp_err_t units_control_handler(httpd_req_t *req)
{
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request");
        return ESP_OK;
    }
    buf[ret] = '\0';
    
    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_OK;
    }
    
    cJSON *id_item = cJSON_GetObjectItem(json, "id");
    cJSON *action_item = cJSON_GetObjectItem(json, "action");
    
    if (!id_item || !action_item) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing id or action");
        return ESP_OK;
    }
    
    uint8_t module_id = (uint8_t)id_item->valueint;
    const char *action = action_item->valuestring;
    
    extern esp_err_t game_control_laser(uint8_t module_id, bool laser_on, uint8_t intensity);
    extern esp_err_t game_reset_laser_unit(uint8_t module_id);
    
    esp_err_t result = ESP_OK;
    
    if (strcmp(action, "laser_on") == 0) {
        cJSON *intensity_item = cJSON_GetObjectItem(json, "intensity");
        uint8_t intensity = intensity_item ? (uint8_t)intensity_item->valueint : 100;
        result = game_control_laser(module_id, true, intensity);
    } else if (strcmp(action, "laser_off") == 0) {
        result = game_control_laser(module_id, false, 0);
    } else if (strcmp(action, "reset") == 0) {
        result = game_reset_laser_unit(module_id);
    } else {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown action");
        return ESP_OK;
    }
    
    cJSON_Delete(json);
    
    if (result == ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"message\":\"OK\"}", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Control failed");
    }
    
    return ESP_OK;
}

/**
 * Initialize web server
 */
esp_err_t web_server_init(httpd_handle_t *server_out, game_control_callback_t callback)
{
    if (server != NULL) {
        ESP_LOGW(TAG, "Web server already initialized");
        return ESP_OK;
    }
    
    game_callback = callback;
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.stack_size = 8192;
    
    ESP_LOGI(TAG, "Starting web server on port %d", config.server_port);
    
    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Register URI handlers
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler
    };
    httpd_register_uri_handler(server, &root_uri);
    
    httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_handler
    };
    httpd_register_uri_handler(server, &status_uri);
    
    // WiFi endpoints
    httpd_uri_t wifi_scan_uri = {
        .uri = "/api/wifi/scan",
        .method = HTTP_GET,
        .handler = wifi_scan_handler
    };
    httpd_register_uri_handler(server, &wifi_scan_uri);
    
    httpd_uri_t wifi_connect_uri = {
        .uri = "/api/wifi/connect",
        .method = HTTP_POST,
        .handler = wifi_connect_handler
    };
    httpd_register_uri_handler(server, &wifi_connect_uri);
    
    httpd_uri_t wifi_status_uri = {
        .uri = "/api/wifi/status",
        .method = HTTP_GET,
        .handler = wifi_status_handler
    };
    httpd_register_uri_handler(server, &wifi_status_uri);
    
    httpd_uri_t wifi_disconnect_uri = {
        .uri = "/api/wifi/disconnect",
        .method = HTTP_POST,
        .handler = wifi_disconnect_handler
    };
    httpd_register_uri_handler(server, &wifi_disconnect_uri);
    
    // Game control endpoints
    httpd_uri_t game_start_uri = {
        .uri = "/api/game/start",
        .method = HTTP_POST,
        .handler = game_control_handler
    };
    httpd_register_uri_handler(server, &game_start_uri);
    
    httpd_uri_t game_stop_uri = {
        .uri = "/api/game/stop",
        .method = HTTP_POST,
        .handler = game_control_handler
    };
    httpd_register_uri_handler(server, &game_stop_uri);
    
    httpd_uri_t game_pause_uri = {
        .uri = "/api/game/pause",
        .method = HTTP_POST,
        .handler = game_control_handler
    };
    httpd_register_uri_handler(server, &game_pause_uri);
    
    httpd_uri_t game_resume_uri = {
        .uri = "/api/game/resume",
        .method = HTTP_POST,
        .handler = game_control_handler
    };
    httpd_register_uri_handler(server, &game_resume_uri);
    
    // Unit management endpoints
    httpd_uri_t units_list_uri = {
        .uri = "/api/units",
        .method = HTTP_GET,
        .handler = units_list_handler
    };
    httpd_register_uri_handler(server, &units_list_uri);
    
    httpd_uri_t units_control_uri = {
        .uri = "/api/units/control",
        .method = HTTP_POST,
        .handler = units_control_handler
    };
    httpd_register_uri_handler(server, &units_control_uri);
    
    if (server_out) {
        *server_out = server;
    }
    
    ESP_LOGI(TAG, "Web server started successfully");
    return ESP_OK;
}

/**
 * Update game status cache
 */
void web_server_update_status(const game_status_t *status)
{
    if (!status) return;
    
    snprintf(cached_status, sizeof(cached_status),
             "{\"state\":\"%s\",\"lives\":%d,\"score\":%d,\"time_remaining\":%d,\"current_level\":%d}",
             status->state, status->lives, status->score,
             status->time_remaining, status->current_level);
}

/**
 * Stop web server
 */
esp_err_t web_server_stop(void)
{
    if (server == NULL) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping web server");
    esp_err_t ret = httpd_stop(server);
    server = NULL;
    game_callback = NULL;
    
    return ret;
}
