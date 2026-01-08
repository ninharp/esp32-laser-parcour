/**
 * ESP-NOW Manager Component - Implementation
 * 
 * Manages ESP-NOW communication between modules.
 * 
 * @author ninharp
 * @date 2025
 */

#include "espnow_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_crc.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "ESPNOW_MGR";

// Broadcast MAC address
static const uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Message receive callback
static espnow_recv_callback_t recv_callback = NULL;

// Message queue (reserved for future use)
static QueueHandle_t espnow_queue __attribute__((unused)) = NULL;

/**
 * ESP-NOW send callback
 */
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGD(TAG, "Message sent successfully");
    } else {
        ESP_LOGW(TAG, "Message send failed");
    }
}

/**
 * ESP-NOW receive callback
 */
static void espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
    if (data_len != sizeof(espnow_message_t)) {
        ESP_LOGW(TAG, "Invalid message size: %d", data_len);
        return;
    }
    
    espnow_message_t *msg = (espnow_message_t *)data;
    
    // Verify checksum
    uint16_t calc_checksum = esp_crc16_le(0, data, sizeof(espnow_message_t) - 2);
    if (calc_checksum != msg->checksum) {
        ESP_LOGW(TAG, "Checksum mismatch");
        return;
    }
    
    ESP_LOGD(TAG, "Received message type 0x%02X from module %d", 
             msg->msg_type, msg->module_id);
    
    // Call user callback if registered
    if (recv_callback) {
        recv_callback(esp_now_info->src_addr, msg);
    }
}

/**
 * Initialize ESP-NOW manager
 */
esp_err_t espnow_manager_init(uint8_t channel, espnow_recv_callback_t callback)
{
    ESP_LOGI(TAG, "Initializing ESP-NOW manager on channel %d...", channel);
    
    // Store callback
    recv_callback = callback;
    
    // WiFi should already be initialized by wifi_ap_manager
    // Just ensure we're on the correct channel
    esp_err_t ret = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set WiFi channel: %s", esp_err_to_name(ret));
    }
    
    // Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    
    // Register callbacks
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    
    // Add broadcast peer
    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, broadcast_mac, 6);
    peer_info.channel = channel;
    peer_info.ifidx = WIFI_IF_STA;
    peer_info.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));
    
    ESP_LOGI(TAG, "ESP-NOW manager initialized successfully");
    
    return ESP_OK;
}

/**
 * Deinitialize ESP-NOW manager
 */
esp_err_t espnow_manager_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing ESP-NOW manager...");
    
    esp_now_deinit();
    esp_wifi_stop();
    
    return ESP_OK;
}

/**
 * Send message to a specific peer
 */
