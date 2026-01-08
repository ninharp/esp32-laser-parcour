/**
 * SSD1306 OLED Display Driver - Implementation
 * 
 * Low-level driver for SSD1306 128x64 OLED displays via I2C.
 * 
 * @author ninharp
 * @date 2025
 */

#include "ssd1306.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SSD1306";

#define I2C_MASTER_NUM I2C_NUM_0
#define SSD1306_I2C_ADDRESS 0x3C

// SSD1306 Commands
#define SSD1306_CMD_SET_CONTRAST 0x81
#define SSD1306_CMD_DISPLAY_ALL_ON_RESUME 0xA4
#define SSD1306_CMD_DISPLAY_ALL_ON 0xA5
#define SSD1306_CMD_NORMAL_DISPLAY 0xA6
#define SSD1306_CMD_INVERT_DISPLAY 0xA7
#define SSD1306_CMD_DISPLAY_OFF 0xAE
#define SSD1306_CMD_DISPLAY_ON 0xAF
#define SSD1306_CMD_SET_DISPLAY_OFFSET 0xD3
#define SSD1306_CMD_SET_COM_PINS 0xDA
#define SSD1306_CMD_SET_VCOMH_DESELECT 0xDB
#define SSD1306_CMD_SET_DISPLAY_CLK_DIV 0xD5
#define SSD1306_CMD_SET_PRECHARGE 0xD9
#define SSD1306_CMD_SET_MULTIPLEX 0xA8
#define SSD1306_CMD_SET_LOW_COLUMN 0x00
#define SSD1306_CMD_SET_HIGH_COLUMN 0x10
#define SSD1306_CMD_SET_START_LINE 0x40
#define SSD1306_CMD_MEMORY_MODE 0x20
#define SSD1306_CMD_COLUMN_ADDR 0x21
#define SSD1306_CMD_PAGE_ADDR 0x22
#define SSD1306_CMD_COM_SCAN_INC 0xC0
#define SSD1306_CMD_COM_SCAN_DEC 0xC8
#define SSD1306_CMD_SEG_REMAP 0xA1
#define SSD1306_CMD_CHARGE_PUMP 0x8D
#define SSD1306_CMD_SCROLL_H_RIGHT 0x26
#define SSD1306_CMD_SCROLL_H_LEFT 0x27
#define SSD1306_CMD_DEACTIVATE_SCROLL 0x2E

// Simple 5x7 font (ASCII 32-127)
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x08, 0x04, 0x08, 0x10, 0x08}, // ~
};

static bool initialized = false;
static uint8_t framebuffer[SSD1306_WIDTH * SSD1306_PAGES]; // 128 * 8 = 1024 bytes

/**
 * Write command to SSD1306
 */
static esp_err_t write_command(uint8_t cmd)
{
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (SSD1306_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, 0x00, true); // Command mode
    i2c_master_write_byte(i2c_cmd, cmd, true);
    i2c_master_stop(i2c_cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, i2c_cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(i2c_cmd);
    return ret;
}

/**
 * Write data to SSD1306
 */
static esp_err_t write_data(uint8_t *data, size_t len)
{
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (SSD1306_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, 0x40, true); // Data mode
    i2c_master_write(i2c_cmd, data, len, true);
    i2c_master_stop(i2c_cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, i2c_cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(i2c_cmd);
    return ret;
}

/**
 * Scan I2C bus for device at address
 */
static esp_err_t i2c_scan_device(uint8_t addr)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));  // Increased timeout
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * Test if I2C communication works at all
 */
static void i2c_test_pins(gpio_num_t sda_pin, gpio_num_t scl_pin)
{
    ESP_LOGI(TAG, "Testing I2C configuration:");
    ESP_LOGI(TAG, "  SDA Pin: GPIO%d", sda_pin);
    ESP_LOGI(TAG, "  SCL Pin: GPIO%d", scl_pin);
    ESP_LOGI(TAG, "  Frequency: 100kHz");
    ESP_LOGI(TAG, "  Pull-ups: Enabled (internal)");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "If scan fails, try:");
    ESP_LOGI(TAG, "  1. Swap SDA/SCL pins");
    ESP_LOGI(TAG, "  2. Add external 4.7k pull-up resistors");
    ESP_LOGI(TAG, "  3. Check display power (3.3V)");
    ESP_LOGI(TAG, "  4. Try lower I2C speed (10kHz)");
}

/**
 * Initialize SSD1306 display
 */
