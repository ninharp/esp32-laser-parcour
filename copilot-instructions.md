# ESP32 Laser Obstacle Course - Project Documentation

## 📋 Projektübersicht

Ein modulares, ESP32-C3 basiertes Laser-Hindernisparcours-Spielsystem mit drahtloser Steuerung, Echtzeit-Überwachung und interaktivem Gaming-Erlebnis.

**Technologie-Stack:**
- ESP-IDF 5.5.2
- ESP-ADF 2.7 (Audio Development Framework)
- FreeRTOS
- ESP-NOW (Wireless Communication)
- I2C (OLED Display)
- ADC (Sensor Detection)
- LEDC PWM (Laser Control)
- I2S (MAX98357A Audio Amplifier)
- SD Card (Web Interface & High Scores)

---

## 📝 Dokumentations-Regeln

**⚠️ KRITISCH - IMMER BEFOLGEN:**

**🔴 NACH JEDEM SCHRITT MUSS DIE `copilot-instructions.md` AKTUALISIERT WERDEN! 🔴**

Dies ist KEINE optionale Aufgabe - es ist eine PFLICHT!

**Signifikante Änderungen umfassen:**
- ✅ Neue Features oder Komponenten
- ✅ Geänderte Funktionalität
- ✅ Neue Hardware-Anforderungen
- ✅ Geänderte Pin-Zuweisungen
- ✅ Update von Abhängigkeiten oder Versionen
- ✅ Neue Menuconfig-Optionen
- ✅ Sicherheitsmechanismen oder Safety Features
- ✅ Neue Modulrollen oder Gerätetypen
- ✅ Änderungen am Kommunikationsprotokoll (ESP-NOW Messages)
- ✅ Web-Interface-Änderungen
- ✅ Code-Refactoring oder Modularisierung
- ✅ Bugfixes die Verhalten ändern

**README Update Checkliste:**
1. Feature-Liste aktualisieren (✨ Features Sektion)
2. Hardware-Anforderungen prüfen (🛠️ Hardware Requirements)
3. Pin-Konfigurationen aktualisieren (GPIO Tables)
4. Menuconfig-Optionen dokumentieren (🔧 Advanced Configuration)
5. Troubleshooting erweitern bei bekannten Problemen
6. System Architecture aktualisieren bei Protokolländerungen
7. Version und Datum am Ende aktualisieren

---

## 🏗️ Projektstruktur

```
esp32-laser-parcour/
├── CMakeLists.txt                    # Haupt-Build-Konfiguration
├── partitions.csv                    # Flash-Partitionstabelle
├── sdkconfig.defaults                # Standard SDK-Konfiguration
├── README.md                         # Projekt-Dokumentation
├── copilot-instructions.md           # Diese Datei
├── TODO                              # Task-Liste
│
├── main/                             # Haupt-Anwendung
│   ├── main.c                        # Hauptprogramm mit app_main()
│   ├── main.c.backup                 # Original vor Refactoring (BACKUP!)
│   ├── module_control.c/.h           # Control Unit Module (Main Unit)
│   ├── module_laser.c/.h             # Laser Unit Module
│   ├── module_finish.c/.h            # Finish Button Module
│   ├── logging_config.c/.h           # Zentrales Logging
│   ├── Kconfig.projbuild             # Menuconfig-Optionen
│   └── CMakeLists.txt                # Build-Config (conditional compilation)
│
├── components/                       # Wiederverwendbare Komponenten
│   ├── display_manager/              # OLED Display Steuerung
│   ├── game_logic/                   # Spiellogik & Scoring
│   ├── espnow_manager/               # ESP-NOW Kommunikation
│   ├── laser_control/                # Laser PWM Steuerung
│   ├── sensor_manager/               # ADC Sensor für Beam Detection
│   ├── button_handler/               # Button-Eingabe mit Debouncing
│   ├── buzzer/                       # PWM Audio-Feedback
│   ├── wifi_ap_manager/              # WiFi Access Point
│   ├── web_server/                   # HTTP Server & REST API
│   └── sd_card_manager/              # SD Card für Web-Files & Scores
│
├── docs/                             # Zusätzliche Dokumentation
│   └── README.md
│
└── build/                            # Build-Artefakte (generiert)
    ├── esp32_laser_parcour.elf       # Ausführbare Datei
    ├── bootloader/                   # Bootloader
    └── partition_table/              # Partitionstabelle
```

---

## 🔄 Aktuelle Architektur-Änderungen (Januar 2026)

### ESP-IDF 5.5.2 Migration (10. Januar 2026)

**Upgrade von 5.4.2 → 5.5.2**
- ✅ ESP-NOW API breaking changes behoben:
  - `espnow_send_cb` Callback-Signatur geändert von `(const uint8_t *mac_addr, ...)` zu `(const wifi_tx_info_t *tx_info, ...)`
  - `components/espnow_manager/espnow_manager.c` aktualisiert
- ✅ WiFi API breaking changes behoben:
  - `esp_wifi_set_config()` kann nicht mehr im laufenden WiFi-State aufgerufen werden
  - Lösung: WiFi stoppen vor Rekonfiguration, dann neu starten
  - `components/wifi_ap_manager/wifi_ap_manager.c:463` - `wifi_connect_sta()` gefixt
  - Fehler war: `ESP_ERR_WIFI_STATE (0x3006)` beim Versuch config während WiFi started zu ändern
- ✅ ESP-ADF Integration vereinfacht:
  - Lokale `audio_stream` Komponente entfernt
  - Verwendet jetzt native ESP-ADF Komponenten direkt
  - `include($ENV{ADF_PATH}/CMakeLists.txt)` in Haupt-CMakeLists.txt
- ✅ Konfiguration:
  - `CONFIG_ESP32C3_DEFAULT_CPU_FREQ_160` → `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_160`
  - Alle deprecated Warnings bereinigt

### Audio System Debugging (Januar 2026)

**Problem:** Audio Crackling/Buzzing auf MAX98357A I2S Amplifier
- Hardware: MAX98357A Class-D Amplifier, SD_MODE hardwired active
- Symptom: Kontinuierliches Krächzen auch ohne Wiedergabe
- Debugging-Ansatz: HTTP-Stream-Wiedergabe aus bekanntem funktionierendem Beispiel
- Status: In Diagnose - Grundlegende Wiedergabe ERST testen bevor weitere Features

**AKTUELLER TEST-ZUSTAND (10. Januar 2026):**
- ⚠️ **SIMPLE TEST PIPELINE:** `http → mp3 → i2s` (NUR diese 3 Elemente)
- ❌ Equalizer/Resample sind ERSTELLT aber NICHT in Pipeline registriert (testweise deaktiviert)
- ❌ `init_i2s_stream()` Funktion ist AUSKOMMENTIERT (testweise)
- ✅ **FIX 1:** I2S Stream wird INLINE erstellt (behebt NULL-Pointer-Crash)
  - Problem war: `i2s_stream_writer` war NULL weil `init_i2s_stream()` auskommentiert war
  - Lösung: I2S Stream direkt in `init_pipeline()` erstellen vor Pipeline-Registrierung
  - Zeile ~220 in `sound_manager.c`: Inline I2S-Stream-Initialisierung
- ✅ **FIX 2:** Pipeline startet NACH WiFi-Verbindung (behebt HTTP-Stream ohne Netzwerk)
  - Problem: HTTP-Stream startete vor WiFi-Verbindung
  - Lösung: `sound_manager_start_streaming()` wird von `module_control.c` nach WiFi-Init aufgerufen
  - Pipeline wird bei Init erstellt aber NICHT gestartet
  - Start erfolgt explizit nach erfolgreicher WiFi-Verbindung
- 🔄 **NÄCHSTER SCHRITT:** Erst wenn grundlegende Audio-Wiedergabe funktioniert:
  - Equalizer/Resample zur Pipeline hinzufügen
  - `init_i2s_stream()` Funktion reaktivieren
  - Vollständige Pipeline: `http → mp3 → resample → equalizer → i2s`

**Wichtige Code-Stellen:**
- `components/sound_manager/sound_manager.c:~220`: Inline I2S-Erstellung (TEMP FIX)
- `components/sound_manager/sound_manager.c:~130`: `init_i2s_stream()` Funktion (auskommentiert)
- `components/sound_manager/sound_manager.c:~228`: Pipeline Register (nur 3 Elemente)
- `components/sound_manager/sound_manager.c:~234`: Link Chain (nur 3 Tags)
- `main/module_control.c:~660-678`: WiFi-Connect mit `sound_manager_start_streaming()` Aufruf

**API-Migration:**
- Alte API: `audio_play_event(AUDIO_EVENT_*, false)` → Entfernt
- Neue API: `sound_manager_play_event(SOUND_EVENT_*, SOUND_MODE_ONCE)`
- Alle Aufrufe in `module_control.c` migriert (18 Stellen)
- `audio_output.c` entfernt (obsolet, Funktionalität in sound_manager.c)
- CMakeLists.txt bereinigt (audio_output.c Referenz entfernt)

### Modularisierung in separate Module

**Problem:** main.c war 1578 Zeilen lang mit #ifdef Blöcken für verschiedene Device Types

**Lösung:** Aufteilung in separate Module mit conditional compilation:

```
main.c (130 lines)           → Core init + module delegation
module_control.c (643 lines) → Control Unit (Main Unit)
module_laser.c (507 lines)   → Laser Unit
module_finish.c (352 lines)  → Finish Button Unit
```

**Build-System:**
- `CMakeLists.txt` kompiliert nur das passende Modul basierend auf `CONFIG_MODULE_ROLE_XXX`
- Header haben `#ifdef CONFIG_MODULE_ROLE_XXX` Guards
- Jedes Modul hat `module_xxx_init()` und `module_xxx_run()` Funktionen

**WICHTIG - Original-Funktionalität:**
- `main.c.backup` enthält die ORIGINAL working implementation (1578 lines)
- Bei Funktionalitätsproblemen IMMER gegen `main.c.backup` vergleichen!
- Alle Fixes müssen EXAKTE Funktionalität aus backup wiederherstellen

---

## 📁 Datei-Referenz und Aufgaben

### Root-Verzeichnis

#### `CMakeLists.txt`
**Zweck:** Haupt-Build-Konfiguration für das gesamte Projekt  
**Aufgaben:**
- Definiert Projektnamen (`esp32_laser_parcour`)
- Bindet ESP-IDF Build-System ein
- Minimale CMake-Version: 3.16

#### `partitions.csv`
**Zweck:** Definition der Flash-Speicher-Partitionierung  
**Aufgaben:**
- Legt Größen für Bootloader, App, NVS fest
- Konfiguriert OTA-Partitionen (falls benötigt)

#### `sdkconfig.defaults`
**Zweck:** Standard-Konfiguration für ESP-IDF  
**Aufgaben:**
- Vordefinierte Einstellungen für WiFi, Bluetooth, etc.
- Board-spezifische Konfiguration
- NVS-Encryption deaktiviert (wurde manuell angepasst)

#### `README.md`
**Zweck:** Hauptdokumentation des Projekts  
**Inhalte:**
- Hardware-Anforderungen
- Setup-Anleitung
- Feature-Beschreibungen
- Pin-Belegungen

---

### `/main` - Hauptanwendung

#### `main/main.c`
**Zweck:** Hauptprogramm und Einstiegspunkt  
**Funktionen:**
```c
void app_main(void)                          // Haupteinstiegspunkt
static void init_nvs(void)                   // NVS-Initialisierung
static void init_network(void)               // Netzwerk-Stack Init
static void print_system_info(void)          // System-Info ausgeben

// CONTROL MODULE (Main Unit)
static void init_main_unit(void)             // Main Unit Initialisierung
static void espnow_recv_callback_main()      // ESP-NOW Message Handler

// LASER MODULE (Laser Unit)
static void init_laser_unit(void)            // Laser Unit Initialisierung
static void espnow_recv_callback_laser()     // ESP-NOW Message Handler
static void beam_break_callback()            // Beam-Break Event Handler
static void pairing_timer_callback()         // Periodischer Pairing Request
static void init_status_leds(void)           // Status-LED Initialisierung
```

