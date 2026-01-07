/**
 * Web Server Component
 * 
 * HTTP server for web interface and REST API.
 * 
 * @author ninharp
 * @date 2025
 */

#include "web_server.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "WEB_SERVER";

static httpd_handle_t server = NULL;
static game_control_callback_t game_callback = NULL;
static char cached_status[512] = "";

// Simple HTML page
static const char* html_page = 
    "<!DOCTYPE html>"
    "<html><head><title>Laser Parcour</title>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>body{font-family:Arial;max-width:600px;margin:50px auto;padding:20px}"
    "button{padding:15px 30px;margin:10px;font-size:16px;cursor:pointer}</style>"
    "</head><body>"
    "<h1>ESP32 Laser Parcour</h1>"
    "<div id='status'><p>Status: Ready</p></div>"
    "<button onclick='sendCmd(\"start\")'>Start Game</button>"
    "<button onclick='sendCmd(\"stop\")'>Stop Game</button>"
    "<button onclick='sendCmd(\"pause\")'>Pause</button>"
    "<button onclick='sendCmd(\"resume\")'>Resume</button>"
    "<script>"
    "function sendCmd(cmd){"
    "  fetch('/api/game/'+cmd,{method:'POST'})"
    "    .then(r=>r.json()).then(d=>alert(d.message))"
    "}"
    "setInterval(()=>{"
    "  fetch('/api/status').then(r=>r.json())"
    "    .then(d=>document.getElementById('status').innerHTML='<pre>'+JSON.stringify(d,null,2)+'</pre>')"
    "},1000)"
    "</script></body></html>";

/**
 * Root handler
 */
static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/**
 * Status API handler
 */
static esp_err_t status_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    if (strlen(cached_status) > 0) {
        httpd_resp_send(req, cached_status, HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send(req, "{\"status\":\"idle\"}", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

/**
 * Game control API handler
 */
static esp_err_t game_control_handler(httpd_req_t *req)
{
    char command[32];
    const char *uri = req->uri;
    
    // Extract command from URI (/api/game/start -> "start")
    const char *cmd_start = strrchr(uri, '/');
    if (cmd_start) {
        strncpy(command, cmd_start + 1, sizeof(command) - 1);
        command[sizeof(command) - 1] = '\0';
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid command");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Game control command: %s", command);

    // Call game callback if registered
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
 * Initialize web server
 */
esp_err_t web_server_init(const web_server_config_t *config, 
                          game_control_callback_t callback)
{
    if (server != NULL) {
        ESP_LOGW(TAG, "Web server already running");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing web server...");

    game_callback = callback;

    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    if (config) {
        server_config.server_port = config->port;
        server_config.max_uri_handlers = config->max_uri_handlers;
        server_config.max_open_sockets = config->max_open_sockets;
    }

    esp_err_t ret = httpd_start(&server, &server_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register URI handlers
    httpd_uri_t root_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &root_uri);

    httpd_uri_t status_uri = {
        .uri       = "/api/status",
        .method    = HTTP_GET,
        .handler   = status_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &status_uri);

    httpd_uri_t game_start_uri = {
        .uri       = "/api/game/start",
        .method    = HTTP_POST,
        .handler   = game_control_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &game_start_uri);

    httpd_uri_t game_stop_uri = {
        .uri       = "/api/game/stop",
        .method    = HTTP_POST,
        .handler   = game_control_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &game_stop_uri);

    httpd_uri_t game_pause_uri = {
        .uri       = "/api/game/pause",
        .method    = HTTP_POST,
        .handler   = game_control_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &game_pause_uri);

    httpd_uri_t game_resume_uri = {
        .uri       = "/api/game/resume",
        .method    = HTTP_POST,
        .handler   = game_control_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &game_resume_uri);

    ESP_LOGI(TAG, "Web server started on port %d", server_config.server_port);
    return ESP_OK;
}

/**
 * Deinitialize web server
 */
esp_err_t web_server_deinit(void)
{
    if (server == NULL) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping web server...");
    httpd_stop(server);
    server = NULL;
    game_callback = NULL;

    return ESP_OK;
}

/**
 * Update game status
 */
esp_err_t web_server_update_status(const char *json_status)
{
    if (!json_status) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(cached_status, json_status, sizeof(cached_status) - 1);
    cached_status[sizeof(cached_status) - 1] = '\0';

    return ESP_OK;
}
