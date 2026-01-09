/**
 * FINISH Module - Finish Button Implementation
 * 
 * Handles finish button unit initialization and event callbacks
 * 
 * @author ninharp
 * @date 2026-01-09
 */

#include "module_finish.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "string.h"

// Component includes
#include "espnow_manager.h"

static const char *TAG = "MODULE_FINISH";

// Finish Button Module State
static bool is_paired = false;
static uint8_t main_unit_mac[6] = {0};
static esp_timer_handle_t pairing_timer = NULL;
static esp_timer_handle_t led_blink_timer = NULL;
static esp_timer_handle_t heartbeat_timer = NULL;
static bool status_led_state = false;
static bool button_led_on = true;  // Button illumination LED starts ON
static volatile bool button_pressed = false;

// Channel scanning state for pairing
static bool channel_scanning = true;
static uint8_t scanning_channels[] = {1, 6, 11};
static uint8_t scanning_channel_index = 0;

/**
 * Initialize status LED for finish button
 */
static void init_finish_button_leds(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    // Configure status LED (connection/pairing indicator)
    io_conf.pin_bit_mask = (1ULL << CONFIG_FINISH_STATUS_LED_PIN);
    gpio_config(&io_conf);
    gpio_set_level(CONFIG_FINISH_STATUS_LED_PIN, 0);
    
    // Configure button illumination LED
    io_conf.pin_bit_mask = (1ULL << CONFIG_FINISH_BUTTON_LED_PIN);
    gpio_config(&io_conf);
    gpio_set_level(CONFIG_FINISH_BUTTON_LED_PIN, 1);  // Initially ON
    
    ESP_LOGI(TAG, "Finish Button LEDs initialized (Status:%d, Button Light:%d)",
             CONFIG_FINISH_STATUS_LED_PIN, CONFIG_FINISH_BUTTON_LED_PIN);
}

/**
 * LED blink timer callback for finish button
 */
static void led_blink_timer_callback(void *arg)
{
    if (!is_paired) {
        // Blink status LED when not paired
        status_led_state = !status_led_state;
        gpio_set_level(CONFIG_FINISH_STATUS_LED_PIN, status_led_state ? 1 : 0);
    } else {
        // Solid on when paired
        gpio_set_level(CONFIG_FINISH_STATUS_LED_PIN, 1);
    }
}

/**
 * Heartbeat timer callback (sends periodic heartbeat to main unit)
 */
static void heartbeat_timer_callback(void *arg)
{
    if (is_paired) {
        espnow_broadcast_message(MSG_HEARTBEAT, NULL, 0);
    }
}

/**
 * Pairing timer callback with multi-channel scanning
 */
static void pairing_timer_callback(void *arg)
{
    if (!is_paired) {
        if (channel_scanning) {
            // Cycle through common channels
            uint8_t target_channel = scanning_channels[scanning_channel_index];
            ESP_LOGI(TAG, "Scanning channel %d for main unit...", target_channel);
            
            // Change to next scanning channel
            espnow_change_channel(target_channel);
            
            // Toggle status LED while scanning (visual feedback)
            status_led_state = !status_led_state;
            gpio_set_level(CONFIG_FINISH_STATUS_LED_PIN, status_led_state ? 1 : 0);
            
            // Move to next channel for next attempt
            scanning_channel_index = (scanning_channel_index + 1) % (sizeof(scanning_channels) / sizeof(scanning_channels[0]));
        }
        
        // Send pairing request (broadcast)
        ESP_LOGI(TAG, "Sending pairing request (Module ID: %d)...", CONFIG_MODULE_ID);
        uint8_t role = 2;  // 2 = Finish Button
        espnow_broadcast_message(MSG_PAIRING_REQUEST, &role, sizeof(role));
    }
}

/**
 * Button interrupt handler (active low)
 */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    button_pressed = true;
}

/**
 * Button handler task - processes button press and sends finish message
 */