**Aufgaben:**
- Rolle-basierte Initialisierung (Main Unit vs. Laser Unit)
- Integration aller Komponenten
- Event-Handling für ESP-NOW Messages
- Status-LED Steuerung
- Automatisches Pairing-System mit Timer

**Dependencies:**
- Alle Component-Header
- FreeRTOS
- ESP-IDF Core Libraries

#### `main/Kconfig.projbuild`
**Zweck:** Menuconfig-Konfigurationsoptionen  
**Konfiguriert:**
- **Module Role:** CONTROL (Main Unit) oder LASER (Laser Unit)
- **Module ID:** Eindeutige ID (1-255)
- **Network Settings:** WiFi SSID, Password, Channel, ESP-NOW
- **Game Parameters:** Duration, Penalties, Scoring
- **Hardware Configuration:**
  - Main Unit: I2C Pins, Buttons, Buzzer
  - Laser Unit: Laser Pin, Sensor Pin, LED Pins, Thresholds

**Wichtig:** Diese Datei definiert CONFIG_* Makros die im Code verwendet werden!

#### `main/CMakeLists.txt`
**Zweck:** Build-Konfiguration für main-Komponente  
**REQUIRES:**
```cmake
nvs_flash esp_wifi esp_netif esp_event esp_http_server driver
display_manager game_logic espnow_manager laser_control sensor_manager
```

---

### `/components` - Wiederverwendbare Komponenten

Alle Komponenten folgen diesem Muster:
```
component_name/
├── component_name.c          # Implementierung
├── include/
│   └── component_name.h      # Public API Header
└── CMakeLists.txt            # Build-Konfiguration
```

---

#### `components/display_manager/`
**Zweck:** OLED Display (SSD1306/SH1106) Steuerung  

**Architektur:**
- `display_manager.c/h` - Abstraktes Interface für alle Display-Treiber
- `ssd1306.c/h` - SSD1306-spezifischer Treiber (128x64, I2C)
- `sh1106.c/h` - SH1106-spezifischer Treiber (TODO)

**Display Manager API:**
```c
esp_err_t display_manager_init(gpio_num_t sda, gpio_num_t scl, uint32_t freq)
esp_err_t display_set_screen(display_screen_t screen)
esp_err_t display_game_status(uint32_t time, uint16_t breaks, int32_t score)
esp_err_t display_countdown(uint8_t seconds)
esp_err_t display_text(const char *message, uint8_t line)
esp_err_t display_update(void)
```

**SSD1306 Driver API:**
```c
esp_err_t ssd1306_init(gpio_num_t sda, gpio_num_t scl, uint32_t freq)
esp_err_t ssd1306_clear(void)
esp_err_t ssd1306_update(void)
void ssd1306_draw_string(uint8_t x, uint8_t page, const char *str)
void ssd1306_draw_char(uint8_t x, uint8_t page, char c)
void ssd1306_draw_large_digit(uint8_t x, uint8_t page, char digit)
void ssd1306_draw_hline(uint8_t page, uint8_t pattern)
esp_err_t ssd1306_set_contrast(uint8_t contrast)
esp_err_t ssd1306_display_power(bool on)
```

**Implementation Details:**
- **SSD1306:** 128x64 OLED, I2C 0x3C, 1024-byte framebuffer
- **Font:** 5x7 ASCII (32-127), 6 pixels pro Zeichen mit Spacing
- **Pages:** 8 pages (0-7), jede Page = 8 pixels hoch
- **Large Digits:** 3x skalierte Ziffern für Countdown
- **Driver Selection:** Basierend auf CONFIG_OLED_SSD1306 / CONFIG_OLED_SH1106

**Screens:**
- SCREEN_IDLE - Welcome Screen
- SCREEN_MENU - Hauptmenü
- SCREEN_GAME_COUNTDOWN - Countdown vor Spielstart
- SCREEN_GAME_RUNNING - Aktives Spiel
- SCREEN_GAME_PAUSED - Pausiert
- SCREEN_GAME_COMPLETE - Endergebnis
- SCREEN_SETTINGS - Einstellungen
- SCREEN_STATS - Statistiken

**Dependencies:** `driver` (I2C)

**Verwendet von:** Main Unit (CONTROL Module)

---

#### `components/game_logic/`
**Zweck:** Spiellogik, Scoring, Timing, State Management  
**Hauptfunktionen:**
```c
esp_err_t game_logic_init(void)
esp_err_t game_start(game_mode_t mode, const char *player_name)  // Broadcasts MSG_GAME_START
esp_err_t game_stop(void)                                         // Broadcasts MSG_GAME_STOP, sets completion status
esp_err_t game_pause(void)
esp_err_t game_resume(void)
esp_err_t game_beam_broken(uint8_t sensor_id)
esp_err_t game_get_player_data(player_data_t *data)
esp_err_t game_get_stats(game_stats_t *stats)
esp_err_t game_set_config(const game_config_t *config)
```

**Game States:**
- GAME_STATE_IDLE - System bereit
- GAME_STATE_READY - Bereit für Start
- GAME_STATE_COUNTDOWN - Countdown läuft (aktuell übersprungen im Web Interface)
- GAME_STATE_RUNNING - Spiel aktiv (nach game_start direkt aktiv)
- GAME_STATE_PENALTY - Penalty Mode
- GAME_STATE_PAUSED - Pausiert
- GAME_STATE_COMPLETE - Beendet
- GAME_STATE_ERROR - Fehlerzustand

**Completion Status:**
- COMPLETION_NONE - Spiel noch nicht beendet
- COMPLETION_SOLVED - Via Finish-Button beendet (erfolgreich abgeschlossen)
- COMPLETION_ABORTED_TIME - Max-Zeit überschritten (abgebrochen/canceled)
- COMPLETION_ABORTED_MANUAL - Manuell abgebrochen via Web-Interface (abgebrochen/canceled)

**Spielende-Logik:**
- **Erfolgreich abgeschlossen (Complete):** Nur wenn MSG_FINISH_PRESSED vom Finish-Button-Device empfangen wird
- **Abgebrochen (Canceled):** 
  - Bei max_time Überschreitung → COMPLETION_ABORTED_TIME
  - Bei manuellem Stop via Web-Interface → COMPLETION_ABORTED_MANUAL
- Display zeigt "GAME COMPLETE!" nur bei COMPLETION_SOLVED
- Bei ABORTED_TIME/MANUAL sollte "GAME CANCELED!" angezeigt werden

**Finish-Button-Device (TODO - Hardware):**
- Neuer ESP32-Modul-Typ: FINISH
- Sendet MSG_FINISH_PRESSED wenn Button gedrückt wird
- Main Unit empfängt MSG und ruft game_finish() auf
- game_finish() setzt completion = COMPLETION_SOLVED
- Spiel wird als "successfully completed" markiert

**Game Modes:**
- GAME_MODE_SINGLE_SPEEDRUN - Einzelspieler Speedrun
- GAME_MODE_MULTIPLAYER - Mehrspieler
- GAME_MODE_TRAINING - Training (keine Penalties)
- GAME_MODE_CUSTOM - Benutzerdefiniert

**Spielkonzept (Zeit-basiert):**
- Zeit zählt **aufwärts** von 0 Sekunden
- Penalty-Sekunden werden **zur Zeit addiert** (nicht subtrahiert)
- Kein Lives/Score-System (entfernt)
- Nur Zeit und Beam Breaks werden getrackt
- Optionales Max-Zeit-Limit (0 = unbegrenzt)

**Dependencies:** `esp_event`, `freertos`, `esp_timer`

**Verwendet von:** Main Unit (CONTROL Module)

---

#### `components/wifi_ap_manager/`
**Zweck:** WiFi Access Point Management für Main Unit  
**Hauptfunktionen:**
```c
esp_err_t wifi_ap_init(wifi_ap_config_t *config)
uint8_t wifi_ap_get_connected_stations(void)
esp_err_t wifi_ap_get_ip_info(esp_netif_ip_info_t *ip_info)
```

**Konfiguration:**
```c
typedef struct {
    char ssid[32];           // WiFi SSID
    char password[64];       // WiFi Passwort (8-64 Zeichen)
    uint8_t channel;         // WiFi Kanal (1-13)
    uint8_t max_connections; // Max. Verbindungen
    bool hidden;             // Hidden SSID
} wifi_ap_config_t;
```

**Verwendung:**
- Erstellt WiFi-Netzwerk für Web-Interface
- DHCP-Server auf 192.168.4.x Subnetz
- Event-Handler für Station Connect/Disconnect
- Typische IP: 192.168.4.1

**Dependencies:** `esp_wifi`, `esp_event`, `esp_netif`

**Konfigurationsoptionen (Kconfig):**
- CONFIG_WIFI_SSID (default: "LaserParcour")
- CONFIG_WIFI_PASSWORD
- CONFIG_WIFI_CHANNEL (default: 6)
- CONFIG_MAX_STA_CONN (default: 4)

**Verwendet von:** Main Unit (CONTROL Module)

---

#### `components/button_handler/`
**Zweck:** Physikalische Button-Eingabe mit Debouncing und Event-Erkennung  
**Hauptfunktionen:**
```c
esp_err_t button_handler_init(button_config_t *buttons, uint8_t count, 
                              button_event_callback_t callback)
bool button_get_state(uint8_t button_id)
```

**Button-Konfiguration:**
```c
typedef struct {
    int pin;                    // GPIO Pin (-1 zum Deaktivieren)
    uint16_t debounce_time_ms;  // Entprellzeit (default: 50ms)
    uint16_t long_press_time_ms; // Long-Press Schwelle (default: 1000ms)
    bool pull_up;               // Interner Pull-Up
    bool active_low;            // Button active-low (typisch bei Pull-Up)
} button_config_t;
```

**Event-Typen:**
- BUTTON_EVENT_PRESSED - Button gedrückt
- BUTTON_EVENT_RELEASED - Button losgelassen
- BUTTON_EVENT_CLICK - Einzelklick erkannt
- BUTTON_EVENT_DOUBLE_CLICK - Doppelklick (innerhalb 300ms)
- BUTTON_EVENT_LONG_PRESS - Long-Press (>1000ms)

**Callback:**
```c
typedef void (*button_event_callback_t)(uint8_t button_id, button_event_t event);
```

**Implementation:**
- Polling-Task mit 100Hz (10ms Intervall)
- Konfigurierbares Debouncing
- Doppelklick-Fenster: 300ms
- Pin -1 Support: Buttons werden ignoriert

**Dependencies:** `driver` (GPIO), `freertos`, `esp_timer`

**Konfigurationsoptionen (Kconfig):**
- CONFIG_BUTTON1_PIN bis CONFIG_BUTTON3_PIN
- CONFIG_DEBOUNCE_TIME (default: 50ms)
- CONFIG_ENABLE_BUTTONS (optional feature flag)
- CONFIG_ENABLE_BUTTON3_DEBUG_FINISH (enable Button 3 as debug finish)

**Verwendet von:** Main Unit (CONTROL Module)

---

#### `components/buzzer/`
**Zweck:** PWM-basiertes Audio-Feedback mit LEDC-Peripheral  
**Hauptfunktionen:**
```c
esp_err_t buzzer_init(int gpio_pin)
void buzzer_play_tone(uint32_t frequency, uint32_t duration_ms)
void buzzer_play_pattern(buzzer_pattern_t pattern)
void buzzer_set_volume(uint8_t volume)  // 0-100%
void buzzer_stop(void)
```

