/**
 * Display Manager Component - Implementation
 * 
 * Abstract display manager that delegates to specific display drivers
 * (SSD1306, SH1106, etc.) based on menuconfig selection.
 * 
 * @author ninharp
 * @date 2025
 */

#include "display_manager.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "DISPLAY_MGR";

#ifdef CONFIG_ENABLE_DISPLAY

// Include driver based on configuration
#ifdef CONFIG_OLED_SSD1306
#include "ssd1306.h"
#define DISPLAY_WIDTH SSD1306_WIDTH
#define DISPLAY_HEIGHT SSD1306_HEIGHT
#define DISPLAY_PAGES SSD1306_PAGES
#elif defined(CONFIG_OLED_SH1106)
#include "sh1106.h"
#define DISPLAY_WIDTH SH1106_WIDTH
#define DISPLAY_HEIGHT SH1106_HEIGHT
#define DISPLAY_PAGES SH1106_PAGES
#else
#error "CONFIG_ENABLE_DISPLAY is set but no display driver selected (CONFIG_OLED_SSD1306 or CONFIG_OLED_SH1106)"
#endif

static bool initialized = false;
static display_screen_t current_screen = SCREEN_IDLE;

/**
 * Initialize display manager
 */
esp_err_t display_manager_init(gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t freq_hz)
{
    ESP_LOGI(TAG, "Initializing display manager...");
    
    esp_err_t ret;
    
#ifdef CONFIG_OLED_SSD1306
    ret = ssd1306_init(sda_pin, scl_pin, freq_hz);
#elif defined(CONFIG_OLED_SH1106)
    ret = sh1106_init(sda_pin, scl_pin, freq_hz);
#endif
    
    if (ret == ESP_OK) {
        initialized = true;
        ESP_LOGI(TAG, "Display manager initialized successfully");
    } else {
        ESP_LOGE(TAG, "Display manager initialization failed");
    }
    
    return ret;
}

/**
 * Clear the display
 */
esp_err_t display_clear(void)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
#ifdef CONFIG_OLED_SSD1306
    return ssd1306_clear();
#elif defined(CONFIG_OLED_SH1106)
    return sh1106_clear();
#endif
    
    return ESP_FAIL;
}

/**
 * Update display (send framebuffer to display)
 */
esp_err_t display_update(void)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
#ifdef CONFIG_OLED_SSD1306
    return ssd1306_update();
#elif defined(CONFIG_OLED_SH1106)
    return sh1106_update();
#endif
    
    return ESP_FAIL;
}

/**
 * Set current screen
 */
esp_err_t display_set_screen(display_screen_t screen)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
    current_screen = screen;
    
    const char *screen_names[] = {
        "IDLE", "MENU", "COUNTDOWN", "RUNNING", "PAUSED", "COMPLETE", "SETTINGS", "STATS"
    };
    
    ESP_LOGD(TAG, "Screen changed to: %s", screen_names[screen]);
    
    return ESP_OK;
}

/**
 * Display game status
 */
esp_err_t display_game_status(uint32_t elapsed_time, uint16_t beam_breaks)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
    uint32_t minutes = elapsed_time / 60000;
    uint32_t seconds = (elapsed_time % 60000) / 1000;
    uint32_t millis = (elapsed_time % 1000) / 10;
    
    display_clear();
    
#ifdef CONFIG_OLED_SSD1306
    // Line 0: Title - show different text based on current screen state
    if (current_screen == SCREEN_GAME_PAUSED) {
        ssd1306_draw_string(25, 0, "*** PAUSED ***");
    } else if (current_screen == SCREEN_GAME_RUNNING) {
        // Check if we should show PENALTY (caller will set screen appropriately)
        ssd1306_draw_string(30, 0, "GAME ACTIVE");
    } else {
        // Default for any other state
        ssd1306_draw_string(30, 0, "GAME ACTIVE");
    }
    
    // Line 2-3: Time (large)
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02lu:%02lu.%02lu", minutes, seconds, millis);
    ssd1306_draw_string(10, 3, time_str);
    
    // Line 5: Beam breaks
    char breaks_str[20];
    snprintf(breaks_str, sizeof(breaks_str), "Breaks: %d", beam_breaks);
    ssd1306_draw_string(5, 6, breaks_str);
#elif defined(CONFIG_OLED_SH1106)
    // SH1106 implementation (TODO when driver is available)
#endif
    
    display_update();
    
    ESP_LOGD(TAG, "Game Status - Time: %02lu:%02lu.%02lu, Breaks: %d",
             minutes, seconds, millis, beam_breaks);
    
    return ESP_OK;
}

/**
 * Display countdown
 */
esp_err_t display_countdown(uint8_t seconds)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
    display_clear();
    
#ifdef CONFIG_OLED_SSD1306
    // Draw "Starting in..." text
    ssd1306_draw_string(20, 1, "Starting in...");
    
    // Draw large countdown number (centered)
    char num[2] = {seconds + '0', 0};
    if (seconds < 10) {
        ssd1306_draw_large_digit(50, 3, num[0]);
    }
