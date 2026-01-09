/**
 * Web Server Component - Extended with WiFi Management and SD Card Support
 * 
 * Provides HTTP server for game control and WiFi configuration
 * Supports serving web files from SD card (/sdcard/web/) with fallback to internal files
 * 
 * @author ninharp
 * @date 2025-01-09
 */

#include "web_server.h"
#include "wifi_ap_manager.h"
#include "game_logic.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "cJSON.h"

#ifdef CONFIG_ENABLE_SD_CARD
#include "sd_card_manager.h"
#endif

// External sound API handlers from sound_api.c
extern esp_err_t sound_mappings_handler(httpd_req_t *req);
extern esp_err_t sound_mapping_set_handler(httpd_req_t *req);
extern esp_err_t sound_files_handler(httpd_req_t *req);
extern esp_err_t sound_upload_handler(httpd_req_t *req);
extern esp_err_t sound_delete_handler(httpd_req_t *req);
extern esp_err_t sound_play_handler(httpd_req_t *req);
extern esp_err_t sound_stop_handler(httpd_req_t *req);
extern esp_err_t sound_volume_get_handler(httpd_req_t *req);
extern esp_err_t sound_volume_set_handler(httpd_req_t *req);
extern esp_err_t sounds_page_handler(httpd_req_t *req);

static const char *TAG = "WEB_SERVER";

static httpd_handle_t server = NULL;
static game_control_callback_t game_callback = NULL;
static char cached_status[512] = "";
static bool use_sd_card_web = false;  // Flag für SD-Karten Web-Interface

// Embedded HTML file (Fallback)
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

/**
 * Helper: Get MIME type from file extension
 */
static const char *get_mime_type(const char *filename)
{
    if (strstr(filename, ".html")) return "text/html";
    if (strstr(filename, ".css")) return "text/css";
    if (strstr(filename, ".js")) return "application/javascript";
    if (strstr(filename, ".json")) return "application/json";
    if (strstr(filename, ".png")) return "image/png";
    if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) return "image/jpeg";
    if (strstr(filename, ".gif")) return "image/gif";
    if (strstr(filename, ".svg")) return "image/svg+xml";
    if (strstr(filename, ".ico")) return "image/x-icon";
    if (strstr(filename, ".txt")) return "text/plain";
    return "application/octet-stream";
}

/**
 * Root handler - serve HTML page (from SD card or internal)
 */
static esp_err_t root_handler(httpd_req_t *req)
{
#ifdef CONFIG_ENABLE_SD_CARD
    // Versuche von SD-Karte zu laden
    if (use_sd_card_web) {
        char filepath[128];
        snprintf(filepath, sizeof(filepath), "%s/web/index.html", sd_card_get_mount_point());
        
        // Versuche Datei zu öffnen
        FILE *file = fopen(filepath, "r");
        if (file) {
            ESP_LOGI(TAG, "Serving index.html from SD card: %s", filepath);
            httpd_resp_set_type(req, "text/html");
            
            char buffer[512];
            size_t read_bytes;
            while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                if (httpd_resp_send_chunk(req, buffer, read_bytes) != ESP_OK) {
                    fclose(file);
                    httpd_resp_send_chunk(req, NULL, 0);  // Abort chunked send
                    return ESP_FAIL;
                }
            }
            
            fclose(file);
            httpd_resp_send_chunk(req, NULL, 0);  // Finish chunked send
            return ESP_OK;
        }
        
        ESP_LOGW(TAG, "Failed to open %s, falling back to internal HTML", filepath);
    }
#endif
    
    // Fallback: Internes HTML
    httpd_resp_set_type(req, "text/html");
    const size_t index_html_len = index_html_end - index_html_start;
    return httpd_resp_send(req, (const char *)index_html_start, index_html_len);
}

/**
 * Generic file handler for SD card web files
 * Serves any file from /sdcard/web/ directory
 */