**Vordefinierte Patterns:**
- BUZZER_PATTERN_BEEP - Kurzer Beep (200ms, 1000Hz)
- BUZZER_PATTERN_SUCCESS - Erfolg (3 aufsteigende Noten)
- BUZZER_PATTERN_ERROR - Fehler (2 tiefe Noten)
- BUZZER_PATTERN_COUNTDOWN - Countdown-Beeps (3x 100ms)
- BUZZER_PATTERN_GAME_START - Spielstart-Fanfare
- BUZZER_PATTERN_GAME_END - Spielende-Sequenz

**Musikalische Noten:**
Definiert C4 (262Hz) bis C5 (523Hz) - chromatische Tonleiter

**Implementation:**
- LEDC Timer 1, Channel 0
- 13-bit Auflösung, 5000Hz Basisfrequenz
- Lautstärke via PWM Duty-Cycle
- FreeRTOS Task für Pattern-Sequenzen

**Dependencies:** `driver` (LEDC), `freertos`

**Konfigurationsoptionen (Kconfig):**
- CONFIG_BUZZER_PIN (-1 zum Deaktivieren)
- CONFIG_ENABLE_BUZZER (optional feature flag)

**Verwendet von:** Main Unit (CONTROL Module)

---

#### `components/web_server/`
**Zweck:** HTTP-Server mit Web-Interface und REST-API für Spielsteuerung  
**Hauptfunktionen:**
```c
esp_err_t web_server_init(httpd_handle_t *server_out, 
                         game_control_callback_t callback)
void web_server_update_status(const game_status_t *status)
```

**HTTP-Endpoints:**
- GET / - Haupt-Webinterface (HTML-Seite)
- GET /api/status - Aktueller Spielstatus (JSON)
- POST /api/game/start - Spiel starten
- POST /api/game/stop - Spiel stoppen
- POST /api/game/pause - Spiel pausieren
- POST /api/game/resume - Spiel fortsetzen

**API Response Format:**
```json
{
  "state": "IDLE",
  "lives": 3,
  "score": 0,
  "time_remaining": 60,
  "current_level": 1
}
```

**Callback-Mechanismus:**
```c
typedef esp_err_t (*game_control_callback_t)(const char *command, const char *data);
```

**Implementation:**
- Port 80 (Standard-HTTP)
- Minimalistisches HTML/CSS/JavaScript UI
- AJAX-Polling für Live-Updates
- Status-Cache via web_server_update_status()
- Callback ermöglicht Web-Befehle → Game Logic

**Dependencies:** `esp_http_server`

**Konfiguration:**
- Server läuft auf WiFi AP IP (192.168.4.1)
- Keine zusätzlichen Kconfig-Optionen nötig

**Verwendet von:** Main Unit (CONTROL Module)

---

#### `components/sd_card_manager/`
**Zweck:** SD Card Management für Web-Interface-Dateien und High Scores  
**Hauptfunktionen:**
```c
esp_err_t sd_card_manager_init(const sd_card_config_t *config)
esp_err_t sd_card_get_info(sd_card_info_t *info)
bool sd_card_is_mounted(void)
const char* sd_card_get_mount_point(void)
```

**SD Card Info:**
```c
typedef struct {
    uint64_t total_bytes;
    uint64_t free_bytes;
    char card_type[16];
    bool web_dir_available;  // /web Verzeichnis vorhanden?
} sd_card_info_t;
```

**Features:**
- SPI-Modus für SD/SDHC/SDXC Karten
- FAT32 Dateisystem
- Mount Point: /sdcard
- Web-Interface von SD: /sdcard/web/
- High Scores: /sdcard/scores/
- Optionale Konfiguration (NULL = Menuconfig Pins)

**Dependencies:** `fatfs`, `sdmmc`, `driver` (SPI)

**Konfigurationsoptionen (Kconfig):**
- CONFIG_ENABLE_SD_CARD (optional feature flag)
- CONFIG_SD_MISO_PIN
- CONFIG_SD_MOSI_PIN
- CONFIG_SD_CLK_PIN
- CONFIG_SD_CS_PIN

**Verwendet von:** Main Unit (CONTROL Module)

---

#### `components/espnow_manager/`
**Zweck:** ESP-NOW Kommunikation zwischen Main Unit und Laser Units  
**Hauptfunktionen:**
```c
esp_err_t espnow_manager_init(uint8_t channel, espnow_recv_callback_t callback)
esp_err_t espnow_send_message(const uint8_t *mac, espnow_msg_type_t type, ...)
esp_err_t espnow_broadcast_message(espnow_msg_type_t type, const uint8_t *data, size_t len)
esp_err_t espnow_add_peer(const uint8_t *mac, uint8_t id, uint8_t role)
esp_err_t espnow_remove_peer(const uint8_t *mac)
esp_err_t espnow_get_peers(espnow_peer_info_t *peers, size_t max, size_t *count)
```

**Message Types:**
- MSG_GAME_START (0x01) - Spiel starten
- MSG_GAME_STOP (0x02) - Spiel stoppen
- MSG_BEAM_BROKEN (0x03) - Beam unterbrochen
- MSG_STATUS_UPDATE (0x04) - Status Update
- MSG_CONFIG_UPDATE (0x05) - Konfiguration
- MSG_HEARTBEAT (0x06) - Keep-Alive
- MSG_PAIRING_REQUEST (0x07) - Pairing Request
- MSG_PAIRING_RESPONSE (0x08) - Pairing Response
- MSG_LASER_ON (0x09) - Laser einschalten
- MSG_LASER_OFF (0x0A) - Laser ausschalten
- MSG_SENSOR_CALIBRATE (0x0B) - Sensor kalibrieren
- MSG_RESET (0x0C) - Modul zurücksetzen
- MSG_CHANNEL_CHANGE (0x0D) - WiFi channel change notification
- MSG_CHANNEL_ACK (0x0E) - Channel change acknowledgement
- MSG_FINISH_PRESSED (0x0F) - Finish button pressed (game completed successfully)

**Message Structure:**
```c
typedef struct {
    uint8_t msg_type;       // Message Type
    uint8_t module_id;      // Sender Module ID
    uint32_t timestamp;     // Timestamp (ms)
    uint8_t data[32];       // Payload
    uint16_t checksum;      // CRC16
} espnow_message_t;
```

**Dependencies:** `esp_wifi`, `esp_event`, `esp_netif`, `nvs_flash`, `esp_timer`

**Verwendet von:** Main Unit & Laser Unit (BEIDE Module)

---

#### `components/laser_control/`
**Zweck:** Laser-Dioden PWM-Steuerung mit Sicherheitsfunktionen  
**Hauptfunktionen:**
```c
esp_err_t laser_control_init(gpio_num_t laser_pin)
esp_err_t laser_turn_on(uint8_t intensity)         // 0-100%
esp_err_t laser_turn_off(void)
esp_err_t laser_set_intensity(uint8_t intensity)
laser_status_t laser_get_status(void)
esp_err_t laser_set_safety_timeout(bool enable)    // 10 Minuten Auto-Off
```

**Laser Status:**
- LASER_OFF
- LASER_ON
- LASER_STANDBY
- LASER_ERROR

**Sicherheit:**
- Automatischer Timeout (10 Minuten)
- PWM-basierte Intensitätskontrolle
- Status-Monitoring

**Dependencies:** `driver` (LEDC PWM), `esp_timer`

**Verwendet von:** Laser Unit (LASER Module)

---

#### `components/sensor_manager/`
**Zweck:** ADC-basierte Photoresistor/Photodiode Beam-Detection  
**Hauptfunktionen:**
```c
esp_err_t sensor_manager_init(uint8_t adc_ch, uint16_t threshold, uint32_t debounce)
esp_err_t sensor_register_callback(beam_break_callback_t callback)
esp_err_t sensor_read_value(uint16_t *value)
sensor_status_t sensor_get_status(void)
esp_err_t sensor_set_threshold(uint16_t threshold)
esp_err_t sensor_calibrate(void)
esp_err_t sensor_start_monitoring(void)
esp_err_t sensor_stop_monitoring(void)
```

**Sensor Status:**
- SENSOR_BEAM_DETECTED - Beam vorhanden
- SENSOR_BEAM_BROKEN - Beam unterbrochen
- SENSOR_ERROR - Sensor-Fehler

**Features:**
- 12-bit ADC Auflösung (0-4095)
- Konfigurierbarer Threshold (default: 2000)
- **Logik:** ADC-Wert **über** Threshold = Beam vorhanden, **unter** Threshold = Beam gebrochen
- **LDR-Setup:** Ohne Laser ~0.7V (~850 ADC), mit Laser ~3.3V (~4095 ADC)
- Debouncing (Anti-Flackern)
- Callback-basierte Events
- Automatische Kalibrierung
- Live ADC-Wert Logging (jede Sekunde) für Debugging

**Dependencies:** `driver` (ADC), `esp_adc`

**Verwendet von:** Laser Unit (LASER Module)

---

## 🔧 Build-System

### CMake Dependency-Hierarchie

```
main (CONTROL)
├── display_manager
├── game_logic
│   ├── esp_event
│   ├── freertos
│   └── esp_timer
└── espnow_manager
    ├── esp_wifi
    ├── esp_event
    ├── esp_netif
    ├── nvs_flash
    └── esp_timer

main (LASER)
├── laser_control
│   ├── driver
│   └── esp_timer
├── sensor_manager
│   └── driver (ADC)
└── espnow_manager
    └── (siehe oben)
```

### Build-Befehle

```bash
# Konfigurieren (Module Role wählen)
idf.py menuconfig

# Kompilieren
idf.py build

# Flashen
idf.py -p /dev/ttyUSB0 flash

# Monitor
idf.py -p /dev/ttyUSB0 monitor

# Flash + Monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Größenanalyse
idf.py size-components
```

---

## ⚙️ Konfiguration

### Module Role Selection (menuconfig)

**Main Unit (CONTROL):**
```
CONFIG_MODULE_ROLE_CONTROL=y
CONFIG_MODULE_ID=1
CONFIG_I2C_SDA_PIN=8
CONFIG_I2C_SCL_PIN=9
CONFIG_BUTTON1_PIN=2
CONFIG_BUZZER_PIN=5
```

**Laser Unit (LASER):**
```
CONFIG_MODULE_ROLE_LASER=y
CONFIG_MODULE_ID=2
CONFIG_LASER_PIN=10
CONFIG_SENSOR_PIN=0
CONFIG_SENSOR_THRESHOLD=500
CONFIG_LASER_STATUS_LED_PIN=2
CONFIG_SENSOR_LED_GREEN_PIN=1
CONFIG_SENSOR_LED_RED_PIN=2
```

### Wichtige CONFIG-Optionen

| Option | Typ | Default | Beschreibung |
|--------|-----|---------|--------------|
| CONFIG_MODULE_ROLE | choice | CONTROL | CONTROL oder LASER |
| CONFIG_MODULE_ID | int | 1 | Eindeutige Modul-ID (1-255) |
| CONFIG_WIFI_CHANNEL | int | 1 | WiFi/ESP-NOW Kanal (1-13) |
| CONFIG_ESPNOW_CHANNEL | int | 1 | ESP-NOW Kanal (muss mit WiFi übereinstimmen) |
| CONFIG_GAME_DURATION | int | 180 | Spieldauer (Sekunden) |
| CONFIG_PENALTY_TIME | int | 5 | Penalty pro Beam Break (Sek.) |
| CONFIG_SENSOR_THRESHOLD | int | 500 | ADC Threshold (0-4095) |
| CONFIG_DEBOUNCE_TIME | int | 100 | Debounce Zeit (ms) |

---

## 🔄 Programmablauf

### Main Unit (CONTROL)

1. **System Init:**
   - NVS initialisieren
   - Netzwerk-Stack starten
   - System-Info ausgeben

2. **Module Init:**
   - OLED Display (I2C)
   - Game Logic
   - ESP-NOW Manager (als Coordinator)
   - Display auf SCREEN_IDLE setzen

3. **Main Loop:**
   - Button-Events verarbeiten (TODO)
   - Display aktualisieren
   - ESP-NOW Messages empfangen & verarbeiten
   - Game State aktualisieren

