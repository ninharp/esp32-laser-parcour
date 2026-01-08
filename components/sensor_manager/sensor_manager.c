/**
 * Sensor Manager Component - Implementation
 * 
 * Manages photoresistor sensors for laser beam detection.
 * 
 * @author ninharp
 * @date 2025
 */

#include "sensor_manager.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SENSOR_MGR";

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_channel_t adc_chan;
static uint16_t detection_threshold = 2000;  // For LDR: no laser ~850, with laser ~4095
static uint32_t debounce_time_ms = 100;
static sensor_status_t current_status = SENSOR_BEAM_DETECTED;
static beam_break_callback_t break_callback = NULL;
static beam_restore_callback_t restore_callback = NULL;
static TaskHandle_t monitor_task_handle = NULL;
static bool monitoring_active = false;

/**
 * Sensor monitoring task
 */
static void sensor_monitor_task(void *arg)
{
    uint32_t last_break_time = 0;
    bool last_state = true; // true = beam detected
    uint32_t last_log_time = 0;
    
    ESP_LOGI(TAG, "Sensor monitoring task started");
    ESP_LOGI(TAG, "Threshold: %d (ADC values above this = beam present)", detection_threshold);
    
    while (monitoring_active) {
        int adc_value = 0;
        esp_err_t err = adc_oneshot_read(adc_handle, adc_chan, &adc_value);
        
        if (err == ESP_OK) {
            bool beam_present = (adc_value > detection_threshold);
            
            // Log ADC value every second for debugging
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (current_time - last_log_time > 1000) {
                ESP_LOGI(TAG, "ADC: %d | Threshold: %d | Beam: %s", 
                         adc_value, detection_threshold, beam_present ? "PRESENT" : "BROKEN");
                last_log_time = current_time;
            }
            
            // Check for state change
            if (beam_present != last_state) {
                uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                
                // Debounce
                if (current_time - last_break_time > debounce_time_ms) {
                    last_state = beam_present;
                    last_break_time = current_time;
                    
                    if (!beam_present) {
                        // Beam broken!
                        current_status = SENSOR_BEAM_BROKEN;
                        ESP_LOGW(TAG, "Beam broken! ADC: %d (threshold: %d)", 
                                 adc_value, detection_threshold);
                        
                        if (break_callback) {
                            // Pass sensor identifier (use module ID as sensor ID)
                            break_callback(adc_chan);
                        }
                    } else {
                        // Beam restored
                        current_status = SENSOR_BEAM_DETECTED;
                        ESP_LOGI(TAG, "Beam restored. ADC: %d", adc_value);
                        
                        if (restore_callback) {
                            restore_callback(adc_chan);
                        }
                    }
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Sample every 10ms
    }
    
    ESP_LOGI(TAG, "Sensor monitoring task stopped");
    monitor_task_handle = NULL;
    vTaskDelete(NULL);
}

/**
 * Initialize sensor manager
 */
esp_err_t sensor_manager_init(uint8_t adc_channel, uint16_t threshold, uint32_t debounce_ms)
{
    ESP_LOGI(TAG, "Initializing sensor manager (ADC channel %d, threshold %d)...",
             adc_channel, threshold);
    
    adc_chan = adc_channel;
    detection_threshold = threshold;
    debounce_time_ms = debounce_ms;
    
    // Initialize ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
    
    // Configure ADC channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, adc_chan, &config));
    
    ESP_LOGI(TAG, "Sensor manager initialized");
    
    return ESP_OK;
}

/**
 * Register beam break callback
 */
esp_err_t sensor_register_callback(beam_break_callback_t callback)
{
    break_callback = callback;
    ESP_LOGI(TAG, "Beam break callback registered");
    return ESP_OK;
}

/**
 * Register beam restore callback
 */
esp_err_t sensor_register_restore_callback(beam_restore_callback_t callback)
{
    restore_callback = callback;
    ESP_LOGI(TAG, "Beam restore callback registered");
    return ESP_OK;
}

/**
 * Read current ADC value
 */
esp_err_t sensor_read_value(uint16_t *value)
{
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int adc_value = 0;
    esp_err_t err = adc_oneshot_read(adc_handle, adc_chan, &adc_value);
    
    if (err == ESP_OK) {
        *value = (uint16_t)adc_value;
    }
    
    return err;
}

/**
 * Get current sensor status
 */
sensor_status_t sensor_get_status(void)
{
    return current_status;
}

/**
 * Set detection threshold
 */
esp_err_t sensor_set_threshold(uint16_t threshold)
{
    if (threshold > 4095) {
        return ESP_ERR_INVALID_ARG;
    }
    
    detection_threshold = threshold;
    ESP_LOGI(TAG, "Threshold set to %d", threshold);
    
    return ESP_OK;
}

/**
 * Calibrate sensor
 */
esp_err_t sensor_calibrate(void)
{
    uint16_t current_value = 0;
    esp_err_t err = sensor_read_value(&current_value);
    
    if (err == ESP_OK) {
        // Set threshold to 80% of current value
        detection_threshold = (current_value * 80) / 100;
        ESP_LOGI(TAG, "Calibrated: current=%d, new threshold=%d", 
                 current_value, detection_threshold);
    }
    
    return err;
}

/**
 * Start sensor monitoring
 */
esp_err_t sensor_start_monitoring(void)
{
    if (monitoring_active) {
        ESP_LOGW(TAG, "Monitoring already active");
        return ESP_OK;
    }
    
    monitoring_active = true;
    
    xTaskCreate(sensor_monitor_task, "sensor_monitor", 2048, NULL, 5, &monitor_task_handle);
    
    ESP_LOGI(TAG, "Sensor monitoring started");
    
    return ESP_OK;
}

/**
 * Stop sensor monitoring
 */
esp_err_t sensor_stop_monitoring(void)
{
    if (!monitoring_active) {
        return ESP_OK;
    }
    
    monitoring_active = false;
    
    // Wait for task to finish
    while (monitor_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    ESP_LOGI(TAG, "Sensor monitoring stopped");
    
    return ESP_OK;
}
