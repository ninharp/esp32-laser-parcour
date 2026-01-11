# ESP32 Laser Obstacle Course Game System

A modular, ESP32-C3 based laser obstacle course game system featuring wireless control, real-time monitoring, and an interactive gaming experience. Perfect for events, arcades, or DIY gaming setups.

## 🎯 Overview

This project implements a distributed laser obstacle course system where players must navigate through laser beams without breaking them. The system consists of multiple ESP32-C3 modules working together:

- **Main Unit**: Central game controller with OLED display and web interface
- **Laser Units**: Combined laser emitter and beam detection units  
- **Finish Button**: Optional finish line button for successful game completion
- **All modules communicate wirelessly via ESP-NOW** for low-latency, reliable communication

Built with **ESP-IDF 5.5.2** and **ESP-ADF 2.7** for maximum performance and reliability.

## ✨ Features

### Game Mechanics
- ⏱️ **Time-based gameplay** - Time counts UP from zero, penalties add to total time
- 🎯 **Beam break detection** - Instant detection when lasers are interrupted
- ⚡ **Penalty system** - 3-second visual penalty display, configurable penalty time added to score
- 🏁 **Finish button support** - Complete game successfully via dedicated finish button device
- 🎮 **Multiple completion modes**:
  - ✅ **SOLVED**: Successfully completed via finish button
  - ❌ **CANCELED**: Aborted manually via web interface
  - ⏰ **TIME LIMIT**: Exceeded maximum allowed time

### Hardware Features
- 🌐 **ESP-NOW mesh network** - Wireless communication between all modules
- 🔄 **Automatic pairing** - Laser and finish button units auto-discover main unit
- 📡 **Multi-channel scanning** - Units scan channels 1, 6, 11 for reliable pairing
- 💓 **Heartbeat system** - 3-second heartbeat for online status monitoring
- 🔒 **Laser safety mechanism** - Auto-shutdown after 10 seconds without main unit heartbeat
- 🔋 **Low power optimized** - Efficient ESP32-C3 RISC-V architecture

### User Interface  
- 📱 **Web interface** - Full game control and monitoring via WiFi
- 🖥️ **OLED display (optional)** - Shows game status, time, and results
- 🔘 **Physical buttons (optional)** - 3-button control for standalone operation:
  - **Button 1**: Start/Stop/Resume (long press: toggle all lasers)
  - **Button 2**: Stop/Reset active game, returns to idle screen
  - **Button 3**: Debug Finish (configurable via menuconfig)
- 🎵 **Audio feedback (optional)** - I2S audio via MAX98357A amplifier or simple buzzer
- 💾 **SD Card support (optional)** - Custom web interface files and sound files from SD card

> ℹ️ **Note**: Display, buttons, buzzer, and SD card are all **optional**. The system works perfectly with just the web interface for full remote control.

### Display Features
- 📊 **Game status screens**:
  - Welcome/Idle screen
  - Countdown (3-2-1)
  - Running game with live time and beam breaks
  - Penalty notification (3-second display)
  - Results screen with completion status
- ✅ **Completion differentiation**:
  - "GAME COMPLETE!" for successful finish
  - "GAME CANCELED!" for manual abort or time limit
- 🕐 **Live time display** - Real-time updates in MM:SS.ms format
- 📈 **Beam break counter** - Shows total number of beam interruptions

### Web Interface Features
- 🎮 **Game control** - Start, stop, pause, resume games
- 🔴 **Laser control** - Individual ON/OFF control for each laser unit
- 📊 **Live status updates** - Real-time game state and statistics
- 🏁 **Special finish button display** - Finish buttons shown with 🏁 icon and green border
- 🌐 **Unit management** - View all connected units (laser and finish button)
- 📡 **Connection monitoring** - Online/offline status with RSSI indicators
- 💾 **SD Card web files** - Serve custom HTML/CSS/JS from SD card (optional)
- 🔄 **Automatic fallback** - Uses internal web interface if SD card unavailable