4. **Event Handling:**
   - MSG_BEAM_BROKEN → game_beam_broken() aufrufen
   - MSG_PAIRING_REQUEST → Laser Unit registrieren
   - MSG_STATUS_UPDATE → Status loggen

### Laser Unit (LASER)

1. **System Init:**
   - NVS initialisieren
   - Netzwerk-Stack starten
   - System-Info ausgeben

2. **Module Init:**
   - Laser Control (PWM)
   - Sensor Manager (ADC)
   - Status LEDs (GPIO)
   - ESP-NOW Manager (als Client)

3. **Pairing:**
   - Periodischer Pairing Request (alle 5 Sek.)
   - Timer stoppt bei MSG_PAIRING_RESPONSE
   - Status-LED leuchtet bei erfolgreichem Pairing

4. **Main Loop:**
   - Sensor kontinuierlich überwachen
   - Beam-Break Events → MSG_BEAM_BROKEN senden
   - ESP-NOW Messages verarbeiten:
     - MSG_GAME_START → Laser einschalten
     - MSG_GAME_STOP → Laser ausschalten
     - MSG_LASER_ON/OFF → Laser steuern
     - MSG_RESET → Laser ausschalten, LEDs aus, Pairing zurücksetzen

5. **LED Feedback:**
   - **Status-LED**: Verbindungsstatus (blinkt während Pairing, dauerhaft an wenn verbunden/paired)
   - **Grün+Rot LEDs**: Dual-Modus
     - **Manueller Modus** (MSG_LASER_ON/OFF): Beide LEDs an/aus
     - **Game-Modus** (MSG_GAME_START/STOP): 
       - Grün-LED: Beam OK (gesteuert durch sensor_manager)
       - Rot-LED: Beam unterbrochen (gesteuert durch sensor_manager)

---

## � Bekannte Fixes und Lösungen

### ESP-NOW Pairing Issues

**Problem:** ESP_ERR_ESPNOW_IF beim Senden von Messages
**Ursache:** WiFi war nur im AP-Modus, ESP-NOW benötigt STA-Interface
**Lösung:**
1. Main Unit WiFi explizit in APSTA-Modus setzen BEVOR wifi_connect_with_fallback()
2. wifi_ap_init() modifiziert um APSTA-Modus zu erhalten wenn bereits gesetzt

**Code-Änderungen:**
- `main/main.c`: WiFi APSTA-Init vor wifi_connect_with_fallback()
- `wifi_ap_manager.c`: Prüfung auf aktuellen WiFi-Modus vor set_mode(WIFI_MODE_AP)

### Game Start Broadcasting

**Problem:** game_start() änderte State aber sendete keine Messages an Laser Units
**Lösung:** game_start() ruft espnow_broadcast_message(MSG_GAME_START) auf
**Status:** RUNNING-State wird direkt gesetzt (kein COUNTDOWN für Web-Interface)

### Spielsystem-Vereinfachung (2025-01-08)

**Änderung:** Komplette Umstellung auf zeit-basiertes Spiel ohne Lives/Score

**Neues Spielkonzept:**
- **Zeit zählt aufwärts** von 0 Sekunden
- **Penalty-Sekunden werden zur Zeit addiert** (nicht mehr subtrahiert)
- **Kein Score-System** mehr (entfernt)
- **Kein Lives-System** (entfernt)
- **Nur Zeit und Beam Breaks** werden getrackt

**Completion Status:**
- `COMPLETION_SOLVED` - Spiel via Finish-Button beendet (TODO: Button-Device)
- `COMPLETION_ABORTED_TIME` - Spiel durch Max-Zeit abgebrochen
- `COMPLETION_ABORTED_MANUAL` - Spiel manuell per Web-Interface abgebrochen

**Code-Änderungen:**
- `game_logic.h`: `completion_status_t` Enum hinzugefügt
- `player_data_t`: `score` entfernt, `completion` hinzugefügt
- `game_config_t`: `duration` → `max_time` (0 = unbegrenzt), alle Score-Felder entfernt
- `game_logic.c`: Penalty-Zeit wird **addiert** statt subtrahiert
- `game_logic.c`: `game_calculate_score()` Funktion entfernt
- `display_manager.h/c`: Score-Parameter aus allen Funktionen entfernt
- `display_manager.c`: Display zeigt nur noch Zeit (MM:SS.ms) und Beam Breaks
- `main.c`: Alle display_game_status/display_game_results Aufrufe ohne Score

**Penalty-System (NEU - Addition statt Subtraktion):**
```c
// Alte Version (FALSCH - Clock pause):
player_data->elapsed_time = raw_elapsed - total_penalty_time;

// Neue Version (KORREKT - Penalty seconds added):
player_data->elapsed_time = raw_elapsed + total_penalty_time;
```

**Beispiel-Spielablauf:**
1. Start: Zeit = 0:00
2. Nach 30 Sekunden: Beam Break → +15s Penalty
3. Anzeige: 0:45 (30s + 15s Penalty)
4. Finish nach insgesamt 2 Minuten + 3 Breaks (45s Penalty)
5. Endzeit: 2:45 (2 min + 3×15s)

**Max-Zeit-Limit:**
- `configuration.max_time` (Sekunden, 0 = unbegrenzt)
- Bei Überschreitung: Automatischer Abort mit `COMPLETION_ABORTED_TIME`
- **IMPLEMENTIERT (2025-01-08):** Auto-Stop in `game_get_player_data()`
  - Prüft elapsed_time gegen max_time bei jedem Abruf
  - Setzt `completion = COMPLETION_ABORTED_TIME`
  - Ruft automatisch `game_stop()` auf
  - Sendet MSG_GAME_STOP an alle Laser Units

**Penalty-System (Sofortige Addition - 2025-01-08):**
```c
// Bei Beam-Break in game_beam_broken():
total_penalty_time += (configuration.penalty_time * 1000);  // SOFORT addiert
penalty_start_time = jetzt;  // Penalty-Display startet (3 Sekunden)

// In game_get_player_data():
player_data->elapsed_time = raw_elapsed + total_penalty_time;  // Penalty ist bereits enthalten

// Penalty-Display wechselt automatisch nach 3 Sekunden zurück zu RUNNING
if (penalty_elapsed >= PENALTY_DISPLAY_TIME_MS) {  // 3000ms = 3 Sekunden
    current_state = GAME_STATE_RUNNING;
}
```

**Penalty-Phase Verhalten (NUR Display-Anzeige):**
- PENALTY ist **keine echte Spielphase**, sondern nur eine **visuelle Bestrafungs-Anzeige**
- Die Penalty-Zeit (z.B. 15 Sekunden) wird **sofort zur Gesamtzeit addiert**
- Display zeigt "*** PENALTY! ***" für **3 Sekunden** (fest eingestellt)
- Nach 3 Sekunden automatischer Wechsel zu RUNNING
- **Während der 3-Sekunden-Anzeige können KEINE weiteren Beam Breaks registriert werden** (blockiert)
- Dies verhindert mehrere Penalties bei schnellen Mehrfach-Breaks

**Beispiel - Penalty-Ablauf:**
1. Zeit 0:30: Beam Break → Zeit springt sofort auf 0:45 (+15s Penalty addiert)
2. Display zeigt 3 Sekunden lang "*** PENALTY! ***" mit Zeit und Breaks
3. Nach 3 Sekunden: Display wechselt zurück zu "GAME ACTIVE"
4. Weitere Beam Breaks sind wieder möglich

**Display bei GAME_STATE_COMPLETE:**
- Zeigt "GAME COMPLETE!" bei COMPLETION_SOLVED (erfolgreicher Abschluss via Finish-Button)
- Zeigt "GAME CANCELED!" bei COMPLETION_ABORTED_TIME oder COMPLETION_ABORTED_MANUAL
- Gesamtzeit im Format MM:SS.ms (inklusive aller Penalties) mit Label "Total Time:"
- Anzahl der Beam Breaks
- Diese Werte bleiben dauerhaft sichtbar bis zum nächsten Spiel

**Beispiel-Spielablauf mit Max-Zeit:**
1. Start: Zeit = 0:00, max_time = 180 (3 Minuten)
2. Nach 2:30: Beam Break → +15s → Zeit: 2:45
3. Nach 2:50: Beam Break → +15s → Zeit: 3:05
4. **Auto-Stop**: Zeit > 3:00 → Game stoppt automatisch mit COMPLETION_ABORTED_TIME
5. Display zeigt: "TIME LIMIT!" / Final Zeit: 3:05

### Laser Unit MSG_RESET Support

**Problem:** Laser Unit kannte MSG_RESET (0x0C) nicht → "Unknown message type"
**Lösung:** MSG_RESET Handler hinzugefügt:
- Laser ausschalten
- Alle LEDs ausschalten  
- is_paired auf false setzen
- Pairing-Timer neu starten wenn nötig

### Web Interface Unit Status

**Problem:** Webinterface zeigt Units als "offline" obwohl verbunden
**Lösung:** 
- Units werden bei jeder ESP-NOW Message via game_update_laser_unit() aktualisiert
- `last_seen` Timestamp wird gesetzt und `is_online = true`
- Status wird als "offline" markiert wenn keine Message seit 5 Sekunden
- **GELÖST (2025-01-08):** Heartbeat-System implementiert für persistente Online-Anzeige

**Heartbeat-System (2025-01-08):**
- Laser Units senden alle 3 Sekunden MSG_HEARTBEAT Broadcasts
- Main Unit aktualisiert `last_seen` Timestamp bei jedem Heartbeat
- Laser Units ignorieren eigene Heartbeat-Broadcasts (MSG_HEARTBEAT Handler)
- Units bleiben online solange Heartbeats empfangen werden

### Web Interface Status Updates

**Problem:** Game Status (IDLE/RUNNING/PAUSED) änderte sich nicht im Web Interface
**Lösung:**
- status_handler() ruft jetzt direkt game_get_state() und game_get_player_data() auf
- Echte Spielzeit und Score werden live berechnet und angezeigt
- Alert-Boxen bei Start/Stop entfernt (nur noch console.log)
- Status wird alle 2 Sekunden automatisch aktualisiert

### Sensor Monitoring on Game Start

**Problem:** Laser Unit schaltet Laser ein bei MSG_GAME_START, aber ADC Sensor wird nicht gestartet
**Lösung:** 
- MSG_GAME_START Handler ruft sensor_start_monitoring() auf
- MSG_GAME_STOP Handler ruft sensor_stop_monitoring() auf
- Beam breaks werden nun korrekt erkannt während des Spiels

### Pairing Timer Restart on Reset

**Problem:** Nach Main Unit Neustart versucht Laser Unit kein erneutes Pairing
**Lösung:**
- MSG_RESET Handler setzt `is_paired = false`
- MSG_RESET Handler startet pairing_timer neu mit esp_timer_start_periodic()
- Laser Unit sendet wieder alle 5 Sekunden MSG_PAIRING_REQUEST
- Automatisches Re-Pairing nach Main Unit Neustart funktioniert

**Code-Änderungen (2025-01-07):**
- `main/main.c`: Sensor Start/Stop in MSG_GAME_START/STOP Handlers
- `main/main.c`: Pairing-Timer Neustart in MSG_RESET Handler
- `web_server/web_server.c`: Echte Game-Daten in status_handler()
- `web_server/web_server.c`: Alert-Boxen entfernt, nur console.log
- `web_server/web_server.c`: esp_timer.h Include hinzugefügt

