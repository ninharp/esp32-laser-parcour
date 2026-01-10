/*
 * Custom Audio Board Implementation - Minimal Stub
 * Provides stub functions for ESP-ADF audio board interface
 * These functions are not used in this project but satisfy linker requirements
 */

#include "esp_log.h"
#include "board.h"

static const char *TAG = "AUDIO_BOARD";

esp_err_t get_i2c_pins(int port, void *config)
{
    ESP_LOGW(TAG, "get_i2c_pins() stub called - not implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

int get_pa_enable_gpio(void)
{
    // Return invalid GPIO to indicate no PA enable pin
    return -1;
}

esp_err_t audio_board_init(void)
{
    ESP_LOGI(TAG, "Custom audio board init (stub - no action)");
    return ESP_OK;
}

audio_board_handle_t audio_board_get_handle(void)
{
    // Return NULL since we don't use audio board handle in this project
    return NULL;
}

esp_err_t audio_board_deinit(void)
{
    ESP_LOGI(TAG, "Custom audio board deinit (stub - no action)");
    return ESP_OK;
}