### Technical Features
- 🔧 **Modular component architecture** - Clean separation of concerns
- 📦 **Three module roles**:
  - **CONTROL**: Main unit with display and web server
  - **LASER**: Laser emitter + beam sensor unit
  - **FINISH**: Finish line button device
- 🎯 **Role-based pairing** - Units identify themselves during pairing
- 🔄 **Automatic recovery** - Re-pairing after main unit restart
- ⚙️ **Menuconfig-based setup** - Easy configuration via ESP-IDF menuconfig
- 💾 **Optional SD Card** - FAT filesystem support for custom web interface
- 🔌 **Optional peripherals** - Display, buttons, buzzer can be disabled
- 🌐 **Web-only operation** - System fully functional via WiFi without any physical UI

## 🛠️ Hardware Requirements

### Main Unit (CONTROL Module)
- **Microcontroller**: ESP32-C3-DevKitM-1 or compatible
- **Display**: 128x32 or 128x64 OLED (SSD1306) via I2C *(optional)*
- **Audio**: Passive buzzer or small speaker (PWM) *(optional)*
- **Input**: 4 push buttons *(optional, web interface provides full control)*
- **SD Card**: MicroSD card reader via SPI *(optional, for custom web files)*
- **Power**: USB-C or 5V power supply (500mA minimum)
- **WiFi**: Integrated for web interface (AP mode) **required**

> ⚠️ **Minimum Setup**: Only ESP32-C3 + WiFi required! All other components (display, buttons, buzzer, SD card) are optional for headless web-only operation.

### Laser Unit (LASER Module) - Per Beam
- **Microcontroller**: ESP32-C3-DevKitM-1
- **Laser**: 5V laser diode module (650nm red, <5mW Class 2)
- **Sensor**: LDR (Light Dependent Resistor) for beam detection
- **LEDs**: 3 status LEDs (status, green beam OK, red beam broken)
- **Power**: 5V power supply (250mA per unit)
- **Safety**: Automatic laser shutdown after 10s without heartbeat

### Finish Button Unit (FINISH Module) - Optional
- **Microcontroller**: ESP32-C3-DevKitM-1
- **Button**: Push button (active low with pull-up)
- **LEDs**: 2 LEDs (status LED, button illumination LED)
- **Power**: 5V power supply or battery
- **Function**: Press button to mark successful game completion

**⚠️ Laser Safety Warning**: Always use appropriate laser safety glasses. Never point lasers at people or reflective surfaces. Use only Class 2 lasers (<1mW, 650nm). Follow local regulations for laser devices.

## 📋 Bill of Materials (BOM)

| Component | Quantity | Purpose | Estimated Cost (USD) |
|-----------|----------|---------|---------------------|
| ESP32-C3-DevKitM-1 | 1 | Main Unit | $3-5 |
| ESP32-C3-DevKitM-1 | 4-8 | Laser Units | $3-5 each |
| ESP32-C3-DevKitM-1 | 1 | Finish Button (optional) | $3-5 |
| OLED Display 128x32/64 | 1 | Main display (optional) | $5-8 |
| MicroSD Card Module | 1 | Custom web files (optional) | $2-4 |
| MicroSD Card (any size) | 1 | Storage (optional) | $3-10 |
| 650nm Laser Module | 4-8 | Beam emitters | $2-4 each |
| LDR (Light Sensor) | 4-8 | Beam detection | $0.50-1 each |
| Passive Buzzer | 1 | Audio feedback (optional) | $1-2 |
| Push Buttons | 4-5 | Control input (optional) | $0.50 each |
| LEDs (various colors) | 10-20 | Status indicators | $0.10 each |
| Resistors/Capacitors | Various | Electronics | $5-10 |
| Power Supplies 5V | 5-9 | Power | $3-5 each |
| **Total (4-beam setup)** | - | - | **$80-150** |
| **Minimal setup (web-only)** | - | - | **$25-50** |