**Code-Änderungen (2025-01-08):**
- `sensor_manager/sensor_manager.c`: Default Threshold von 500 auf 2000 erhöht
- `sensor_manager/sensor_manager.c`: Live ADC-Logging jede Sekunde hinzugefügt
- `main/Kconfig.projbuild`: SENSOR_THRESHOLD default auf 2000, bessere Dokumentation
- `main/Kconfig.projbuild`: MODULE_ROLE_FINISH hinzugefügt (dritte Rolle neben CONTROL/LASER)
- `main/Kconfig.projbuild`: CONFIG_FINISH_BUTTON_PIN, CONFIG_FINISH_STATUS_LED_PIN, CONFIG_FINISH_BUTTON_LED_PIN
- `main/main.c`: IS_FINISH_MODULE Define für Finish Button Rolle
- `main/main.c` (FINISH): init_finish_button_unit() komplett implementiert
- `main/main.c` (FINISH): Button ISR Handler + button_handler_task für Button-Logik
- `main/main.c` (FINISH): LED-Steuerung (Status-LED blinkt, Button-LED geht aus beim Drücken)
- `main/main.c` (FINISH): Multi-Channel Scanning + Pairing wie Laser Units
- `main/main.c` (FINISH): espnow_recv_callback_finish() mit MSG_PAIRING_RESPONSE, MSG_RESET
- `main/main.c` (FINISH): Heartbeat-Timer (3 Sekunden) nach erfolgreichem Pairing
- `main/main.c` (FINISH): Sendet MSG_FINISH_PRESSED bei Button-Druck
- `main/main.c` (LASER): Multi-Channel Scanning in pairing_timer_callback() implementiert
- `main/main.c` (LASER): Channel-Scan State Reset in MSG_PAIRING_RESPONSE und MSG_RESET
- `main/main.c` (LASER): Verwendung von espnow_change_channel() für Broadcast-Peer Update
- `main/main.c` (LASER): MSG_HEARTBEAT Handler hinzugefügt (ignoriert eigene Broadcasts)
- `main/main.c` (LASER): LED-Blink während Channel-Scanning
- `main/main.c` (LASER): Heartbeat-Timer startet nach erfolgreichem Pairing (3 Sekunden)
- `main/main.c` (LASER): Pairing Request mit role=1 (Laser Unit)
- `main/main.c` (CONTROL): MSG_HEARTBEAT Handler zur Aktualisierung von last_seen
- `main/main.c` (CONTROL): MSG_RESET Broadcast beim Startup für Re-Pairing
- `main/main.c` (CONTROL): MSG_PAIRING_REQUEST Handler extrahiert Role aus Data
- `main/main.c` (CONTROL): Unterscheidung zwischen "Laser Unit" und "Finish Button" in Logs
- `game_logic.h`: laser_unit_info_t erweitert um role-Feld (0=unknown, 1=laser, 2=finish)
- `wifi_ap_manager.c`: wifi_apsta_init() Funktion für korrekte APSTA-Initialisierung
- `main/main.c` (CONTROL): Verwendung von wifi_apsta_init() für STA+AP netif Erstellung
- `espnow_manager.c`: espnow_change_channel() aktualisiert Broadcast-Peer beim Channel-Wechsel
- `espnow_manager.c`: espnow_manager_init() verwendet aktuellen WiFi-Channel statt konfigurierten
- `espnow_manager.c`: espnow_add_peer() verwendet aktuellen WiFi-Channel für neue Peers
- `game_logic.c`: game_control_laser() sendet Unicast statt Broadcast (nur spezifische Unit)
- `web_server.c`: Timer-Display nur bei RUNNING/PAUSED/PENALTY (nicht bei IDLE/COMPLETE)
- `web_server.c`: units_list_handler() fügt role und role_name zu JSON hinzu
- `web_server/index.html`: Finish Button mit 🏁-Icon und grüner Border angezeigt
- `web_server/index.html`: Laser ON/OFF Controls nur bei Laser Units (nicht bei Finish)
- `game_logic.c`: PENALTY ist nur Display-Anzeige (3 Sekunden), keine echte Spielphase
- `game_logic.c`: Beam Breaks nur im RUNNING State akzeptiert (PENALTY blockiert)
- `display_manager.c`: display_game_results() zeigt Gesamtzeit + Beam Breaks
- `espnow_manager.h`: MSG_FINISH_PRESSED (0x0F) für Finish-Button-Device hinzugefügt
- `game_logic.h`: game_finish() Funktion für erfolgreichen Abschluss via Finish-Button
- `game_logic.c`: game_finish() implementiert - setzt COMPLETION_SOLVED
- `game_logic.c`: game_stop() setzt COMPLETION_ABORTED_MANUAL wenn nicht bereits gesetzt
- `main/main.c` (CONTROL): MSG_FINISH_PRESSED Handler ruft game_finish() auf
- `display_manager.h`: display_game_results() erweitert um completion_status Parameter
- `display_manager.c`: Zeigt "GAME COMPLETE!" (SOLVED) oder "GAME CANCELED!" (ABORTED)
- `display_manager.c`: "Total Time:" Label hinzugefügt, prominentere Zeitanzeige
- `display_manager/CMakeLists.txt`: game_logic als REQUIRES hinzugefügt für completion_status_t
- `main/main.c`: display_game_results() Aufruf mit player_data.completion erweitert

### Sensor Detection Threshold

**Problem:** Laser Breaks wurden nicht erkannt trotz funktionierendem Laser
**Ursache:** Default Threshold von 500 war zu niedrig für LDR-Setup
**LDR Verhalten:**
- Ohne Laser (dunkel): ~0.7V = ~850 ADC-Wert
- Mit Laser (hell): ~3.3V = ~4095 ADC-Wert
- Logik: ADC > Threshold = Beam vorhanden, ADC < Threshold = Beam gebrochen

**Lösung:**
- Default Threshold auf 2000 erhöht (zwischen 850 und 4095)
- Live ADC-Logging jede Sekunde für einfaches Debugging
- Kconfig Dokumentation verbessert

**Debugging:**
Monitor-Logs zeigen nun: `ADC: 850 | Threshold: 2000 | Beam: BROKEN`
Bei anliegendem Laser: `ADC: 4095 | Threshold: 2000 | Beam: PRESENT`

### WiFi Channel Synchronization (CRITICAL für Pairing)

**Problem:** Laser Unit kann Main Unit nicht finden wenn Channels unterschiedlich sind
**Ursache:** 
- Laser Unit startet standardmäßig auf ESP-NOW Channel 1
- Main Unit könnte bereits mit WLAN verbunden sein (z.B. Channel 6)
- ESP-NOW funktioniert nur auf dem gleichen WiFi-Channel
- Laser Unit kann keine Pairing Requests senden wenn auf falschem Channel

**Lösung: Multi-Channel Scanning auf Laser Unit**
Die Laser Unit muss alle WiFi-Channels durchscannen bis sie die Main Unit findet:

```c
// Laser Unit - Channel Scanning Implementation
static uint8_t current_scan_channel = 1;
static uint8_t scan_attempts_on_channel = 0;
static const uint8_t MAX_ATTEMPTS_PER_CHANNEL = 3;  // 3 Versuche pro Channel
static const uint8_t MAX_WIFI_CHANNEL = 13;          // Channels 1-13

static void pairing_timer_callback(void *arg)
{
    if (!is_paired) {
        ESP_LOGI(TAG, "Sending pairing request on channel %d (attempt %d/%d)...", 
                 current_scan_channel, scan_attempts_on_channel + 1, MAX_ATTEMPTS_PER_CHANNEL);
        
        espnow_broadcast_message(MSG_PAIRING_REQUEST, NULL, 0);
        scan_attempts_on_channel++;
        
        // Nach MAX_ATTEMPTS_PER_CHANNEL Versuchen zum nächsten Channel wechseln
        if (scan_attempts_on_channel >= MAX_ATTEMPTS_PER_CHANNEL) {
            scan_attempts_on_channel = 0;
            current_scan_channel++;
            
            // Zurück zu Channel 1 nach Channel 13
            if (current_scan_channel > MAX_WIFI_CHANNEL) {
                current_scan_channel = 1;
                ESP_LOGI(TAG, "Completed full channel scan, restarting from channel 1");
            }
            
            // Channel wechseln
            ESP_LOGI(TAG, "Switching to channel %d for pairing scan", current_scan_channel);
            esp_wifi_set_channel(current_scan_channel, WIFI_SECOND_CHAN_NONE);
        }
    }
}

// Bei erfolgreichem Pairing (MSG_PAIRING_RESPONSE):
case MSG_PAIRING_RESPONSE: {
    ESP_LOGI(TAG, "Pairing successful on channel %d!", current_scan_channel);
    is_paired = true;
    scan_attempts_on_channel = 0;  // Reset scan state
    // ... rest of pairing response handler
}
```

**Konfiguration:**
- Pairing Timer: 5 Sekunden Intervall
- 3 Versuche pro Channel (15 Sekunden pro Channel)
- Vollständiger Scan 1-13: ~195 Sekunden (3.25 Minuten) im worst case
- Best case: Sofortiges Pairing auf dem ersten Channel

**Alternative: WiFi AP Scan (schneller aber nur wenn AP aktiv):**
```c
// Optional: Vor Channel-Scanning WiFi AP scannen
esp_err_t find_main_unit_channel(uint8_t *channel)
{
    wifi_scan_config_t scan_config = {
        .ssid = (uint8_t*)CONFIG_WIFI_SSID,  // "LaserParcour"
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };
    
    esp_wifi_scan_start(&scan_config, true);
    
    uint16_t ap_count = 1;
    wifi_ap_record_t ap_info;
    
    if (esp_wifi_scan_get_ap_records(&ap_count, &ap_info) == ESP_OK && ap_count > 0) {
        *channel = ap_info.primary;
        ESP_LOGI(TAG, "Found Main Unit AP on channel %d", *channel);
        return ESP_OK;
    }
    
    return ESP_FAIL;
}
```

**Empfehlung:**
1. **Startup:** WiFi AP Scan nach Main Unit's SSID
2. **Wenn gefunden:** Direkt zu dem Channel wechseln und pairen
3. **Wenn nicht gefunden:** Multi-Channel Scan starten (1-13)

**Code-Änderungen (2025-01-08):**
- Laser Unit: Implementierung von Channel-Scanning in pairing_timer_callback()
- Laser Unit: WiFi AP Scan als Optimierung für schnelleres Pairing (TODO)
- Main Unit: wifi_apsta_init() für korrekte APSTA netif Initialisierung
- Main Unit: Dokumentation des Channel-Sync-Problems

**Implementiert in main.c (CONFIG_MODULE_ROLE_LASER):**
- Variablen: current_scan_channel, scan_attempts_on_channel, MAX_ATTEMPTS_PER_CHANNEL (3), MAX_WIFI_CHANNEL (13)
- pairing_timer_callback(): Automatischer Channel-Wechsel nach 3 erfolglosen Versuchen
- MSG_PAIRING_RESPONSE: Loggt erfolgreichen Channel, resettet Scan-State
- MSG_RESET: Resettet Channel auf CONFIG_ESPNOW_CHANNEL, startet Scan neu

**Performance:**
- 3 Versuche pro Channel à 5 Sekunden = 15 Sekunden pro Channel
- Best Case: Sofort auf Channel 1 (5-15 Sekunden)
- Typical Case: Channel 6 (75-90 Sekunden)
- Worst Case: Channel 13 (195 Sekunden = 3.25 Minuten)

### LED Control Logic (2025-01-08)

**Problem:** Status-LED wurde für Laser ON/OFF verwendet, aber sollte für Verbindungsstatus sein. Grün/Rot LEDs sollten sowohl für manuellen Betrieb als auch Game-Modus verwendet werden.

**Lösung: Dual-Modus LED System**

**Status-LED (CONFIG_LASER_STATUS_LED_PIN):**
- **Während Pairing:** Blinkt mit 500ms (led_blink_timer)
- **Nach erfolgreichem Pairing:** Dauerhaft an (Verbunden)
- **Nach MSG_RESET:** Aus (getrennt)
- **Unabhängig von Laser ON/OFF**

**Grün + Rot LEDs (CONFIG_SENSOR_LED_GREEN/RED_PIN):**

