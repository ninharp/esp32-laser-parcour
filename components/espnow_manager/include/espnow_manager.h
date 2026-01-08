/**
 * ESP-NOW Manager Component - Header
 * 
 * Manages ESP-NOW communication between main unit and laser units.
 * 
 * @author ninharp
 * @date 2025
 */

#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_now.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ESP-NOW message types
 */
typedef enum {
    MSG_GAME_START = 0x01,          // Start game command
    MSG_GAME_STOP = 0x02,           // Stop game command
    MSG_BEAM_BROKEN = 0x03,         // Beam break notification
    MSG_STATUS_UPDATE = 0x04,       // Periodic status update
    MSG_CONFIG_UPDATE = 0x05,       // Configuration update
    MSG_HEARTBEAT = 0x06,           // Keep-alive signal
    MSG_PAIRING_REQUEST = 0x07,     // Request to pair with network
    MSG_PAIRING_RESPONSE = 0x08,    // Response to pairing request
    MSG_LASER_ON = 0x09,            // Turn laser on
    MSG_LASER_OFF = 0x0A,           // Turn laser off
    MSG_SENSOR_CALIBRATE = 0x0B,    // Calibrate sensor
    MSG_RESET = 0x0C,               // Reset module
    MSG_CHANNEL_CHANGE = 0x0D,      // WiFi channel change notification
    MSG_CHANNEL_ACK = 0x0E          // Channel change acknowledgement
} espnow_msg_type_t;

/**
 * ESP-NOW message structure
 */
typedef struct __attribute__((packed)) {
    uint8_t msg_type;               // Message type
    uint8_t module_id;              // Source module ID
    uint32_t timestamp;             // Message timestamp (ms)
    uint8_t data[32];               // Payload data
    uint16_t checksum;              // CRC16 checksum
} espnow_message_t;

/**
 * ESP-NOW peer information
 */
typedef struct {
    uint8_t mac_addr[6];            // MAC address
    uint8_t module_id;              // Module ID
    uint8_t module_role;            // Module role (control/laser/sensor)
    int8_t rssi;                    // Signal strength (RSSI)
    uint32_t last_seen;             // Last message timestamp
    bool is_paired;                 // Is peer paired
} espnow_peer_info_t;

/**
 * Message received callback
 * 
 * @param sender_mac MAC address of sender
 * @param message Received message
 */
typedef void (*espnow_recv_callback_t)(const uint8_t *sender_mac, const espnow_message_t *message);

/**
 * Initialize ESP-NOW manager
 * 
 * @param channel WiFi/ESP-NOW channel (1-13)
 * @param callback Callback for received messages
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_manager_init(uint8_t channel, espnow_recv_callback_t callback);

/**
 * Deinitialize ESP-NOW manager
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_manager_deinit(void);

/**
 * Send message to a specific peer
 * 
 * @param dest_mac Destination MAC address (NULL for broadcast)
 * @param msg_type Message type
 * @param data Data payload
 * @param data_len Length of data
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_send_message(const uint8_t *dest_mac, espnow_msg_type_t msg_type, 
                              const uint8_t *data, size_t data_len);

/**
 * Broadcast message to all peers
 * 
 * @param msg_type Message type
 * @param data Data payload
 * @param data_len Length of data
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_broadcast_message(espnow_msg_type_t msg_type, 
                                   const uint8_t *data, size_t data_len);

/**
 * Add a peer to ESP-NOW
 * 
 * @param mac_addr MAC address of peer
 * @param module_id Module ID
 * @param module_role Module role
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_add_peer(const uint8_t *mac_addr, uint8_t module_id, uint8_t module_role);

/**
 * Remove a peer from ESP-NOW
 * 
 * @param mac_addr MAC address of peer
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_remove_peer(const uint8_t *mac_addr);

/**
 * Get list of all paired peers
 * 
 * @param peers Array to store peer information
 * @param max_peers Maximum number of peers to retrieve
 * @param peer_count Pointer to store actual number of peers
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_get_peers(espnow_peer_info_t *peers, size_t max_peers, size_t *peer_count);

/**
 * Get local MAC address
 * 
 * @param mac_addr Buffer to store MAC address (6 bytes)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_get_local_mac(uint8_t *mac_addr);

/**
 * Update all existing peers with new channel
 * Call this after WiFi channel change to update peer configurations
 * 
 * @param new_channel New channel (1-13)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_update_all_peers_channel(uint8_t new_channel);

/**
 * Change WiFi/ESP-NOW channel
 * 
 * @param new_channel New channel (1-13)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_change_channel(uint8_t new_channel);

/**
 * Broadcast channel change to all peers and wait for acknowledgement
 * 
 * @param new_channel New channel to broadcast
 * @param timeout_ms Timeout in milliseconds to wait for ACKs
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t espnow_broadcast_channel_change(uint8_t new_channel, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // ESPNOW_MANAGER_H
