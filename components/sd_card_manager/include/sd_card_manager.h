/**
 * @file sd_card_manager.h
 * @brief SD Card Manager für SPI-basierte SD-Karten mit FAT-Dateisystem
 * 
 * Unterstützt:
 * - SPI-Mode SD-Karten-Zugriff
 * - FAT-Dateisystem (FAT12/FAT16/FAT32)
 * - Mount-Point: /sdcard
 * - Automatisches Fallback bei Fehler
 */

#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#include "esp_err.h"
#include "driver/gpio.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SD-Karten-Status
 */
typedef enum {
    SD_STATUS_NOT_INITIALIZED,  ///< Noch nicht initialisiert
    SD_STATUS_MOUNTED,           ///< Erfolgreich gemountet
    SD_STATUS_MOUNT_FAILED,      ///< Mount fehlgeschlagen
    SD_STATUS_NO_CARD,           ///< Keine Karte erkannt
    SD_STATUS_ERROR              ///< Fehler
} sd_status_t;

/**
 * @brief SD-Karten-Informationen
 */
typedef struct {
    sd_status_t status;          ///< Aktueller Status
    uint64_t total_bytes;        ///< Gesamtgröße in Bytes
    uint64_t free_bytes;         ///< Freier Speicher in Bytes
    char card_type[16];          ///< Kartentyp (SD, SDHC, SDXC)
    bool web_dir_available;      ///< /web Verzeichnis vorhanden
} sd_card_info_t;

/**
 * @brief SD-Karten-Konfiguration
 */
typedef struct {
    gpio_num_t mosi_pin;         ///< SPI MOSI Pin
    gpio_num_t miso_pin;         ///< SPI MISO Pin
    gpio_num_t clk_pin;          ///< SPI CLK Pin
    gpio_num_t cs_pin;           ///< SPI CS (Chip Select) Pin
    uint32_t max_freq_khz;       ///< Maximale SPI-Frequenz in kHz (Standard: 20000)
    const char *mount_point;     ///< Mount-Point (Standard: "/sdcard")
} sd_card_config_t;

/**
 * @brief SD-Karte initialisieren und mounten
 * 
 * @param config SD-Karten-Konfiguration (NULL für Standard-Pins aus menuconfig)
 * @return esp_err_t ESP_OK bei Erfolg
 * 
 * @note Bei Fehler wird ESP_FAIL zurückgegeben, aber die Anwendung läuft weiter
 */
esp_err_t sd_card_manager_init(const sd_card_config_t *config);

/**
 * @brief SD-Karte unmounten und Ressourcen freigeben
 * 
 * @return esp_err_t ESP_OK bei Erfolg
 */
esp_err_t sd_card_manager_deinit(void);

/**
 * @brief Aktuellen SD-Karten-Status abrufen
 * 
 * @return sd_status_t Aktueller Status
 */
sd_status_t sd_card_get_status(void);

/**
 * @brief SD-Karten-Informationen abrufen
 * 
 * @param info Pointer auf Info-Struktur
 * @return esp_err_t ESP_OK bei Erfolg
 */
esp_err_t sd_card_get_info(sd_card_info_t *info);

/**
 * @brief Prüfen ob /web Verzeichnis mit index.html existiert
 * 
 * @return true wenn /sdcard/web/index.html vorhanden ist
 */
bool sd_card_has_web_interface(void);

/**
 * @brief Mount-Point abrufen
 * 
 * @return const char* Mount-Point (z.B. "/sdcard")
 */
const char *sd_card_get_mount_point(void);

#ifdef __cplusplus
}
#endif

#endif // SD_CARD_MANAGER_H