1. **Manueller Modus** (MSG_LASER_ON/OFF):
   - `MSG_LASER_ON`: Beide LEDs an (grün + rot = gelb/orange)
   - `MSG_LASER_OFF`: Beide LEDs aus
   - Nur aktiv wenn `!is_game_mode`

2. **Game-Modus** (MSG_GAME_START/STOP):
   - `MSG_GAME_START`: 
     - `is_game_mode = true`
     - Grün-LED an (initial Beam OK)
     - Rot-LED aus
   - **Während Game:** 
     - `beam_break_callback()`: Rot an, Grün aus
     - `beam_restore_callback()`: Grün an, Rot aus
   - `MSG_GAME_STOP`: 
     - `is_game_mode = false`
     - Beide LEDs aus

**Code-Änderungen:**
- `main/main.c`: Variable `is_game_mode` hinzugefügt
- `main/main.c`: MSG_GAME_START/STOP setzen `is_game_mode` Flag
- `main/main.c`: MSG_LASER_ON/OFF prüfen `!is_game_mode` vor LED-Steuerung
- `main/main.c`: MSG_PAIRING_RESPONSE schaltet Status-LED dauerhaft an
- `main/main.c`: beam_restore_callback() hinzugefügt für grüne LED
- `sensor_manager.h`: beam_restore_callback_t Typedef hinzugefügt
- `sensor_manager.c`: sensor_register_restore_callback() implementiert
- `sensor_manager.c`: restore_callback beim Beam-Restore aufgerufen

**Ergebnis:**
- Klare Trennung: Status-LED = Verbindung, Grün/Rot = Laser/Game-Status
- Im Game-Modus: Live-Feedback über Beam-Status
- Im manuellen Modus: Beide LEDs = visuelles Feedback für Laser ON

### WiFi Auto-Connect Race Condition (2025-01-08)

**Problem:** ESP32 crasht beim Start mit `ESP_ERR_WIFI_CONN` in `wifi_connect_sta()`

**Symptome:**
```
I (762) wifi:connected with ninIOT, aid = 32, channel 6, BW20, bssid = 78:45:58:27:21:e4
ESP_ERROR_CHECK failed: esp_err_t 0x3007 (ESP_ERR_WIFI_CONN) at 0x420118bc
file: "./components/wifi_ap_manager/wifi_ap_manager.c" line 475
func: wifi_connect_sta
expression: esp_wifi_connect()
abort() was called at PC 0x40388d37 on core 0
```

**Ursache:**
- WiFi hatte gespeicherte Credentials im NVS (z.B. "ninIOT")
- Bei `esp_wifi_start()` verbindet sich WiFi **automatisch** mit gespeichertem Netzwerk
- Code versuchte dann `esp_wifi_connect()` aufzurufen → **ESP_ERR_WIFI_CONN** (already connecting)
- `ESP_ERROR_CHECK()` führte zu Abort

**Lösung: Intelligente Verbindungsprüfung**

Vor `esp_wifi_connect()` prüfen ob WiFi bereits verbunden/connecting ist:

```c
// wifi_ap_manager.c - wifi_connect_sta()
if (!is_initialized) {
    ESP_ERROR_CHECK(esp_wifi_start());
    is_initialized = true;
} else {
    // Check if already connected or connecting
    wifi_ap_record_t ap_info;
    esp_err_t check = esp_wifi_sta_get_ap_info(&ap_info);
    
    if (check == ESP_OK) {
        // Already connected, no need to call esp_wifi_connect()
        ESP_LOGI(TAG, "WiFi already connected to: %s", ap_info.ssid);
    } else if (check == ESP_ERR_WIFI_NOT_CONNECT) {
        // Not connected, safe to call connect
        esp_err_t connect_ret = esp_wifi_connect();
        if (connect_ret != ESP_OK && connect_ret != ESP_ERR_WIFI_CONN) {
            ESP_ERROR_CHECK(connect_ret);
        } else if (connect_ret == ESP_ERR_WIFI_CONN) {
            ESP_LOGI(TAG, "WiFi already connecting, waiting for result...");
        }
    }
}
```

**Code-Änderungen:**
- `wifi_ap_manager.c`: wifi_connect_sta() prüft vor connect ob bereits verbunden
- `wifi_ap_manager.c`: ESP_ERR_WIFI_CONN wird als gültig behandelt (already connecting)
- `wifi_ap_manager.c`: Verhindert doppelten esp_wifi_connect() Aufruf

**Verhalten:**
1. **WiFi nicht verbunden**: `esp_wifi_connect()` wird aufgerufen
2. **WiFi bereits verbunden**: Überspringt connect, nutzt bestehende Verbindung
3. **WiFi connecting**: Akzeptiert ESP_ERR_WIFI_CONN, wartet auf Event
4. **Kein Crash mehr** bei automatischer WiFi-Verbindung

**Ergebnis:**
- Main Unit startet erfolgreich auch mit gespeicherten WiFi-Credentials
- Automatische Verbindung zu bekannten Netzwerken funktioniert
- Fallback zu AP-Modus bei Verbindungsfehlern bleibt erhalten

### Online Status Flackern Fix (2025-01-08)

**Problem:** Online-Status im Webinterface flackert zwischen Online/Offline

**Ursache:**
- Heartbeats werden alle 3 Sekunden gesendet
- Online-Timeout war nur 5 Sekunden
- Bei minimaler Verzögerung wurde Unit sofort als offline markiert
- Keine Mechanik zum Entfernen "toter" Units aus der Liste

**Lösung: Stabiles Timeout-System mit Auto-Cleanup**

**Zwei-Stufen-Timeout-System:**

1. **Online-Timeout: 15 Sekunden** (5x Heartbeat-Intervall)
   - Unit wird als "Offline" markiert wenn 15 Sekunden kein Heartbeat
   - Bleibt in der Liste und kann zurückkommen
   - Verhindert Flackern bei kurzen Netzwerk-Verzögerungen

2. **Removal-Timeout: 60 Sekunden** (20x Heartbeat-Intervall)
   - Unit wird komplett aus der Liste entfernt nach 60 Sekunden
   - ESP-NOW Peer wird ebenfalls entfernt
   - Automatisches Cleanup von toten/abgeschalteten Units

**Implementation:**
```c
// game_logic.c - game_get_laser_units()
uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
size_t active_count = 0;

for (size_t i = 0; i < laser_unit_count; i++) {
    uint32_t time_since_last_seen = now - laser_units[i].last_seen;
    
    // Remove units offline for >60 seconds
    if (time_since_last_seen > 60000) {
        ESP_LOGI(TAG, "Removing inactive laser unit %d", laser_units[i].module_id);
        espnow_remove_peer(laser_units[i].mac_address);
        continue;  // Skip (don't copy to active list)
    }
    
    // Mark offline if >15 seconds since last heartbeat
    if (time_since_last_seen > 15000) {
        laser_units[i].is_online = false;
        snprintf(laser_units[i].status, "Offline");
    } else {
        laser_units[i].is_online = true;
        snprintf(laser_units[i].status, "Online");
    }
    
    // Compact array (remove gaps)
    if (active_count != i) {
        laser_units[active_count] = laser_units[i];
    }
    active_count++;
}

laser_unit_count = active_count;  // Update to reflect removals
```

**Code-Änderungen:**
- `game_logic.c`: Online-Timeout von 5s auf 15s erhöht (stabiler)
- `game_logic.c`: Removal-Timeout von 60s hinzugefügt (Auto-Cleanup)
- `game_logic.c`: espnow_remove_peer() wird beim Entfernen aufgerufen
- `game_logic.c`: Array-Kompaktierung um Lücken zu entfernen

**Verhalten:**
1. **0-15 Sekunden**: Unit ist "Online" (grünes Icon)
2. **15-60 Sekunden**: Unit ist "Offline" (rotes Icon, aber noch in Liste)
3. **>60 Sekunden**: Unit wird komplett entfernt aus Liste und Peers
4. **Wiederverbindung**: Unit kann jederzeit durch neuen Heartbeat zurückkommen

**Vorteile:**
- ✅ Kein Flackern mehr bei minimalen Netzwerk-Verzögerungen
- ✅ Automatisches Cleanup von abgeschalteten/defekten Units
- ✅ ESP-NOW Peer-Liste bleibt sauber
- ✅ Toleranz für 5 verpasste Heartbeats vor "Offline"
- ✅ Klare visuelle Trennung zwischen temporär offline und entfernt

**Ergebnis:**
- Stabiler Online-Status ohne Flackern
- Webinterface zeigt nur aktive/erreichbare Units
- Automatische Bereinigung nach 1 Minute Inaktivität

### Heartbeat ESP-NOW Interface Fix (2025-01-08)

**Problem:** Heartbeats wurden gesendet aber nicht empfangen, Units gingen nach 15s offline

**Ursache:**
- Laser Units sendeten Heartbeats als **Broadcast** (`espnow_broadcast_message`)
- Main Unit im **APSTA-Modus** (AP + STA gleichzeitig)
- Laser Units im **STA-Modus**
- ESP-NOW Broadcasts über WIFI_IF_STA werden nicht zuverlässig von APSTA-Geräten empfangen
- Broadcast-Peer war nur für WIFI_IF_STA registriert, nicht für beide Interfaces

**Lösung: Unicast Heartbeats an bekannte Main Unit MAC**

1. **Globale Variable auf Laser Unit:**
   ```c
   static uint8_t main_unit_mac[6] = {0};  // MAC address of paired main unit
   ```

2. **MAC-Adresse speichern bei Pairing:**
   ```c
   case MSG_PAIRING_RESPONSE:
       memcpy(main_unit_mac, sender_mac, 6);
       ESP_LOGI(TAG, "Main unit MAC: %02X:%02X:%02X:%02X:%02X:%02X", ...);
       espnow_add_peer(main_unit_mac, 0, 0);  // Add as peer for unicast
   ```

3. **Heartbeat als Unicast senden:**
   ```c
   static void heartbeat_timer_callback(void *arg)
   {
       if (is_paired) {
           esp_err_t ret = espnow_send_message(main_unit_mac, MSG_HEARTBEAT, NULL, 0);
           ESP_LOGI(TAG, "Heartbeat sent to main unit: %s", esp_err_to_name(ret));
       }
   }
   ```

**Code-Änderungen:**
- `main/main.c` (Laser): Variable `main_unit_mac[6]` hinzugefügt
- `main/main.c` (Laser): `heartbeat_timer_callback()` verwendet `espnow_send_message()` statt `espnow_broadcast_message()`
- `main/main.c` (Laser): MSG_PAIRING_RESPONSE speichert MAC und fügt Main Unit als Peer hinzu
- `main/main.c` (Main): MSG_HEARTBEAT Handler aktualisiert `last_seen` Timestamp

**Ergebnis:**
- ✅ Heartbeats kommen zuverlässig an (Unicast ist robust)
- ✅ Units bleiben online (kein Flackern mehr)
- ✅ Interface-Kompatibilität (APSTA ↔ STA funktioniert)
- ✅ Direkte Punkt-zu-Punkt-Kommunikation statt Broadcast
- ✅ Bessere Netzwerk-Performance (weniger Broadcast-Traffic)

### OLED Display Integration (2025-01-08)

**Implementation:** Vollständige Display-Integration für Main Unit

**Features:**
1. **Display Update Task:**
   - Eigener FreeRTOS Task mit 100ms Update-Intervall
   - Automatische Screen-Auswahl basierend auf Game-State
   - 4KB Stack-Größe, Priorität 5

2. **State-basierte Screens:**
   - **IDLE**: Welcome-Screen mit Connected Units Counter
   - **COUNTDOWN**: Countdown vor Spielstart
   - **RUNNING/PENALTY/PAUSED**: Live Game-Status (Zeit, Breaks, Score)
   - **COMPLETE**: Endergebnis-Anzeige

3. **Display-Inhalte:**
   - Welcome Screen: "Laser Parcour", "Ready to Start", "Units: X", "Start via Web"
   - Game Running: Echtzeit-Updates von Zeit, Beam Breaks, Score
   - Game Complete: Final Time, Beam Breaks, Final Score