#ifdef CONFIG_ENABLE_SD_CARD
static esp_err_t sd_file_handler(httpd_req_t *req)
{
    if (!use_sd_card_web) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    // URI path z.B. "/style.css" → "/sdcard/web/style.css"
    char filepath[1024];  // Größerer Buffer für lange Pfade
    snprintf(filepath, sizeof(filepath), "%s/web%s", sd_card_get_mount_point(), req->uri);
    
    ESP_LOGI(TAG, "Requesting SD card file: %s", filepath);
    
    // Sicherheit: Path traversal verhindern
    if (strstr(filepath, "..")) {
        ESP_LOGW(TAG, "Path traversal attempt blocked: %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    // Prüfen ob Datei existiert
    struct stat st;
    if (stat(filepath, &st) != 0) {
        ESP_LOGW(TAG, "File not found: %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    // Datei öffnen
    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    // MIME-Type setzen
    httpd_resp_set_type(req, get_mime_type(filepath));
    
    // Datei chunked senden
    char buffer[512];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, read_bytes) != ESP_OK) {
            fclose(file);
            httpd_resp_send_chunk(req, NULL, 0);
            return ESP_FAIL;
        }
    }
    
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    
    return ESP_OK;
}
#endif

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
        // Show elapsed time during active game states (counts UP)
        uint32_t elapsed_sec = player_data.elapsed_time / 1000;
        
        snprintf(cached_status, sizeof(cached_status),
                 "{\"state\":\"%s\",\"time_remaining\":%lu,\"beam_breaks\":%d}",
                 state_str, elapsed_sec, player_data.beam_breaks);
    } else if (state == GAME_STATE_COUNTDOWN && game_get_player_data(&player_data) == ESP_OK) {
        // Show countdown remaining time
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
        uint32_t countdown_remaining = 0;
        if (now < player_data.start_time) {
            countdown_remaining = (player_data.start_time - now) / 1000;
        }
        snprintf(cached_status, sizeof(cached_status),
                 "{\"state\":\"%s\",\"countdown\":%lu,\"beam_breaks\":0}",
                 state_str, countdown_remaining);
    } else if (state == GAME_STATE_COMPLETE && game_get_player_data(&player_data) == ESP_OK) {
        // Show final time when complete
        uint32_t elapsed_sec = player_data.elapsed_time / 1000;
        snprintf(cached_status, sizeof(cached_status),
                 "{\"state\":\"%s\",\"time_remaining\":%lu,\"beam_breaks\":%d}",
                 state_str, elapsed_sec, player_data.beam_breaks);
    } else {
        snprintf(cached_status, sizeof(cached_status),
                 "{\"state\":\"%s\",\"time_remaining\":0,\"beam_breaks\":0}",
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
        } else if (ret == ESP_ERR_INVALID_STATE && strcmp(command, "start") == 0) {
            // Special handling for start command with no laser units
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"error\":\"No laser units found. Please check unit connections.\"}", HTTPD_RESP_USE_STRLEN);
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
        
        // Add role information (1=laser, 2=finish)
        cJSON_AddNumberToObject(unit, "role", units[i].role);
        const char *role_str = (units[i].role == 2) ? "finish" : "laser";
        cJSON_AddStringToObject(unit, "role_name", role_str);
        
        cJSON_AddBoolToObject(unit, "online", units[i].is_online);
        cJSON_AddBoolToObject(unit, "laser_on", units[i].laser_on);
        cJSON_AddNumberToObject(unit, "rssi", units[i].rssi);
        cJSON_AddStringToObject(unit, "status", units[i].status);
        cJSON_AddNumberToObject(unit, "last_seen", units[i].last_seen);
        
        cJSON_AddItemToArray(units_array, unit);
    }
    
    cJSON_AddItemToObject(root, "units", units_array);
    cJSON_AddNumberToObject(root, "count", unit_count);
    
    // Add current game state so frontend can disable controls during active game
    extern game_state_t game_get_state(void);
    game_state_t state = game_get_state();
    cJSON_AddNumberToObject(root, "game_state", state);
    bool game_active = (state == GAME_STATE_RUNNING || state == GAME_STATE_COUNTDOWN || 
                       state == GAME_STATE_PENALTY || state == GAME_STATE_PAUSED);
    cJSON_AddBoolToObject(root, "game_active", game_active);
    
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
    
    // Check if game is running - block manual laser control during game
    extern game_state_t game_get_state(void);
    game_state_t state = game_get_state();
    
    // Block laser_on/laser_off during active game (RUNNING, COUNTDOWN, PENALTY, PAUSED)
    if ((strcmp(action, "laser_on") == 0 || strcmp(action, "laser_off") == 0) &&
        (state == GAME_STATE_RUNNING || state == GAME_STATE_COUNTDOWN || 
         state == GAME_STATE_PENALTY || state == GAME_STATE_PAUSED)) {
        cJSON_Delete(json);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Cannot control laser during active game\"}", HTTPD_RESP_USE_STRLEN);
        ESP_LOGW(TAG, "Laser control blocked - game is active (state: %d)", state);
        return ESP_OK;
    }
    
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
    
