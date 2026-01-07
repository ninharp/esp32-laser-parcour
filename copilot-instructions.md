# ESP32 Laser Obstacle Course - Project Documentation

## 📋 Projektübersicht

Ein modulares, ESP32-C3 basiertes Laser-Hindernisparcours-Spielsystem mit drahtloser Steuerung, Echtzeit-Überwachung und interaktivem Gaming-Erlebnis.

**Technologie-Stack:**
- ESP-IDF 5.4.2
- FreeRTOS
- ESP-NOW (Wireless Communication)
- I2C (OLED Display)
- ADC (Sensor Detection)
- LEDC PWM (Laser Control)

---

## 🏗️ Projektstruktur

```
esp32-laser-parcour/
├── CMakeLists.txt                    # Haupt-Build-Konfiguration
├── partitions.csv                    # Flash-Partitionstabelle
├── sdkconfig.defaults                # Standard SDK-Konfiguration
├── README.md                         # Projekt-Dokumentation
├── copilot-instructions.md           # Diese Datei
│
├── main/                             # Haupt-Anwendung
│   ├── main.c                        # Hauptprogramm mit app_main()
│   ├── Kconfig.projbuild             # Menuconfig-Optionen
│   └── CMakeLists.txt                # Build-Konfiguration für main
│
├── components/                       # Wiederverwendbare Komponenten
│   ├── display_manager/              # OLED Display Steuerung
│   ├── game_logic/                   # Spiellogik & Scoring
│   ├── espnow_manager/               # ESP-NOW Kommunikation
│   ├── laser_control/                # Laser PWM Steuerung
│   └── sensor_manager/               # ADC Sensor für Beam Detection
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
**Hauptfunktionen:**
```c
esp_err_t display_manager_init(gpio_num_t sda, gpio_num_t scl, uint32_t freq)
esp_err_t display_set_screen(display_screen_t screen)
esp_err_t display_game_status(uint32_t time, uint16_t breaks, int32_t score)
esp_err_t display_countdown(uint8_t seconds)
esp_err_t display_update(void)
```

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
esp_err_t game_start(game_mode_t mode, const char *player_name)
esp_err_t game_stop(void)
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
- GAME_STATE_COUNTDOWN - Countdown läuft
- GAME_STATE_RUNNING - Spiel aktiv
- GAME_STATE_PENALTY - Penalty Mode
- GAME_STATE_PAUSED - Pausiert
- GAME_STATE_COMPLETE - Beendet
- GAME_STATE_ERROR - Fehlerzustand

**Game Modes:**
- GAME_MODE_SINGLE_SPEEDRUN - Einzelspieler Speedrun
- GAME_MODE_MULTIPLAYER - Mehrspieler
- GAME_MODE_TRAINING - Training (keine Penalties)
- GAME_MODE_CUSTOM - Benutzerdefiniert

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
- CONFIG_BUTTON1_PIN bis CONFIG_BUTTON4_PIN
- CONFIG_DEBOUNCE_TIME (default: 50ms)
- CONFIG_ENABLE_BUTTONS (optional feature flag)

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
- Konfigurierbarer Threshold
- Debouncing (Anti-Flackern)
- Callback-basierte Events
- Automatische Kalibrierung

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

5. **LED Feedback:**
   - Status-LED: Pairing-Status / Laser aktiv
   - Grün-LED: Beam OK
   - Rot-LED: Beam unterbrochen

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
- [ ] Button-Handler Component
- [ ] Buzzer/Audio Component
- [ ] WiFi AP Implementation
- [ ] Web Server Component
- [ ] OTA Update System

### Laser Unit
- [ ] LED Component (Status-LEDs in eigene Komponente)
- [ ] Erweiterte Kalibrierung

### Beide
- [ ] Persistente Statistiken (NVS)
- [ ] Multi-Player Support
- [ ] Game Modes erweitern
- [ ] Web-Interface für Live-Monitoring

---

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