**Code-Änderungen:**
- `main/main.c`: display_update_task() implementiert (100ms Update-Intervall)
- `main/main.c`: Task-Start nach game_logic_init()
- `main/main.c`: display_update_task_handle Variable hinzugefügt

**Display-Update-Flow:**
```c
while (1) {
    game_state = game_get_state();
    game_get_player_data(&player_data);
    game_get_laser_units(units, &count);
    
    switch (game_state) {
        case IDLE: Show welcome + unit count
        case COUNTDOWN: Show countdown
        case RUNNING: Show time/breaks/score
        case COMPLETE: Show final results
    }
    
    vTaskDelayUntil(100ms);
}
```

**Konfiguration:**
- Display aktiviert wenn CONFIG_ENABLE_DISPLAY=y
- Pins: CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN (z.B. 19/18)
- Frequenz: CONFIG_I2C_FREQUENCY (100kHz oder 400kHz)
- Display-Typ: CONFIG_OLED_SSD1306 oder CONFIG_OLED_SH1106

**Ergebnis:**
- ✅ Automatische Display-Updates alle 100ms
- ✅ Echtzeit-Anzeige von Spielstatus
- ✅ Connected Units werden gezählt und angezeigt
- ✅ State-abhängige Screen-Verwaltung
- ✅ Keine manuelle Display-Aktualisierung nötig

### SSD1306 OLED Driver Implementation (2025-01-08)

**Architektur:** Display-Manager separiert von spezifischen Treibern

**Struktur:**
```
display_manager/
├── display_manager.c/h      # Abstract interface
├── ssd1306.c/h               # SSD1306 driver (128x64)
└── sh1106.c/h                # SH1106 driver (TODO)
```

**SSD1306 Features:**
1. **Hardware:**
   - 128x64 Pixel OLED
   - I2C Interface (0x3C)
   - 1024-byte Framebuffer (128 * 8 pages)
   - Charge Pump für 3.3V Betrieb

2. **Rendering:**
   - 5x7 ASCII Font (Zeichen 32-127)
   - 8 Pages (je 8 pixel hoch)
   - 21 Zeichen pro Zeile
   - Large Digits (3x skaliert) für Countdown

3. **API-Funktionen:**
   - `ssd1306_init()` - I2C & Display initialisieren
   - `ssd1306_clear()` - Framebuffer leeren
   - `ssd1306_update()` - Framebuffer zum Display senden
   - `ssd1306_draw_string()` - Text zeichnen
   - `ssd1306_draw_char()` - Einzelnes Zeichen
   - `ssd1306_draw_large_digit()` - Große Ziffer (3x)
   - `ssd1306_draw_hline()` - Horizontale Linie
   - `ssd1306_set_contrast()` - Helligkeit (0-255)
   - `ssd1306_display_power()` - Display ein/aus

**Display Manager Delegation:**
Der display_manager.c delegiert basierend auf CONFIG:
```c
#ifdef CONFIG_ENABLE_DISPLAY
  #ifdef CONFIG_OLED_SSD1306
    ssd1306_init(...);
  #elif defined(CONFIG_OLED_SH1106)
    sh1106_init(...);
  #endif
#else
  // Stub implementations when CONFIG_ENABLE_DISPLAY not set
  // Returns ESP_OK without hardware access
  // Allows Laser Units to compile without display support
#endif
```

**Stub-Implementation für Laser Units:**
Wenn CONFIG_ENABLE_DISPLAY nicht gesetzt ist (z.B. auf Laser Units):
```c
#else // CONFIG_ENABLE_DISPLAY not defined
esp_err_t display_manager_init(...) { return ESP_OK; }
esp_err_t display_clear(void) { return ESP_OK; }
esp_err_t display_update(void) { return ESP_OK; }
// ... alle anderen Funktionen als leere Stubs
#endif
```

**Code-Änderungen:**
- `display_manager/ssd1306.c/h`: Neuer SSD1306-Treiber
- `display_manager/display_manager.c`: Abstrakte Schicht mit CONFIG_ENABLE_DISPLAY ifdef
- `display_manager/display_manager.c`: Stub-Implementierungen für Units ohne Display
- `display_manager/CMakeLists.txt`: ssd1306.c zu SRCS hinzugefügt

**Vorteile:**
- ✅ Modulare Treiber-Architektur
- ✅ Einfach erweiterbar (SH1106, SSD1327, etc.)
- ✅ Konfigurierbar via menuconfig
- ✅ Saubere Trennung: Interface ↔ Hardware
- ✅ Laser Units kompilieren ohne Display-Code (Stubs)
- ✅ Kein Dead-Code auf Laser Units

---

## 🏁 Finish-Button-Device - IMPLEMENTIERT ✅

**Zweck:** Separates ESP32-Modul mit Button zum erfolgreichen Abschließen des Spiels

**Modul-Typ: FINISH**
- Dritte Modul-Rolle neben CONTROL und LASER
- ESP32-C3 Modul mit Button und LEDs
- Sendet MSG_FINISH_PRESSED via ESP-NOW an Main Unit

**Hardware-Komponenten:**
- ESP32-C3 Mikrocontroller
- Push-Button (Active Low, GPIO 5 default)
- Status-LED (GPIO 21 default) - Blinkt beim Pairing, solid wenn verbunden
- Button-Beleuchtungs-LED (GPIO 20 default) - Leuchtet permanent, geht aus beim Drücken
- Stromversorgung (5V USB oder Batterie)

**Funktionsweise:**
1. **Pairing:** Automatisches Pairing mit Main Unit beim Start (wie Laser Units)
   - Sendet MSG_PAIRING_REQUEST mit role=2 (Finish Button)
   - Scannt Channels 1, 6, 11 zyklisch
   - Status-LED blinkt bis erfolgreiches Pairing
   
2. **Button-Press:** Wenn Button gedrückt wird (Active Low)
   - Debouncing (50ms)
   - Button-Beleuchtungs-LED geht AUS
   - MSG_FINISH_PRESSED an Main Unit senden
   - Nach Loslassen: LED geht wieder AN
   
3. **Main Unit Reaktion:** 
   - Empfängt MSG_FINISH_PRESSED
   - Ruft game_finish() auf
   - Setzt completion = COMPLETION_SOLVED
   - Display zeigt "GAME COMPLETE!" (Erfolg)
   - Sendet MSG_GAME_STOP an alle Laser Units
   
4. **Heartbeat:** Sendet alle 3 Sekunden MSG_HEARTBEAT nach erfolgreichem Pairing

**Implementation:**
```c
// main/main.c - Finish Button Module
#ifdef IS_FINISH_MODULE

// GPIO-Konfiguration (menuconfig):
CONFIG_FINISH_BUTTON_PIN = 5          // Button GPIO (Active Low)
CONFIG_FINISH_STATUS_LED_PIN = 21     // Status-LED (Pairing/Connection)
CONFIG_FINISH_BUTTON_LED_PIN = 20     // Button-Beleuchtungs-LED

static void button_isr_handler(void *arg)
{
    button_pressed = true;  // Flag setzen
}

static void button_handler_task(void *arg)
{
    while (1) {
        if (button_pressed) {
            button_pressed = false;
            vTaskDelay(pdMS_TO_TICKS(50));  // Debounce
            
            if (gpio_get_level(CONFIG_FINISH_BUTTON_PIN) == 0) {  // Still pressed
                gpio_set_level(CONFIG_FINISH_BUTTON_LED_PIN, 0);  // Turn OFF illumination
                
                if (is_paired) {
                    espnow_send_message(main_unit_mac, MSG_FINISH_PRESSED, NULL, 0);
                }
                
                // Wait for release
                while (gpio_get_level(CONFIG_FINISH_BUTTON_PIN) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                
                gpio_set_level(CONFIG_FINISH_BUTTON_LED_PIN, 1);  // Turn ON illumination
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void pairing_timer_callback(void *arg)
{
    if (!is_paired) {
        // Multi-Channel Scanning (1, 6, 11)
        uint8_t role = 2;  // 2 = Finish Button
        espnow_broadcast_message(MSG_PAIRING_REQUEST, &role, sizeof(role));
    }
}
#endif
```

**Main Unit Handler:**
```c
// main/main.c - espnow_recv_callback_main()
case MSG_PAIRING_REQUEST:
    uint8_t peer_role = (message->data_len >= 1) ? message->data[0] : 1;
    const char *role_name = (peer_role == 2) ? "Finish Button" : "Laser Unit";
    
    espnow_add_peer(sender_mac, message->module_id, peer_role);
    espnow_send_message(sender_mac, MSG_PAIRING_RESPONSE, NULL, 0);
    ESP_LOGI(TAG, "%s %d added as peer", role_name, message->module_id);
    break;

case MSG_FINISH_PRESSED:
    ESP_LOGI(TAG, "Finish button pressed on module %d - completing game!", message->module_id);
    game_finish();  // Successful completion (COMPLETION_SOLVED)
    break;
```

**Game Logic:**
```c
// components/game_logic/game_logic.c
esp_err_t game_finish(void)
{
    if (current_state != GAME_STATE_RUNNING && current_state != GAME_STATE_PENALTY) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Set completion status to SOLVED
    current_player.completion = COMPLETION_SOLVED;
    current_state = GAME_STATE_COMPLETE;
    
    // Stop game timer
    esp_timer_stop(game_timer);
    
    // Broadcast MSG_GAME_STOP to all units
    espnow_broadcast_message(MSG_GAME_STOP, NULL, 0);
    
    // Update statistics
    game_stats.total_games++;
    game_stats.games_completed++;  // Only count SOLVED as completed
    
    ESP_LOGI(TAG, "Game finished successfully via finish button!");
    return ESP_OK;
}
```

**Display-Unterscheidung:**
```c
// components/display_manager/display_manager.c
void display_game_results(uint32_t elapsed_time, uint16_t beam_breaks, completion_status_t completion)
{
    ssd1306_clear();
    
    if (completion == COMPLETION_SOLVED) {
        ssd1306_draw_string(20, 0, "GAME COMPLETE!");  // Erfolg
    } else {
        ssd1306_draw_string(15, 0, "GAME CANCELED!");  // Abbruch
    }
    
    ssd1306_draw_string(25, 3, "Total Time:");
    // Zeit-Anzeige in Line 5
    // Breaks-Anzeige in Line 7
    ssd1306_update();
}
```