*Costs are estimates and may vary by supplier and region.*

## 🚀 Quick Start Guide

### 1. Install ESP-IDF 5.4.2

```bash
# Install prerequisites
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Clone ESP-IDF v5.4.2
mkdir -p ~/esp
cd ~/esp
git clone -b v5.4.2 --recursive https://github.com/espressif/esp-idf.git

# Install ESP-IDF
cd ~/esp/esp-idf
./install.sh esp32c3

# Activate environment
. ~/esp/esp-idf/export.sh
```

### 2. Clone and Build

```bash
git clone https://github.com/ninharp/esp32-laser-parcour.git
cd esp32-laser-parcour

# Configure module role
idf.py menuconfig
# Navigate to: Laser Parcour Configuration → Module Role
# Select: Main Unit / Laser Unit / Finish Button

# Build and flash
idf.py set-target esp32c3
idf.py build flash monitor
```

### 3. Pin Configuration

#### Main Unit (CONTROL)
| GPIO | Component | Description |
|------|-----------|-------------|
| 19 | OLED SDA | I2C Data *(optional)* |
| 18 | OLED SCL | I2C Clock *(optional)* |
| 5 | Buzzer | PWM Audio *(optional)* |
| 1 | Button 1 | Start/Stop/Resume *(optional)* |
| 3 | Button 2 | Stop/Reset *(optional)* |
| 2 | Button 3 | Debug Finish *(optional, configurable)* |
| 10 | SD CS | SD Card Chip Select *(optional)* |
| 6 | SD CLK | SD Card Clock *(optional)* |
| 2 | SD MISO | SD Card Data In *(optional)* |
| 7 | SD MOSI | SD Card Data Out *(optional)* |

#### Laser Unit (LASER)
| GPIO | Component | Description |
|------|-----------|-------------|
| 2 | Laser Diode | PWM Control |
| 4 | LDR Sensor | ADC Input (threshold: 2000) |
| 21 | Status LED | Connection status |
| 20 | Green LED | Beam detected |
| 10 | Red LED | Beam broken |

#### Finish Button (FINISH)
| GPIO | Component | Description |
|------|-----------|-------------|
| 5 | Button | Active low input |
| 21 | Status LED | Pairing/connection |
| 20 | Button LED | Illumination (turns off when pressed) |

*All pin assignments configurable via menuconfig*

## 🎮 How to Play

1. **Power on all units** - Main unit and all laser units
2. **Wait for pairing** - Units automatically find and pair with main unit
3. **Connect to WiFi** - Default SSID: "ESP32-LaserParcour"
4. **Open web interface** - Navigate to http://192.168.4.1
5. **Start game** - Via web interface or Button 1 on main unit
6. **Navigate course** - Avoid breaking laser beams
7. **Reach finish button** - Press finish button to complete successfully
8. **View results** - Display shows total time and beam breaks

### Scoring System
- **Time counts UP** from 0:00
- **Each beam break** adds penalty time (default: 15 seconds)
- **Final score** = Total time (including all penalties)
- **Lower time = better score**

## 📡 System Architecture

### Communication Flow
```
Main Unit (CONTROL)
    ├── ESP-NOW Channel (1, 6, or 11)
    ├── WiFi AP Mode (192.168.4.1)
    │
    ├─→ Laser Unit 1 (LASER) ──→ Heartbeat every 3s
    ├─→ Laser Unit 2 (LASER) ──→ Beam break messages
    ├─→ Laser Unit N (LASER) ──→ Auto-pairing
    │
    └─→ Finish Button (FINISH) ─→ MSG_FINISH_PRESSED on button press
```

