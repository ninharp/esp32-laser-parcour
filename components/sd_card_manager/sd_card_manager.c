/**
 * @file sd_card_manager.c
 * @brief SD Card Manager Implementierung
 */

#include "sd_card_manager.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include <sys/stat.h>
#include <string.h>

static const char *TAG = "SD_CARD_MANAGER";

// Globale Variablen
static sd_status_t current_status = SD_STATUS_NOT_INITIALIZED;
static sdmmc_card_t *card = NULL;
static const char *mount_point = "/sdcard";
static bool web_dir_checked = false;
static bool web_dir_available = false;

esp_err_t sd_card_manager_init(const sd_card_config_t *config)
{
    esp_err_t ret;
    
    // Standard-Konfiguration aus menuconfig verwenden wenn keine übergeben
    sd_card_config_t default_config = {
#ifdef CONFIG_SD_CARD_MOSI_PIN
        .mosi_pin = CONFIG_SD_CARD_MOSI_PIN,
        .miso_pin = CONFIG_SD_CARD_MISO_PIN,
        .clk_pin = CONFIG_SD_CARD_CLK_PIN,
        .cs_pin = CONFIG_SD_CARD_CS_PIN,
        .max_freq_khz = 20000,  // 20 MHz Standard
        .mount_point = "/sdcard"
#else
        .mosi_pin = GPIO_NUM_NC,
        .miso_pin = GPIO_NUM_NC,
        .clk_pin = GPIO_NUM_NC,
        .cs_pin = GPIO_NUM_NC,
        .max_freq_khz = 20000,
        .mount_point = "/sdcard"
#endif
    };
    
    const sd_card_config_t *active_config = config ? config : &default_config;
    mount_point = active_config->mount_point;
    
    // Prüfen ob SD-Karte aktiviert ist
#ifndef CONFIG_ENABLE_SD_CARD
    ESP_LOGW(TAG, "SD Card support is disabled in menuconfig");
    current_status = SD_STATUS_NOT_INITIALIZED;
    return ESP_ERR_NOT_SUPPORTED;
#endif
    
    // Prüfen ob Pins konfiguriert sind
    if (active_config->mosi_pin == GPIO_NUM_NC || 
        active_config->miso_pin == GPIO_NUM_NC ||
        active_config->clk_pin == GPIO_NUM_NC ||
        active_config->cs_pin == GPIO_NUM_NC) {
        ESP_LOGE(TAG, "Invalid SD Card pin configuration");
        current_status = SD_STATUS_ERROR;
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Initializing SD Card (SPI Mode)");
    ESP_LOGI(TAG, "  MOSI: GPIO%d, MISO: GPIO%d, CLK: GPIO%d, CS: GPIO%d", 
             active_config->mosi_pin, active_config->miso_pin, 
             active_config->clk_pin, active_config->cs_pin);
    
    // FAT-Dateisystem Mount-Konfiguration
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,  // Nicht formatieren bei Fehler
        .max_files = 5,                   // Maximal 5 gleichzeitig offene Dateien
        .allocation_unit_size = 16 * 1024 // 16 KB Cluster
    };
    
    // SPI-Bus-Konfiguration
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = active_config->max_freq_khz;
    
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = active_config->mosi_pin,
        .miso_io_num = active_config->miso_pin,
        .sclk_io_num = active_config->clk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    // SPI-Bus initialisieren
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        current_status = SD_STATUS_ERROR;
        return ret;
    }
    
    // SD-Karten Slot-Konfiguration
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = active_config->cs_pin;
    slot_config.host_id = host.slot;
    
    // SD-Karte mounten
    ESP_LOGI(TAG, "Mounting SD Card at %s", mount_point);
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. Card may be unformatted or incompatible.");
            current_status = SD_STATUS_MOUNT_FAILED;
        } else {
            ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(ret));
            current_status = SD_STATUS_NO_CARD;
        }
        
        // SPI-Bus freigeben bei Fehler
        spi_bus_free(host.slot);
        return ret;
    }
    
    current_status = SD_STATUS_MOUNTED;
    
    // Karteninfo ausgeben
    sdmmc_card_print_info(stdout, card);
    
    // Prüfen ob /web Verzeichnis existiert
    web_dir_checked = false;  // Wird beim ersten Aufruf geprüft
    
    ESP_LOGI(TAG, "SD Card successfully mounted at %s", mount_point);
    
    return ESP_OK;
}

esp_err_t sd_card_manager_deinit(void)
{
    if (current_status != SD_STATUS_MOUNTED) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Unmounting SD Card");
    
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(mount_point, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // SPI-Bus freigeben
    spi_bus_free(SDSPI_DEFAULT_HOST);
    
    current_status = SD_STATUS_NOT_INITIALIZED;
    card = NULL;
    web_dir_checked = false;
    web_dir_available = false;
    
    ESP_LOGI(TAG, "SD Card unmounted successfully");
    
    return ESP_OK;
}

sd_status_t sd_card_get_status(void)
{
    return current_status;
}

esp_err_t sd_card_get_info(sd_card_info_t *info)
{
    if (!info) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(info, 0, sizeof(sd_card_info_t));
    info->status = current_status;
    
    if (current_status != SD_STATUS_MOUNTED || !card) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Kartentyp basierend auf Kapazität
    uint64_t capacity = ((uint64_t)card->csd.capacity) * card->csd.sector_size;
    if (capacity > 2LL * 1024 * 1024 * 1024) {  // > 2GB
        snprintf(info->card_type, sizeof(info->card_type), "SDHC/SDXC");
    } else {
        snprintf(info->card_type, sizeof(info->card_type), "SDSC");
    }
    
    // Größeninformationen
    FATFS *fs;
    DWORD fre_clust;
    
    char path[16];
    snprintf(path, sizeof(path), "%s:", mount_point);
    
    if (f_getfree(path, &fre_clust, &fs) == FR_OK) {
        info->total_bytes = ((uint64_t)fs->n_fatent - 2) * fs->csize * 512;
        info->free_bytes = (uint64_t)fre_clust * fs->csize * 512;
    }
    
    // /web Verzeichnis
    info->web_dir_available = sd_card_has_web_interface();
    
    return ESP_OK;
}

bool sd_card_has_web_interface(void)
{
    if (current_status != SD_STATUS_MOUNTED) {
        return false;
    }
    
    // Cache verwenden wenn bereits geprüft
    if (web_dir_checked) {
        return web_dir_available;
    }
    
    // Prüfen ob /sdcard/web/index.html existiert
    char path[64];
    snprintf(path, sizeof(path), "%s/web/index.html", mount_point);
    
    struct stat st;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
        ESP_LOGI(TAG, "Found web interface on SD card: %s", path);
        web_dir_available = true;
    } else {
        ESP_LOGW(TAG, "No web interface found on SD card (missing %s)", path);
        web_dir_available = false;
    }
    
    web_dir_checked = true;
    return web_dir_available;
}

const char *sd_card_get_mount_point(void)
{
    return mount_point;
}