**Web Interface - Spezielle Anzeige:**
- Finish Button Units werden mit 🏁-Icon angezeigt
- Grüne Border-Linie (border-left: 4px solid #4CAF50)
- Keine Laser ON/OFF Controls (nur Reset Button)
- Deutlich als "Finish Button" beschriftet statt "Laser Unit"

```javascript
// components/web_server/index.html
let isFinish = u.role === 2;
let unitType = isFinish ? '🏁 Finish Button' : 'Laser Unit';
let unitStyle = isFinish ? 'style="border-left: 4px solid #4CAF50;"' : '';
```

**Konfiguration (Kconfig.projbuild):**
```
choice MODULE_ROLE
    prompt "Module Role"
    default MODULE_ROLE_CONTROL
    config MODULE_ROLE_CONTROL
        bool "CONTROL (Main Unit)"
    config MODULE_ROLE_LASER
        bool "LASER (Laser Unit)"
    config MODULE_ROLE_FINISH
        bool "FINISH (Finish Button)"
endchoice

config FINISH_BUTTON_PIN
    int "Finish Button GPIO Pin"
    depends on MODULE_ROLE_FINISH
    default 0
    help
        GPIO pin for finish button (active low with pull-up)
```

**Vorteile:**
- ✅ Klare Unterscheidung: Erfolgreich vs. Abgebrochen
- ✅ Physikalisches Feedback für Spieler (Button drücken = Ziel erreicht)
- ✅ Unabhängiges Modul (kein Verkabelung zur Main Unit nötig)
- ✅ ESP-NOW Kommunikation (wireless)
- ✅ Einfach zu platzieren am Ziel des Parcours

---

## 🐛 Debugging

### Log-Levels

```c
ESP_LOGE(TAG, "Critical error");    // Error
ESP_LOGW(TAG, "Warning");           // Warning
ESP_LOGI(TAG, "Information");       // Info (Standard)
ESP_LOGD(TAG, "Debug info");        // Debug
ESP_LOGV(TAG, "Verbose");           // Verbose
```

### Wichtige Log-Tags

- `LASER_PARCOUR` - Hauptprogramm
- `DISPLAY_MGR` - Display Manager
- `GAME_LOGIC` - Game Logic
- `ESPNOW_MGR` - ESP-NOW Manager
- `LASER_CTRL` - Laser Control
- `SENSOR_MGR` - Sensor Manager

### Monitor-Befehle

```bash
# Mit Filter
idf.py monitor --print-filter="LASER_PARCOUR:I ESPNOW_MGR:D"

# Reset
Ctrl+T, Ctrl+R

# Menu
Ctrl+T, Ctrl+H

# Exit
Ctrl+]
```

---

## 🔐 Sicherheit

### Laser Safety
- ⚠️ **Class 2 Laser Only** (<5mW, 650nm)
- 10 Minuten Auto-Timeout
- Manueller Ein/Aus-Schalter empfohlen
- Niemals direkt in den Beam schauen

### Electrical Safety
- 5V Stromversorgung
- Strombegrenzung für Laser-Dioden
- ESD-Schutz für ESP32 empfohlen

---

## 📊 Performance

### ESP-NOW Latency
- Typisch: 5-10ms
- Max: 50ms

### ADC Sampling Rate
- Konfigurierbar
- Typisch: 100 Samples/Sekunde

### Display Refresh
- 60 FPS möglich
- Typisch: 20-30 FPS für Spiel-Status

---

## 🚀 Zukünftige Erweiterungen (TODO)

### Main Unit
- [x] Button-Handler Component (✅ Implementiert)
- [x] Buzzer/Audio Component (✅ Implementiert)
- [x] WiFi AP Implementation (✅ Implementiert)
- [x] Web Server Component (✅ Implementiert)
- [x] SD Card Support (✅ Implementiert)
- [ ] OTA Update System

### Laser Unit
- [x] LED Component (Status-LEDs implementiert)
- [x] Erweiterte Kalibrierung (✅ Sensor-Kalibrierung)
- [x] Multi-Channel Pairing (✅ Implementiert)
- [x] Heartbeat System (✅ Implementiert)

### Beide
- [x] Multi-Module Support (✅ CONTROL, LASER, FINISH)
- [ ] Persistente Statistiken (NVS)
- [ ] Multi-Player Support
- [ ] Game Modes erweitern
- [x] Web-Interface für Live-Monitoring (✅ Implementiert)

---

## 📅 Aktuelle Änderungen (Januar 2026)

### 2026-01-09: Code Modularisierung & Funktionalitäts-Wiederherstellung

**Problem:** main.c war 1578 Zeilen lang und schwer zu warten

**Lösung:** Aufteilung in separate Module mit conditional compilation:

**Neue Struktur:**
```
main/
├── main.c (130 lines)              # Core init + module delegation
├── main.c.backup (1578 lines)      # ORIGINAL WORKING CODE - BACKUP!
├── module_control.c (643 lines)    # Control Unit implementation
├── module_control.h                # Control Unit header
├── module_laser.c (507 lines)      # Laser Unit implementation
├── module_laser.h                  # Laser Unit header
├── module_finish.c (352 lines)     # Finish Button implementation
├── module_finish.h                 # Finish Button header
├── logging_config.c                # Centralized logging
└── logging_config.h                # Logging declarations
```

**Build-System Änderungen:**
- `main/CMakeLists.txt` kompiliert nur das passende Modul:
  ```cmake
  if(CONFIG_MODULE_ROLE_CONTROL)
      list(APPEND MODULE_SRCS "module_control.c")
  elseif(CONFIG_MODULE_ROLE_LASER)
      list(APPEND MODULE_SRCS "module_laser.c")
  elseif(CONFIG_MODULE_ROLE_FINISH)
      list(APPEND MODULE_SRCS "module_finish.c")
  endif()
  ```

**Header Guards:**
- Header haben `#ifdef CONFIG_MODULE_ROLE_XXX` Guards INNERHALB
- NICHT wrappen des gesamten Headers (verhindert Compile-Fehler)

**Funktionalitäts-Wiederherstellung:**

1. **Control Module (module_control.c):**
   - ✅ `button_event_callback`: Laser Toggle nutzt `game_control_laser()` statt Broadcast
   - ✅ `game_control_callback`: Fehlerbehandlung wie im Original
   - ✅ `espnow_recv_callback_main`: MSG_FINISH_PRESSED ruft `game_finish()` auf
   - ✅ `display_update_task`: Update-Interval 100ms, zeigt "Units: X" im IDLE-Screen
   - ✅ `GAME_STATE_COUNTDOWN`: Case hinzugefügt mit `display_countdown()`
   - ✅ Display init mit `CONFIG_I2C_FREQUENCY`
   - ✅ Button config mit `CONFIG_DEBOUNCE_TIME`
   - ✅ **Heartbeat-System**: Main Unit sendet alle 5 Sekunden MSG_HEARTBEAT Broadcasts
   - ✅ `heartbeat_timer_callback`: Hält Laser Unit Safety-Timer am Leben (bereits implementiert)

2. **Laser Module (module_laser.c):**
   - ✅ `espnow_recv_callback_laser`: War bereits korrekt
   - ✅ Initial Pairing Request: Sendet `NULL` statt `&role` (wie im Original)
   - ✅ `MSG_HEARTBEAT` Handler: Aktualisiert `last_main_unit_heartbeat` bei Main Unit Heartbeats
   - ✅ Safety Timer: 10 Sekunden (funktioniert mit Main Unit Heartbeats alle 5s)

3. **Finish Module (module_finish.c):**
   - ✅ `button_handler_task`: War bereits korrekt
   - ✅ `button_isr_handler`: War bereits korrekt
   - ✅ Finish Message: Sendet **Unicast** an `main_unit_mac` (nicht Broadcast)

**Wichtige Learnings:**
- ⚠️ IMMER gegen `main.c.backup` vergleichen bei Funktionsproblemen!
- ⚠️ EXAKTE Funktionalität kopieren, nicht "verbessern"!
- ⚠️ API-Strukturen NICHT annehmen - im Code nachschauen!
- ⚠️ Display braucht explizite Updates im IDLE-State
- ⚠️ Update-Interval ist kritisch für responsive UI (100ms nicht 1000ms)

**Compile-Fehler behoben:**
- Duplizierte Button-Initialisierungs-Sektion entfernt
- `button_configs` → `buttons` (korrekte Variable)
- Redundante `#else #endif` Blöcke entfernt

**Web-Interface Sicherheit (2026-01-09):**
- Problem: Manuelle Laser-Steuerung war während eines laufenden Spiels möglich
- Lösung: `units_control_handler` prüft jetzt den Game-State
- Blockiert: `laser_on` und `laser_off` während RUNNING, COUNTDOWN, PENALTY, PAUSED
- Erlaubt: `reset` ist immer möglich (auch während Spiel)
- Error-Response: `{"error":"Cannot control laser during active game"}`
- File: `components/web_server/web_server.c` Lines 495-509

**Web-Interface Status-Anzeige (2026-01-09):**
- Problem: Laser-Status zeigte immer OFF während Spiel läuft (obwohl Laser AN sind)
- Lösung 1: `/api/units` gibt jetzt `game_state` und `game_active` zurück
- Lösung 2: Frontend zeigt Laser als ON während Spiel läuft (für role=1 Units)
- Lösung 3: ON/OFF Buttons werden disabled und zeigen "🔒 LOCKED" während Spiel
- Lösung 4: `control()` Funktion ruft `updateUnits()` nach Game Start/Stop auf (sofortiges Update)
- CSS: `.btn:disabled` Style für deaktivierte Buttons (grau, not-allowed cursor)
- File: `components/web_server/web_server.c` Lines 452-459
- File: `components/web_server/index.html` Lines 237-269 (updateUnits function)
- File: `components/web_server/index.html` Lines 68-75 (disabled button style)
- File: `components/web_server/index.html` Lines 193-197 (control function with updateUnits)

---

## 🔧 Best Practices

### Code-Änderungen

**🔴 NACH JEDEM SCHRITT:**
1. ✅ `copilot-instructions.md` aktualisieren
2. ✅ Bei Funktionalitätsänderungen: Gegen Backup vergleichen
3. ✅ Build testen (`idf.py build`)
4. ✅ Auf allen betroffenen Devices testen

### Debugging

**Bei Funktionsproblemen:**
1. 🔍 Logs überprüfen (`idf.py monitor`)
2. 🔍 Gegen `main.c.backup` vergleichen
3. 🔍 API-Dokumentation in Component-Headers checken
4. 🔍 EXAKTE Original-Implementierung wiederherstellen

### Refactoring

**Beim Umstrukturieren von Code:**
1. ⚠️ BACKUP erstellen BEVOR Änderungen gemacht werden
2. ⚠️ Funktionalität 1:1 kopieren, nicht "verbessern"
3. ⚠️ Alle Event-Handler und Callbacks exakt übernehmen
4. ⚠️ Variablennamen, Typen, Parameter identisch lassen
5. ⚠️ Testen auf ALLEN Device-Typen (CONTROL, LASER, FINISH)

---

## 📚 Referenzen

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32c3/)
- [ESP-NOW Protocol](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32c3/api-reference/network/esp_now.html)
- [SSD1306 OLED](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
- [LEDC PWM](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32c3/api-reference/peripherals/ledc.html)

---

**Version:** 3.0.0  
**Letztes Update:** 9. Januar 2026  
**Autor:** @ninharp


## 📝 Coding-Konventionen

### Naming
- **Funktionen:** `module_action()` (snake_case)
- **Typen:** `module_type_t` (snake_case + _t Suffix)
- **Konstanten:** `MODULE_CONSTANT` (UPPER_CASE)
- **Variablen:** `variable_name` (snake_case)
- **Config:** `CONFIG_OPTION_NAME` (UPPER_CASE)

### Header Guards
```c
#ifndef MODULE_NAME_H
#define MODULE_NAME_H
// ...
#endif // MODULE_NAME_H
```

### Fehlerbehandlung
```c
esp_err_t ret = function_call();
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error: %s", esp_err_to_name(ret));
    return ret;
}
// oder
ESP_ERROR_CHECK(function_call());  // Abort on error
```

### Komponenten-Template
```c
// header.h
#ifndef COMPONENT_H
#define COMPONENT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t component_init(void);
// ... more functions

#ifdef __cplusplus
}
#endif

#endif // COMPONENT_H
```

---

## 🔗 Wichtige Links

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [ESP-NOW Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)

---

## 👤 Autor

**ninharp**  
Version: 1.0  
Datum: Januar 2025  
ESP-IDF: 5.4.2

---

## 📜 Lizenz

Siehe LICENSE Datei im Projekt-Root.

---

**Hinweis für GitHub Copilot:**
Dieses Dokument beschreibt die vollständige Architektur eines ESP32-basierten Laser-Parcours-Systems. Bei Code-Änderungen bitte:
1. Die modulare Struktur beibehalten
2. Naming-Konventionen befolgen
3. Error-Handling mit ESP_ERROR_CHECK() oder manueller Prüfung
4. Log-Ausgaben mit passenden TAG und Level
5. Komponenten-Abhängigkeiten in CMakeLists.txt aktualisieren
6. Kconfig-Optionen für neue Features hinzufügen
7. **WICHTIG: Diese copilot-instructions.md Datei IMMER aktuell halten bei Änderungen an Code, Architektur oder Features!**
