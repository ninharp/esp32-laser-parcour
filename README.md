# ESP32 Laser Obstacle Course Game System

A modular, ESP32-C3 based laser obstacle course game system featuring wireless control, real-time monitoring, and an interactive gaming experience. Perfect for events, arcades, or DIY gaming setups.

## 🎯 Overview

This project implements a distributed laser obstacle course system where players must navigate through laser beams without breaking them. The system consists of multiple ESP32-C3 modules working together:

- **Main Unit**: Central game controller with OLED display and web interface
- **Laser Units**: Combined laser emitter and beam detection units  
- **Finish Button**: Optional finish line button for successful game completion
- **All modules communicate wirelessly via ESP-NOW** for low-latency, reliable communication

Built with **ESP-IDF 5.4.2** for maximum performance and reliability.

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
- 🖥️ **OLED display (32px)** - Shows game status, time, and results
- 🔘 **Physical buttons** - 4-button control for standalone operation:
  - **Button 1**: Start/Stop/Resume (long press: toggle all lasers)
  - **Button 2**: Pause/Resume during game
  - **Button 3**: Stop/Reset active game
  - **Button 4**: Reserved for future use
- 🎵 **Audio feedback** - Buzzer with multiple sound patterns

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

### Technical Features
- 🔧 **Modular component architecture** - Clean separation of concerns
- 📦 **Three module roles**:
  - **CONTROL**: Main unit with display and web server
  - **LASER**: Laser emitter + beam sensor unit
  - **FINISH**: Finish line button device
- 🎯 **Role-based pairing** - Units identify themselves during pairing
- 🔄 **Automatic recovery** - Re-pairing after main unit restart
- ⚙️ **Menuconfig-based setup** - Easy configuration via ESP-IDF menuconfig

## 🛠️ Hardware Requirements

### Main Unit (CONTROL Module)
- **Microcontroller**: ESP32-C3-DevKitM-1 or compatible
- **Display**: 128x32 or 128x64 OLED (SSD1306) via I2C
- **Audio**: Passive buzzer or small speaker (PWM)
- **Input**: 4 push buttons (optional, web interface also available)
- **Power**: USB-C or 5V power supply (500mA minimum)
- **WiFi**: Integrated for web interface (AP mode)

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
| OLED Display 128x32/64 | 1 | Main display | $5-8 |
| 650nm Laser Module | 4-8 | Beam emitters | $2-4 each |
| LDR (Light Sensor) | 4-8 | Beam detection | $0.50-1 each |
| Passive Buzzer | 1 | Audio feedback | $1-2 |
| Push Buttons | 4-5 | Control input | $0.50 each |
| LEDs (various colors) | 10-20 | Status indicators | $0.10 each |
| Resistors/Capacitors | Various | Electronics | $5-10 |
| Power Supplies 5V | 5-9 | Power | $3-5 each |
| **Total (4-beam setup)** | - | - | **$80-150** |

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
| 19 | OLED SDA | I2C Data |
| 18 | OLED SCL | I2C Clock |
| 5 | Buzzer | PWM Audio |
| 1 | Button 1 | Start/Stop/Resume |
| 3 | Button 2 | Pause/Resume |
| 7 | Button 3 | Stop/Reset |
| 6 | Button 4 | Reserved |

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
- **Display Type**: SSD1306 / SH1106
- **I2C Pins**: SDA/SCL for OLED
- **Button Pins**: GPIO assignments for buttons
- **Buzzer Pin**: PWM output for audio
- **Laser Pin**: PWM output for laser (LASER module)
- **Sensor Pin**: ADC input for LDR (LASER module)
- **Sensor Threshold**: ADC value (0-4095, default: 2000)
- **Finish Button Pins**: Button, status LED, illumination LED (FINISH module)

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
- **display_manager** - OLED display abstraction (SSD1306/SH1106)
- **game_logic** - Game state management and scoring
- **espnow_manager** - ESP-NOW communication layer
- **laser_control** - Laser PWM control with safety
- **sensor_manager** - ADC-based beam detection
- **web_server** - HTTP server with REST API
- **button_handler** - Physical button input with debouncing
- **buzzer** - Audio feedback via PWM

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