#elif defined(CONFIG_OLED_SH1106)
    // SH1106 implementation (TODO)
#endif
    
    display_update();
    
    ESP_LOGD(TAG, "Countdown: %d", seconds);
    
    return ESP_OK;
}

/**
 * Display text message
 */
esp_err_t display_text(const char *message, uint8_t line)
{
    if (!initialized || !message) {
        return ESP_FAIL;
    }
    
    if (line >= DISPLAY_PAGES) {
        return ESP_ERR_INVALID_ARG;
    }
    
#ifdef CONFIG_OLED_SSD1306
    ssd1306_draw_string(0, line, message);
#elif defined(CONFIG_OLED_SH1106)
    // SH1106 implementation (TODO)
#endif
    
    ESP_LOGD(TAG, "Display text (line %d): %s", line, message);
    
    return ESP_OK;
}

/**
 * Display game results
 */
esp_err_t display_game_results(uint32_t final_time, uint16_t beam_breaks, completion_status_t completion)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
    uint32_t minutes = final_time / 60000;
    uint32_t seconds = (final_time % 60000) / 1000;
    uint32_t millis = (final_time % 1000) / 10;
    
    display_clear();
    
#ifdef CONFIG_OLED_SSD1306
    // Line 0: Title - show different message based on completion status
    if (completion == COMPLETION_SOLVED) {
        ssd1306_draw_string(20, 0, "GAME COMPLETE!");
    } else {
        // ABORTED_TIME or ABORTED_MANUAL
        ssd1306_draw_string(15, 0, "GAME CANCELED!");
    }
    
    // Line 2: Divider
    ssd1306_draw_hline(2, 0xFF);
    
    // Line 3: Total Time label
    ssd1306_draw_string(25, 3, "Total Time:");
    
    // Line 4-5: Time (large)
    char time_str[20];
    snprintf(time_str, sizeof(time_str), "%02lu:%02lu.%02lu", minutes, seconds, millis);
    ssd1306_draw_string(15, 5, time_str);
    
    // Line 7: Breaks
    char breaks_str[20];
    snprintf(breaks_str, sizeof(breaks_str), "Breaks: %d", beam_breaks);
    ssd1306_draw_string(30, 7, breaks_str);
#elif defined(CONFIG_OLED_SH1106)
    // SH1106 implementation (TODO)
#endif
    
    display_update();
    
    const char* completion_str = (completion == COMPLETION_SOLVED) ? "COMPLETE" : 
                                 (completion == COMPLETION_ABORTED_TIME) ? "CANCELED (TIME LIMIT)" :
                                 "CANCELED (MANUAL)";
    
    ESP_LOGI(TAG, "=== GAME RESULTS ===");
    ESP_LOGI(TAG, "Status: %s", completion_str);
    ESP_LOGI(TAG, "Total Time: %02lu:%02lu.%02lu", minutes, seconds, millis);
    ESP_LOGI(TAG, "Beam Breaks: %d", beam_breaks);
    ESP_LOGI(TAG, "====================");
    
    return ESP_OK;
}

/**
 * Set display contrast
 */
esp_err_t display_set_contrast(uint8_t contrast)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
#ifdef CONFIG_OLED_SSD1306
    ssd1306_set_contrast(contrast);
#elif defined(CONFIG_OLED_SH1106)
    // SH1106 implementation (TODO)
#endif
    
    ESP_LOGI(TAG, "Contrast set to %d", contrast);
    
    return ESP_OK;
}

/**
 * Turn display on/off
 */
esp_err_t display_power(bool on)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
#ifdef CONFIG_OLED_SSD1306
    ssd1306_display_power(on);
#elif defined(CONFIG_OLED_SH1106)
    // SH1106 implementation (TODO)
#endif
    
    ESP_LOGI(TAG, "Display power: %s", on ? "ON" : "OFF");
    
    return ESP_OK;
}

#else // CONFIG_ENABLE_DISPLAY not defined - provide stub implementations

esp_err_t display_manager_init(gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t freq_hz)
{
    ESP_LOGD(TAG, "Display support disabled");
    return ESP_OK;
}

esp_err_t display_clear(void)
{
    return ESP_OK;
}

esp_err_t display_update(void)
{
    return ESP_OK;
}

esp_err_t display_set_screen(display_screen_t screen)
{
    return ESP_OK;
}

esp_err_t display_game_status(uint32_t elapsed_time, uint16_t beam_breaks)
{
    return ESP_OK;
}

esp_err_t display_countdown(uint8_t seconds)
{
    return ESP_OK;
}

esp_err_t display_text(const char *message, uint8_t line)
{
    return ESP_OK;
}

esp_err_t display_game_results(uint32_t final_time, uint16_t beam_breaks)
{
    return ESP_OK;
}

esp_err_t display_set_contrast(uint8_t contrast)
{
    return ESP_OK;
}

esp_err_t display_power(bool on)
{
    return ESP_OK;
}

#endif // CONFIG_ENABLE_DISPLAY