static void button_handler_task(void *arg)
{
    ESP_LOGI(TAG, "Button handler task started");
    
    while (1) {
        // Check if button was pressed
        if (button_pressed) {
            button_pressed = false;  // Clear flag
            
            // Debounce: wait for button to stabilize
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // Check if button is still pressed (active low)
            if (gpio_get_level(CONFIG_FINISH_BUTTON_PIN) == 0) {
                ESP_LOGI(TAG, "Finish button pressed!");
                
                // Turn off button illumination LED while pressed
                gpio_set_level(CONFIG_FINISH_BUTTON_LED_PIN, 0);
                button_led_on = false;
                
                // Send finish pressed message to main unit (unicast)
                if (is_paired) {
                    esp_err_t ret = espnow_send_message(main_unit_mac, MSG_FINISH_PRESSED, NULL, 0);
                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "Finish message sent to main unit successfully!");
                    } else {
                        ESP_LOGE(TAG, "Failed to send finish message: %s", esp_err_to_name(ret));
                    }
                } else {
                    ESP_LOGW(TAG, "Not paired, cannot send finish message");
                }
                
                // Wait for button release
                while (gpio_get_level(CONFIG_FINISH_BUTTON_PIN) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                
                // Turn button illumination LED back ON after release
                gpio_set_level(CONFIG_FINISH_BUTTON_LED_PIN, 1);
                button_led_on = true;
                ESP_LOGI(TAG, "Button released");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * ESP-NOW message received callback (Finish Button Unit)
 */
static void espnow_recv_callback_finish(const uint8_t *sender_mac, const espnow_message_t *message)
{
    ESP_LOGI(TAG, "ESP-NOW message received from %02X:%02X:%02X:%02X:%02X:%02X",
             sender_mac[0], sender_mac[1], sender_mac[2], 
             sender_mac[3], sender_mac[4], sender_mac[5]);
    
    switch (message->msg_type) {
        case MSG_PAIRING_RESPONSE:
            ESP_LOGI(TAG, "Pairing response received!");
            if (!is_paired) {
                is_paired = true;
                memcpy(main_unit_mac, sender_mac, 6);
                ESP_LOGI(TAG, "Successfully paired with main unit: %02X:%02X:%02X:%02X:%02X:%02X",
                         main_unit_mac[0], main_unit_mac[1], main_unit_mac[2],
                         main_unit_mac[3], main_unit_mac[4], main_unit_mac[5]);
                
                // Stop channel scanning
                channel_scanning = false;
                
                // Turn on status LED solid
                gpio_set_level(CONFIG_FINISH_STATUS_LED_PIN, 1);
                
                // Start heartbeat timer (every 3 seconds)
                esp_timer_start_periodic(heartbeat_timer, 3000000);
            }
            break;
            
        case MSG_HEARTBEAT:
            // Ignore heartbeat broadcasts from other units
            break;
            
        case MSG_RESET:
            ESP_LOGI(TAG, "Reset command received - resetting pairing state");
            is_paired = false;
            channel_scanning = true;
            scanning_channel_index = 0;
            
            // Turn off button illumination LED
            gpio_set_level(CONFIG_FINISH_BUTTON_LED_PIN, 1);  // Reset to ON
            button_led_on = true;
            
            // Stop heartbeat timer
            esp_timer_stop(heartbeat_timer);
            break;
            
        case MSG_CHANNEL_CHANGE: {
            uint8_t new_channel = message->data[0];
            ESP_LOGI(TAG, "Channel change requested: %d", new_channel);
            
            // Stop channel scanning when main unit sets specific channel
            channel_scanning = false;
            
            // Change channel
            esp_err_t ret = espnow_change_channel(new_channel);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Channel changed successfully to %d", new_channel);
                
                // Send ACK back to main unit
                espnow_broadcast_message(MSG_CHANNEL_ACK, NULL, 0);
            } else {
                ESP_LOGE(TAG, "Failed to change channel: %s", esp_err_to_name(ret));
            }
            break;
        }
            
        default:
            ESP_LOGW(TAG, "Unknown message type: 0x%02X", message->msg_type);
            break;
    }
}

/**
 * Initialize the finish button unit
 */
void module_finish_init(void)
{
    ESP_LOGI(TAG, "Initializing Finish Button Unit...");
    
    // Initialize button GPIO (active low with pull-up)
    ESP_LOGI(TAG, "  Initializing Button (GPIO %d)", CONFIG_FINISH_BUTTON_PIN);
    gpio_config_t button_conf = {
        .pin_bit_mask = (1ULL << CONFIG_FINISH_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,  // Trigger on falling edge (button press)
    };
    gpio_config(&button_conf);
    
    // Install GPIO ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(CONFIG_FINISH_BUTTON_PIN, button_isr_handler, NULL);
    
    // Initialize LEDs
    ESP_LOGI(TAG, "  Initializing LEDs (Status: GPIO %d, Button Light: GPIO %d)",
             CONFIG_FINISH_STATUS_LED_PIN, CONFIG_FINISH_BUTTON_LED_PIN);
    init_finish_button_leds();
    
    // Initialize WiFi (required for ESP-NOW)
    ESP_LOGI(TAG, "  Initializing WiFi for ESP-NOW");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Initialize ESP-NOW
    ESP_LOGI(TAG, "  Initializing ESP-NOW (Channel: %d)", CONFIG_ESPNOW_CHANNEL);
    ESP_ERROR_CHECK(espnow_manager_init(CONFIG_ESPNOW_CHANNEL, espnow_recv_callback_finish));
    
    // Set up periodic pairing request timer (every 1.5 seconds until paired)
    ESP_LOGI(TAG, "  Setting up pairing request timer");
    const esp_timer_create_args_t pairing_timer_args = {
        .callback = &pairing_timer_callback,
        .name = "pairing_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&pairing_timer_args, &pairing_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(pairing_timer, 1500000));  // 1.5 seconds
    
    // Set up LED blink timer for visual feedback during pairing
    ESP_LOGI(TAG, "  Setting up LED blink timer");
    const esp_timer_create_args_t led_blink_timer_args = {
        .callback = &led_blink_timer_callback,
        .name = "led_blink_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&led_blink_timer_args, &led_blink_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(led_blink_timer, 500000));  // 500ms (fast blink)
    
    // Set up heartbeat timer (starts after successful pairing)
    ESP_LOGI(TAG, "  Setting up heartbeat timer");
    const esp_timer_create_args_t heartbeat_timer_args = {
        .callback = &heartbeat_timer_callback,
        .name = "heartbeat_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&heartbeat_timer_args, &heartbeat_timer));
    
    // Create button handler task
    ESP_LOGI(TAG, "  Creating button handler task");
    xTaskCreate(button_handler_task, "button_handler", 4096, NULL, 5, NULL);
    
    // Send initial pairing request
    ESP_LOGI(TAG, "  Sending initial pairing request");
    uint8_t role = 2;  // 2 = Finish Button
    espnow_broadcast_message(MSG_PAIRING_REQUEST, &role, sizeof(role));
    
    // Print GPIO configuration summary
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "   Finish Button - GPIO Configuration");
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "Finish Button:  GPIO%d (Active Low)", CONFIG_FINISH_BUTTON_PIN);
    ESP_LOGI(TAG, "Status LED:     GPIO%d", CONFIG_FINISH_STATUS_LED_PIN);
    ESP_LOGI(TAG, "Button Light:   GPIO%d", CONFIG_FINISH_BUTTON_LED_PIN);
    ESP_LOGI(TAG, "ESP-NOW Ch:     %d (scanning)", CONFIG_ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "=================================================");
    
    ESP_LOGI(TAG, "Finish Button Unit initialized - ready to signal completion");
}

/**
 * Run the finish button unit loop
 */
void module_finish_run(void)
{
    // Main loop for finish button module
    while (1) {
        ESP_LOGI(TAG, "Status: Running - Free heap: %ld bytes - Paired: %s",
                 esp_get_free_heap_size(), is_paired ? "Yes" : "No");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