esp_err_t ssd1306_init(gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t freq_hz)
{
    ESP_LOGI(TAG, "Initializing SSD1306 (SDA:%d, SCL:%d, Freq:%lu Hz)...", sda_pin, scl_pin, freq_hz);
    
    // Configure I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = freq_hz,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "I2C driver installed, scanning for display...");
    
    // Wait for display to power up
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Show I2C configuration for debugging
    i2c_test_pins(sda_pin, scl_pin);
    
    // Try common OLED display addresses (0x3C and 0x3D)
    uint8_t display_addr = 0;
    ESP_LOGI(TAG, "Trying display address 0x3C...");
    if (i2c_scan_device(0x3C) == ESP_OK) {
        display_addr = 0x3C;
        ESP_LOGI(TAG, "✓ Display found at address 0x3C");
    } else {
        ESP_LOGI(TAG, "Trying display address 0x3D...");
        if (i2c_scan_device(0x3D) == ESP_OK) {
            display_addr = 0x3D;
            ESP_LOGI(TAG, "✓ Display found at address 0x3D");
            ESP_LOGW(TAG, "Note: Address 0x3D detected. This might be a SH1106 display!");
        } else {
            ESP_LOGE(TAG, "✗ Display NOT found at 0x3C or 0x3D");
            
            // Full I2C bus scan with detailed output
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "=== Full I2C Bus Scan ===");
            bool found_any = false;
            for (uint8_t addr = 0x01; addr < 0x7F; addr++) {
                esp_err_t ret = i2c_scan_device(addr);
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "  ✓ Device found at 0x%02X", addr);
                    found_any = true;
                } else if (addr % 16 == 0) {
                    // Progress indicator
                    ESP_LOGD(TAG, "  Scanning 0x%02X...", addr);
                }
            }
            ESP_LOGI(TAG, "=========================");
            ESP_LOGI(TAG, "");
            
            if (!found_any) {
                ESP_LOGE(TAG, "❌ NO I2C devices found on bus!");
                ESP_LOGE(TAG, "");
                ESP_LOGE(TAG, "Possible issues:");
                ESP_LOGE(TAG, "  1. SDA/SCL pins swapped (try: SDA=18, SCL=19)");
                ESP_LOGE(TAG, "  2. Missing pull-up resistors (add 4.7kΩ)");
                ESP_LOGE(TAG, "  3. Display not powered (check 3.3V)");
                ESP_LOGE(TAG, "  4. Loose wiring/bad connections");
            } else {
                ESP_LOGW(TAG, "Found I2C device(s) but not at expected OLED addresses");
            }
            
            // Don't fail - allow system to continue without display
            ESP_LOGW(TAG, "Continuing without display...");
            return ESP_FAIL;
        }
    }
    
    // Update I2C address if different from default
    if (display_addr != SSD1306_I2C_ADDRESS) {
        ESP_LOGI(TAG, "Using detected address 0x%02X instead of default 0x%02X", 
                 display_addr, SSD1306_I2C_ADDRESS);
        // Note: We would need to modify SSD1306_I2C_ADDRESS or store display_addr globally
        // For now, we'll try to continue with detection
    }
    
    // Init sequence for 128x32 OLED module
    ESP_LOGI(TAG, "Sending initialization sequence for 128x32 display...");
    write_command(SSD1306_CMD_DISPLAY_OFF);
    write_command(SSD1306_CMD_SET_DISPLAY_CLK_DIV);
    write_command(0x80);
    write_command(SSD1306_CMD_SET_MULTIPLEX);
    write_command(0x1F); // 32 lines
    write_command(SSD1306_CMD_SET_DISPLAY_OFFSET);
    write_command(0x00);
    write_command(SSD1306_CMD_SET_START_LINE | 0x00);
    write_command(SSD1306_CMD_CHARGE_PUMP);
    write_command(0x14); // Enable charge pump
    write_command(SSD1306_CMD_MEMORY_MODE);
    write_command(0x00); // Horizontal addressing mode
    
    // Display rotation (CONFIG_DISPLAY_ROTATION_180)
#ifdef CONFIG_DISPLAY_ROTATION_180
    // 180° rotation
    write_command(SSD1306_CMD_SEG_REMAP | 0x00);  // Column address 0 is mapped to SEG0
    write_command(SSD1306_CMD_COM_SCAN_INC);       // Scan from COM0 to COM[N-1]
    ESP_LOGI(TAG, "Display rotation: 180°");
#else
    // Normal orientation (0°)
    write_command(SSD1306_CMD_SEG_REMAP | 0x01);  // Column address 127 is mapped to SEG0
    write_command(SSD1306_CMD_COM_SCAN_DEC);       // Scan from COM[N-1] to COM0
    ESP_LOGI(TAG, "Display rotation: 0°");