#ifdef CONFIG_ENABLE_SD_CARD
    // Prüfe ob SD-Karte mit /web Verzeichnis verfügbar ist
    if (sd_card_get_status() == SD_STATUS_MOUNTED && sd_card_has_web_interface()) {
        use_sd_card_web = true;
        ESP_LOGI(TAG, "Using web interface from SD card: %s/web/", sd_card_get_mount_point());
    } else {
        use_sd_card_web = false;
        ESP_LOGI(TAG, "SD card web interface not available, using internal HTML");
    }
#else
    use_sd_card_web = false;
    ESP_LOGI(TAG, "SD card support disabled, using internal HTML");
#endif
    
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
    
    // Sound API endpoints
    httpd_uri_t sounds_page_uri = {
        .uri = "/sounds.html",
        .method = HTTP_GET,
        .handler = sounds_page_handler
    };
    httpd_register_uri_handler(server, &sounds_page_uri);
    
    httpd_uri_t sound_mappings_uri = {
        .uri = "/api/sounds/mappings",
        .method = HTTP_GET,
        .handler = sound_mappings_handler
    };
    httpd_register_uri_handler(server, &sound_mappings_uri);
    
    httpd_uri_t sound_mapping_set_uri = {
        .uri = "/api/sounds/mapping",
        .method = HTTP_POST,
        .handler = sound_mapping_set_handler
    };
    httpd_register_uri_handler(server, &sound_mapping_set_uri);
    
    httpd_uri_t sound_files_uri = {
        .uri = "/api/sounds/files",
        .method = HTTP_GET,
        .handler = sound_files_handler
    };
    httpd_register_uri_handler(server, &sound_files_uri);
    
    httpd_uri_t sound_upload_uri = {
        .uri = "/api/sounds/upload",
        .method = HTTP_POST,
        .handler = sound_upload_handler
    };
    httpd_register_uri_handler(server, &sound_upload_uri);
    
    httpd_uri_t sound_delete_uri = {
        .uri = "/api/sounds/delete",
        .method = HTTP_POST,
        .handler = sound_delete_handler
    };
    httpd_register_uri_handler(server, &sound_delete_uri);
    
    httpd_uri_t sound_play_uri = {
        .uri = "/api/sounds/play",
        .method = HTTP_POST,
        .handler = sound_play_handler
    };
    httpd_register_uri_handler(server, &sound_play_uri);
    
    httpd_uri_t sound_stop_uri = {
        .uri = "/api/sounds/stop",
        .method = HTTP_POST,
        .handler = sound_stop_handler
    };
    httpd_register_uri_handler(server, &sound_stop_uri);
    
    httpd_uri_t sound_volume_get_uri = {
        .uri = "/api/sounds/volume",
        .method = HTTP_GET,
        .handler = sound_volume_get_handler
    };
    httpd_register_uri_handler(server, &sound_volume_get_uri);
    
    httpd_uri_t sound_volume_set_uri = {
        .uri = "/api/sounds/volume",
        .method = HTTP_POST,
        .handler = sound_volume_set_handler
    };
    httpd_register_uri_handler(server, &sound_volume_set_uri);
    
#ifdef CONFIG_ENABLE_SD_CARD
    // Wildcard handler für SD-Karten-Dateien registrieren (NACH allen API-Handlers!)
    // Dadurch werden zuerst /api/* Routen geprüft, dann SD-Card-Dateien
    if (use_sd_card_web) {
        httpd_uri_t sd_file_uri = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = sd_file_handler
        };
        httpd_register_uri_handler(server, &sd_file_uri);
        ESP_LOGI(TAG, "Registered wildcard handler for SD card files");
    }
#endif
    
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
