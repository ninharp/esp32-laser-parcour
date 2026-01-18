/**
 * Sound API Handlers for Web Server
 * 
 * Provides REST API endpoints for sound management.
 */

#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "sound_manager.h"
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

/**
 * GET /api/sounds/mappings - Get current sound event mappings
 */
esp_err_t sound_mappings_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    
    cJSON *root = cJSON_CreateObject();
    cJSON *mappings = cJSON_CreateObject();
    
    for (int i = 0; i < SOUND_EVENT_MAX; i++) {
        const char *filename = sound_manager_get_event_file((sound_event_t)i);
        char key[8];
        snprintf(key, sizeof(key), "%d", i);
        cJSON_AddStringToObject(mappings, key, filename ? filename : "");
    }
    
    cJSON_AddItemToObject(root, "mappings", mappings);
    
    const char *json_str = cJSON_Print(root);
    httpd_resp_sendstr(req, json_str);
    
    free((void*)json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

/**
 * POST /api/sounds/mapping - Set sound file for event
 * Body: {"event": 0, "filename": "startup.mp3"}
 */
esp_err_t sound_mapping_set_handler(httpd_req_t *req)
{
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    int event = cJSON_GetObjectItem(json, "event")->valueint;
    const char *filename = cJSON_GetObjectItem(json, "filename")->valuestring;
    
    esp_err_t err = sound_manager_set_event_file((sound_event_t)event, filename);
    if (err == ESP_OK) {
        sound_manager_save_config();
    }
    
    cJSON_Delete(json);
    
    httpd_resp_set_type(req, "application/json");
    if (err == ESP_OK) {
        httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    } else {
        httpd_resp_sendstr(req, "{\"status\":\"error\"}");
    }
    
    return ESP_OK;
}

/**
 * GET /api/sounds/files - List available sound files
 */
esp_err_t sound_files_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    
    cJSON *root = cJSON_CreateObject();
    cJSON *files_array = cJSON_CreateArray();
    
#ifdef CONFIG_ENABLE_SOUND_MANAGER
    DIR *dir = opendir(CONFIG_SOUND_FILES_PATH);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) {
                const char *ext = strrchr(entry->d_name, '.');
                if (ext && (strcmp(ext, ".mp3") == 0 || strcmp(ext, ".wav") == 0)) {
                    cJSON *file_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(file_obj, "name", entry->d_name);
                    
                    // Get file size
                    char filepath[300];
                    snprintf(filepath, sizeof(filepath), "%s/%s", CONFIG_SOUND_FILES_PATH, entry->d_name);
                    struct stat st;
                    if (stat(filepath, &st) == 0) {
                        cJSON_AddNumberToObject(file_obj, "size", st.st_size);
                    }
                    
                    cJSON_AddItemToArray(files_array, file_obj);
                }
            }
        }
        closedir(dir);
    }
#endif
    
    cJSON_AddItemToObject(root, "files", files_array);
    
    const char *json_str = cJSON_Print(root);
    httpd_resp_sendstr(req, json_str);
    
    free((void*)json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

/**
 * POST /api/sounds/upload - Upload sound file
 */
esp_err_t sound_upload_handler(httpd_req_t *req)
{
    int uploaded = 0;
    char filepath[128];
    FILE *fp = NULL;
    char *buf = NULL;
    
    // Parse multipart form data (simplified - full implementation would parse boundaries)
    // For now, we'll accept raw file data with filename in header
    
    const char *filename = NULL;
    size_t header_len = httpd_req_get_hdr_value_len(req, "X-Filename");
    if (header_len > 0) {
        char *header_val = malloc(header_len + 1);
        if (httpd_req_get_hdr_value_str(req, "X-Filename", header_val, header_len + 1) == ESP_OK) {
            filename = header_val;
        }
    }
    
    if (!filename) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No filename");
        return ESP_FAIL;
    }
    
#ifdef CONFIG_ENABLE_SOUND_MANAGER
    snprintf(filepath, sizeof(filepath), "%s/%s", CONFIG_SOUND_FILES_PATH, filename);
    
    fp = fopen(filepath, "wb");
    if (!fp) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        free((void*)filename);
        return ESP_FAIL;
    }
    
    buf = malloc(1024);
    int remaining = req->content_len;
    
    while (remaining > 0) {
        int recv_len = httpd_req_recv(req, buf, MIN(remaining, 1024));
        if (recv_len <= 0) {
            if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            break;
        }
        
        fwrite(buf, 1, recv_len, fp);
        remaining -= recv_len;
    }
    
    fclose(fp);
    free(buf);
    uploaded = 1;
#endif
    
    free((void*)filename);
    
    httpd_resp_set_type(req, "application/json");
    char resp[64];
    snprintf(resp, sizeof(resp), "{\"uploaded\":%d}", uploaded);
    httpd_resp_sendstr(req, resp);
    
    return ESP_OK;
}

/**
 * POST /api/sounds/delete - Delete sound file
 * Body: {"filename": "sound.mp3"}
 */
esp_err_t sound_delete_handler(httpd_req_t *req)
{
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *filename_item = cJSON_GetObjectItem(json, "filename");
    if (!filename_item || !filename_item->valuestring) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing filename");
        return ESP_FAIL;
    }
    
#ifdef CONFIG_ENABLE_SOUND_MANAGER
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", CONFIG_SOUND_FILES_PATH, filename_item->valuestring);
    unlink(filepath);
#endif
    
    cJSON_Delete(json);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    
    return ESP_OK;
}

/**
 * POST /api/sounds/play - Play sound event
 * Body: {"event": 0}
 */
esp_err_t sound_play_handler(httpd_req_t *req)
{
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    int event = cJSON_GetObjectItem(json, "event")->valueint;
    cJSON_Delete(json);
    
    sound_manager_play_event((sound_event_t)event, SOUND_MODE_ONCE);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"playing\"}");
    
    return ESP_OK;
}

/**
 * POST /api/sounds/stop - Stop playback
 */
esp_err_t sound_stop_handler(httpd_req_t *req)
{
    sound_manager_stop();
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"stopped\"}");
    
    return ESP_OK;
}

/**
 * GET /api/sounds/volume - Get current volume
 */
esp_err_t sound_volume_get_handler(httpd_req_t *req)
{
    int volume = sound_manager_get_volume();
    
    httpd_resp_set_type(req, "application/json");
    char resp[64];
    snprintf(resp, sizeof(resp), "{\"volume\":%d}", volume);
    httpd_resp_sendstr(req, resp);
    
    return ESP_OK;
}

/**
 * POST /api/sounds/volume - Set volume
 * Body: {"volume": 70}
 */
esp_err_t sound_volume_set_handler(httpd_req_t *req)
{
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    int volume = cJSON_GetObjectItem(json, "volume")->valueint;
    cJSON_Delete(json);
    
    sound_manager_set_volume(volume);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    
    return ESP_OK;
}

/**
 * GET /sounds.html - Serve sound management page
 */
esp_err_t sounds_page_handler(httpd_req_t *req)
{
    extern const uint8_t sounds_html_start[] asm("_binary_sounds_html_start");
    extern const uint8_t sounds_html_end[] asm("_binary_sounds_html_end");
    const size_t sounds_html_size = (sounds_html_end - sounds_html_start);
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)sounds_html_start, sounds_html_size);
    
    return ESP_OK;
}