#endif
    
    write_command(SSD1306_CMD_SET_COM_PINS);
    write_command(0x02); // Sequential COM pin config for 32px
    write_command(SSD1306_CMD_SET_CONTRAST);
    write_command(0xCF);
    write_command(SSD1306_CMD_SET_PRECHARGE);
    write_command(0xF1);
    write_command(SSD1306_CMD_SET_VCOMH_DESELECT);
    write_command(0x40);
    write_command(SSD1306_CMD_DISPLAY_ALL_ON_RESUME);
    write_command(SSD1306_CMD_NORMAL_DISPLAY);
    write_command(SSD1306_CMD_DEACTIVATE_SCROLL);
    write_command(SSD1306_CMD_DISPLAY_ON);
    
    // Clear framebuffer
    memset(framebuffer, 0, sizeof(framebuffer));
    
    initialized = true;
    
    ESP_LOGI(TAG, "SSD1306 initialized successfully (128x32, 4 pages)");
    
    return ESP_OK;
}

/**
 * Clear the framebuffer
 */
esp_err_t ssd1306_clear(void)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
    memset(framebuffer, 0, sizeof(framebuffer));
    return ESP_OK;
}

/**
 * Send framebuffer to display
 */
esp_err_t ssd1306_update(void)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
    // Set column address range
    write_command(SSD1306_CMD_COLUMN_ADDR);
    write_command(0);   // Start
    write_command(127); // End
    
    // Set page address range
    write_command(SSD1306_CMD_PAGE_ADDR);
    write_command(0); // Start
    write_command(7); // End
    
    // Send framebuffer in chunks
    const size_t chunk_size = 16;
    for (size_t i = 0; i < sizeof(framebuffer); i += chunk_size) {
        size_t len = (i + chunk_size <= sizeof(framebuffer)) ? chunk_size : sizeof(framebuffer) - i;
        write_data(&framebuffer[i], len);
    }
    
    return ESP_OK;
}

/**
 * Draw a character to framebuffer
 */
void ssd1306_draw_char(uint8_t x, uint8_t page, char c)
{
    if (!initialized || page >= SSD1306_PAGES) return;
    
    if (c < 32 || c > 126) c = 32; // Replace unsupported chars with space
    
    const uint8_t *glyph = font5x7[c - 32];
    
    for (int i = 0; i < 5; i++) {
        if (x + i < SSD1306_WIDTH) {
            framebuffer[page * SSD1306_WIDTH + x + i] = glyph[i];
        }
    }
    // Add 1 pixel space after character
    if (x + 5 < SSD1306_WIDTH) {
        framebuffer[page * SSD1306_WIDTH + x + 5] = 0x00;
    }
}

/**
 * Draw a string to framebuffer
 */
void ssd1306_draw_string(uint8_t x, uint8_t page, const char *str)
{
    if (!initialized || !str) return;
    
    uint8_t pos = x;
    while (*str && pos < SSD1306_WIDTH) {
        ssd1306_draw_char(pos, page, *str);
        pos += 6; // 5 pixels + 1 space
        str++;
    }
}

/**
 * Draw a large digit (3x size) to framebuffer
 */
void ssd1306_draw_large_digit(uint8_t x, uint8_t page, char digit)
{
    if (!initialized || digit < '0' || digit > '9') return;
    
    const uint8_t *glyph = font5x7[digit - 32];
    
    // Draw 3x scaled character across 3 pages
    for (int p = 0; p < 3; p++) {
        for (int i = 0; i < 5; i++) {
            for (int scale = 0; scale < 3; scale++) {
                uint8_t col = 0;
                for (int row = 0; row < 8; row++) {
                    if (glyph[i] & (1 << row)) {
                        // Each pixel becomes 3 pixels tall
                        if (row / 3 == p) {
                            col |= (7 << ((row % 3) * 3));
                        }
                    }
                }
                if (x + i * 3 + scale < SSD1306_WIDTH && page + p < SSD1306_PAGES) {
                    framebuffer[(page + p) * SSD1306_WIDTH + x + i * 3 + scale] = col;
                }
            }
        }
    }
}

/**
 * Draw a horizontal line
 */
void ssd1306_draw_hline(uint8_t page, uint8_t pattern)
{
    if (!initialized || page >= SSD1306_PAGES) return;
    
    for (int i = 0; i < SSD1306_WIDTH; i++) {
        framebuffer[page * SSD1306_WIDTH + i] = pattern;
    }
}

/**
 * Set display contrast
 */
esp_err_t ssd1306_set_contrast(uint8_t contrast)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
    write_command(SSD1306_CMD_SET_CONTRAST);
    write_command(contrast);
    
    return ESP_OK;
}

/**
 * Turn display on/off
 */
esp_err_t ssd1306_display_power(bool on)
{
    if (!initialized) {
        return ESP_FAIL;
    }
    
    if (on) {
        write_command(SSD1306_CMD_DISPLAY_ON);
    } else {
        write_command(SSD1306_CMD_DISPLAY_OFF);
    }
    
    return ESP_OK;
}

/**
 * Get direct access to framebuffer
 */
uint8_t* ssd1306_get_framebuffer(void)
{
    return initialized ? framebuffer : NULL;
}