esp_err_t espnow_send_message(const uint8_t *dest_mac, espnow_msg_type_t msg_type,
                              const uint8_t *data, size_t data_len)
{
    if (data_len > 32) {
        ESP_LOGE(TAG, "Data too large: %d bytes (max 32)", data_len);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Build message
    espnow_message_t msg = {0};
    msg.msg_type = msg_type;
    // Note: CONFIG_MODULE_ID comes from sdkconfig, included automatically by build system
    msg.module_id = CONFIG_MODULE_ID;
    msg.timestamp = (uint32_t)(esp_timer_get_time() / 1000);
    
    if (data && data_len > 0) {
        memcpy(msg.data, data, data_len);
    }
    
    // Calculate checksum
    msg.checksum = esp_crc16_le(0, (uint8_t *)&msg, sizeof(espnow_message_t) - 2);
    
    // Send message
    const uint8_t *target_mac = dest_mac ? dest_mac : broadcast_mac;
    esp_err_t err = esp_now_send(target_mac, (uint8_t *)&msg, sizeof(espnow_message_t));
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send message: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGD(TAG, "Sent message type 0x%02X", msg_type);
    
    return ESP_OK;
}

/**
 * Broadcast message to all peers
 */
esp_err_t espnow_broadcast_message(espnow_msg_type_t msg_type,
                                   const uint8_t *data, size_t data_len)
{
    return espnow_send_message(NULL, msg_type, data, data_len);
}

/**
 * Add a peer to ESP-NOW
 */
esp_err_t espnow_add_peer(const uint8_t *mac_addr, uint8_t module_id, uint8_t module_role)
{
    if (!mac_addr) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check if peer already exists
    if (esp_now_is_peer_exist(mac_addr)) {
        ESP_LOGW(TAG, "Peer already exists");
        return ESP_OK;
    }
    
    // Add peer
    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, mac_addr, 6);
    peer_info.channel = CONFIG_ESPNOW_CHANNEL;
    peer_info.ifidx = WIFI_IF_STA;
    peer_info.encrypt = false;
    
    esp_err_t err = esp_now_add_peer(&peer_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add peer: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Added peer: Module ID %d, Role %d", module_id, module_role);
    
    return ESP_OK;
}

/**
 * Remove a peer from ESP-NOW
 */
esp_err_t espnow_remove_peer(const uint8_t *mac_addr)
{
    if (!mac_addr) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = esp_now_del_peer(mac_addr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remove peer: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Removed peer");
    
    return ESP_OK;
}

/**
 * Get list of all paired peers
 */
esp_err_t espnow_get_peers(espnow_peer_info_t *peers, size_t max_peers, size_t *peer_count)
{
    if (!peers || !peer_count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // TODO: Implement peer list tracking
    *peer_count = 0;
    
    ESP_LOGW(TAG, "Peer list tracking not yet implemented");
    
    return ESP_OK;
}

/**
 * Get local MAC address
 */
esp_err_t espnow_get_local_mac(uint8_t *mac_addr)
{
    if (!mac_addr) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return esp_wifi_get_mac(WIFI_IF_STA, mac_addr);
}

/**
 * Change WiFi/ESP-NOW channel
 */
esp_err_t espnow_change_channel(uint8_t new_channel)
{
    if (new_channel < 1 || new_channel > 13) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Changing WiFi/ESP-NOW channel to %d", new_channel);
    
    // Remove old broadcast peer
    esp_err_t ret = esp_now_del_peer(broadcast_mac);
    if (ret != ESP_OK && ret != ESP_ERR_ESPNOW_NOT_FOUND) {
        ESP_LOGW(TAG, "Failed to remove broadcast peer: %s", esp_err_to_name(ret));
    }
    
    // Change WiFi channel
    ret = esp_wifi_set_channel(new_channel, WIFI_SECOND_CHAN_NONE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to change WiFi channel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Add broadcast peer on new channel
    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, broadcast_mac, 6);
    peer_info.channel = new_channel;
    peer_info.ifidx = WIFI_IF_STA;
    peer_info.encrypt = false;
    
    ret = esp_now_add_peer(&peer_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add broadcast peer on channel %d: %s", 
                 new_channel, esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Channel changed successfully to %d, broadcast peer updated", new_channel);
    return ESP_OK;
}

/**
 * Broadcast channel change to all peers and wait for acknowledgement
 */
esp_err_t espnow_broadcast_channel_change(uint8_t new_channel, uint32_t timeout_ms)
{
    if (new_channel < 1 || new_channel > 13) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Broadcasting channel change to %d (timeout: %lu ms)", new_channel, timeout_ms);
    
    // Prepare channel change message
    uint8_t data[1] = {new_channel};
    
    // Send broadcast 3 times for reliability
    for (int i = 0; i < 3; i++) {
        esp_err_t ret = espnow_broadcast_message(MSG_CHANNEL_CHANGE, data, sizeof(data));
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to broadcast channel change (attempt %d)", i + 1);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait 100ms between broadcasts
    }
    
    // Give peers time to process and switch
    ESP_LOGI(TAG, "Waiting %lu ms for peers to acknowledge channel change...", timeout_ms);
    vTaskDelay(pdMS_TO_TICKS(timeout_ms));
    
    ESP_LOGI(TAG, "Channel change broadcast complete");
    return ESP_OK;
}

