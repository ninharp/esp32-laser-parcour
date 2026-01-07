/**
 * Web Server Component - Header
 * 
 * HTTP server for web interface and REST API.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Web server configuration
 */
typedef struct {
    uint16_t port;              // HTTP port (default 80)
    uint16_t max_uri_handlers;  // Max URI handlers
    uint16_t max_open_sockets;  // Max simultaneous connections
} web_server_config_t;

/**
 * Game control callback function
 * 
 * @param command Command string ("start", "stop", "pause", "resume")
 * @param data Additional data (optional)
 * @return ESP_OK on success
 */
typedef esp_err_t (*game_control_callback_t)(const char *command, const char *data);

/**
 * Initialize web server
 * 
 * @param config Server configuration (NULL for defaults)
 * @param game_callback Game control callback
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t web_server_init(const web_server_config_t *config, 
                          game_control_callback_t game_callback);

/**
 * Deinitialize web server
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t web_server_deinit(void);

/**
 * Update game status (for live updates)
 * 
 * @param json_status JSON string with game status
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t web_server_update_status(const char *json_status);

#ifdef __cplusplus
}
#endif

#endif // WEB_SERVER_H