### Message Types
- **MSG_GAME_START** (0x01) - Start game, turn on lasers
- **MSG_GAME_STOP** (0x02) - Stop game, turn off lasers
- **MSG_BEAM_BROKEN** (0x03) - Beam interrupted notification
- **MSG_HEARTBEAT** (0x06) - Keep-alive every 3 seconds
- **MSG_PAIRING_REQUEST** (0x07) - Auto-discovery message
- **MSG_PAIRING_RESPONSE** (0x08) - Pairing acknowledgment
- **MSG_LASER_ON/OFF** (0x09/0x0A) - Manual laser control
- **MSG_RESET** (0x0C) - Reset module state
- **MSG_FINISH_PRESSED** (0x0F) - Finish button pressed

## 🔧 Advanced Configuration

### Menuconfig Options

```bash
idf.py menuconfig
```

Navigate to **"Laser Parcour Configuration"**:

#### Module Settings
- **Module Role**: CONTROL / LASER / FINISH
- **Module ID**: 1-255 (unique identifier)
- **Device Name**: Custom name for web interface

#### Network Settings
- **WiFi SSID**: Access point name (default: ESP32-LaserParcour)
- **WiFi Password**: Access point password
- **WiFi Channel**: 1-13 (must match ESP-NOW channel)
- **ESP-NOW Channel**: 1, 6, or 11 recommended

#### Game Parameters
- **Max Time**: Maximum game duration (0 = unlimited)
- **Penalty Time**: Seconds added per beam break (default: 15)

#### Hardware Configuration
- **Display Type**: SSD1306 / SH1106 *(optional)*
- **Enable Display**: Checkbox to enable/disable display support
- **I2C Pins**: SDA/SCL for OLED *(optional)*
- **Button Pins**: GPIO assignments for buttons *(optional)*
- **Enable Buttons**: Checkbox to enable/disable button support
- **Buzzer Pin**: PWM output for audio *(optional)*
- **Enable Buzzer**: Checkbox to enable/disable buzzer support
- **Enable SD Card**: Checkbox to enable SD card support *(optional)*
- **SD Card Pins**: MOSI, MISO, CLK, CS for SPI SD card *(optional)*
- **Laser Pin**: PWM output for laser (LASER module)
- **Sensor Pin**: ADC input for LDR (LASER module)
- **Sensor Threshold**: ADC value (0-4095, default: 2000)
- **Finish Button Pins**: Button, status LED, illumination LED (FINISH module)

> 💡 **Tip**: Disable unused features in menuconfig to save flash space and RAM!

## 🌐 Web Interface

Access the web interface by connecting to the main unit's WiFi network:
- **SSID**: ESP32-LaserParcour (configurable)
- **Password**: lasergame (configurable)
- **URL**: http://192.168.4.1

### Features
- 🎮 Game control (start, stop, pause, resume)
- 🔴 Individual laser ON/OFF control
- 📊 Live game status and timer
- 🏁 Unit overview with finish button indicator
- 📡 Connection status and RSSI monitoring

### Custom Web Interface (SD Card)

You can customize the web interface by using an SD card:

1. **Format SD card** as FAT32
2. **Create folder structure**:
   ```
   /web/
   ├── index.html    (required)
   ├── style.css     (optional)
   ├── script.js     (optional)
   └── assets/       (optional)
       ├── logo.png
       └── sounds/
   /sounds/          (for audio feedback - requires MAX98357A I2S amplifier)
   ├── startup2.mp3  (system startup)
   ├── button.mp3    (button press)
   ├── start.mp3     (game start)
   ├── beep.mp3      (countdown tick)
   ├── bg.mp3        (background music - loop)
   ├── penalty.mp3   (laser beam broken)
   ├── finish.mp3    (game complete)
   ├── stop.mp3      (game stopped)
   ├── error.mp3     (error sound)
   └── success.mp3   (success/confirmation)
   ```
   
   > 📢 **Note**: Sound files are MP3 format. WAV files also supported. Sounds are optional - system works with buzzer-only feedback if no I2S audio configured.
