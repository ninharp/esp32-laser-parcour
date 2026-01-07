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
 * Game status structure
 */
typedef struct {
    const char *state;
    int lives;
    int score;
    int time_remaining;
    int current_level;
} game_status_t;

/**
 * Initialize web server
 * 
 * @param server_out Pointer to store server handle (can be NULL)
 * @param game_callback Game control callback
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t web_server_init(httpd_handle_t *server_out, 
                          game_control_callback_t game_callback);

/**
 * Stop web server
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t web_server_stop(void);

/**
 * Update game status (for live updates)
 * 
 * @param status Game status structure
 */
void web_server_update_status(const game_status_t *status);

#ifdef __cplusplus
}
#endif

#endif // WEB_SERVER_H