3. **Enable SD Card** in menuconfig
4. **Configure SPI pins** for SD card module
5. **Insert SD card** into main unit
6. **Reboot** - System automatically detects `/web/index.html`

**Automatic Fallback**: If SD card is missing or `/web/index.html` not found, the system uses the internal web interface.

**Benefits**:
- Customize UI without reflashing firmware
- Update web files via file copy
- Add custom graphics, sounds, or themes
- Easy A/B testing of different interfaces

## 🛡️ Safety Features

### Laser Safety
- ✅ **Automatic shutdown** - Laser turns off after 10 seconds without heartbeat
- ✅ **Safety timeout** - Hardware safety timeout on laser control component
- ✅ **Visual warnings** - Red LED indicates safety shutdown
- ✅ **Low power lasers** - Only Class 2 lasers (<1mW) supported

### System Safety
- ✅ **Heartbeat monitoring** - Continuous connection monitoring
- ✅ **Automatic recovery** - Units re-pair after connection loss
- ✅ **Error logging** - Detailed logs for debugging
- ✅ **Fail-safe defaults** - Conservative default settings

## 📚 Component Documentation

### Core Components
- **display_manager** - OLED display abstraction (SSD1306/SH1106) *(optional)*
- **game_logic** - Game state management and scoring
- **espnow_manager** - ESP-NOW communication layer
- **laser_control** - Laser PWM control with safety
- **sensor_manager** - ADC-based beam detection
- **web_server** - HTTP server with REST API and SD card file serving
- **sd_card_manager** - SD card FAT filesystem support *(optional)*
- **button_handler** - Physical button input with debouncing *(optional)*
- **buzzer** - Audio feedback via PWM *(optional)*
- **wifi_ap_manager** - WiFi Access Point management

### Module Roles
Each ESP32 can be configured as one of three roles:
1. **CONTROL** - Main unit (display, web server, game logic)
2. **LASER** - Laser emitter + beam sensor
3. **FINISH** - Finish line button device

## 🐛 Troubleshooting

### Laser Units Won't Pair
- Check ESP-NOW channel matches WiFi channel
- Units scan channels 1, 6, 11 automatically
- Wait 10-15 seconds for multi-channel scanning
- Check module ID is unique (1-255)

### Laser Not Detecting Beams
- Verify sensor threshold (default: 2000)
- Check LDR connections
- Monitor ADC values in serial output
- LDR should read ~850 without laser, ~4095 with laser

### Display Shows Wrong Time/Breaks
- Verify game state in web interface
- Check heartbeat messages in serial log
- Ensure main unit is receiving beam break messages

### Web Interface Not Accessible
- Verify WiFi credentials in menuconfig
- Check main unit is in AP mode
- Default IP: 192.168.4.1
- Try rebooting main unit

### SD Card Not Detected
- Verify SD card is formatted as FAT32
- Check SPI pin configuration in menuconfig
- Ensure `/web/index.html` exists on SD card
- Monitor serial output for SD card mount errors
- System continues with internal web interface if SD card fails
- Try different SD card (some cards may be incompatible)

### Game Won't Start (No Laser Units Error)
- Check that at least one laser unit is powered on
- Verify laser units have completed pairing (status LED solid)
- Check ESP-NOW channel matches WiFi channel
- Monitor web interface for unit online status
- Error displays on OLED for 5 seconds if no units found
- Web interface shows "No laser units found" error message

## 📄 License

This project is licensed under the MIT License - see LICENSE file for details.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## 📮 Contact

- **Author**: ninharp
- **Project**: https://github.com/ninharp/esp32-laser-parcour
- **Issues**: https://github.com/ninharp/esp32-laser-parcour/issues

## 🎉 Acknowledgments

- ESP-IDF framework by Espressif Systems
- SSD1306 OLED driver community
- ESP-NOW protocol developers

---

**Version**: 1.0.0 (January 2026)  
**ESP-IDF**: 5.4.2  
**Target**: ESP32-C3 (RISC-V)
