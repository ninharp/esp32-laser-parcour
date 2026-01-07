# ESP32 Laser Obstacle Course Game System

A modular, ESP32-C3 based laser obstacle course game system featuring wireless control, real-time monitoring, and an interactive gaming experience. Perfect for events, arcades, or DIY gaming setups.

## 🎯 Overview

This project implements a distributed laser obstacle course system where players must navigate through laser beams without breaking them. The system consists of multiple ESP32-C3 modules working together:

- **Main Unit**: Central game controller managing game state and coordination
- **Laser Units**: Combined units that emit laser beams and detect beam interruptions
- **Display Module**: Shows game status, timer, and scores (integrated in Main Unit)

All modules communicate wirelessly using ESP-NOW protocol for low-latency, reliable communication.

Built with **ESP-IDF 5.1+** for maximum performance and reliability.

## ✨ Features

### Core Functionality
- 🎮 **Multi-player support** - Track multiple players and their scores
- ⏱️ **Real-time timing** - Precise time tracking for speedrun challenges
- 🔴 **Beam break detection** - Instant detection when lasers are interrupted
- 📊 **Live scoring system** - Dynamic scoring based on performance
- 🎵 **Audio feedback** - Sound effects for events (hits, completion, etc.)
- 💡 **Visual indicators** - LED feedback for game states

### Technical Features
- 🌐 **Wireless mesh network** - ESP-NOW based communication
- 🔋 **Low power consumption** - Optimized for battery operation
- 🔧 **Modular architecture** - Easy to add/remove modules
- 📱 **Web interface** - Configure and monitor via built-in web server
- 🔄 **OTA updates** - Update firmware wirelessly
- 📈 **Performance metrics** - Real-time monitoring and statistics

## 🛠️ Hardware Requirements

### Main Unit
- **Microcontroller**: ESP32-C3-DevKitM-1 or compatible
- **Display**: 128x64 OLED (SSD1306) via I2C
- **Audio**: Passive buzzer or small speaker
- **Input**: 2-4 push buttons for control
- **Power**: USB-C or 5V power supply (500mA minimum)

### Laser Unit (per beam)
- **Microcontroller**: ESP32-C3-DevKitM-1
- **Laser**: 5V laser diode module (650nm red, <5mW Class 2)
- **Driver**: Laser driver circuit with safety cutoff
- **Sensor**: Photoresistor or photodiode with amplifier (integrated in same unit)
- **Analog Circuit**: Op-amp based signal conditioning
- **LEDs**: Status indicator LEDs (red/green)
- **Power**: 5V power supply (250mA per unit)

### Optional Components
- **Mounting Hardware**: Adjustable laser mounts, tripods
- **Mirrors**: For creating complex beam patterns
- **Battery Packs**: 18650 Li-ion with boost converter (for portable operation)
- **Enclosures**: 3D printed or commercial project boxes

### Recommended Tools
- Soldering iron and supplies
- Multimeter
- 3D printer (for custom enclosures)
- Basic hand tools

## 📋 Bill of Materials (BOM)

| Component | Quantity | Estimated Cost (USD) |
|-----------|----------|---------------------|
| ESP32-C3-DevKitM-1 | 5 (1 Main + 4 Laser Units) | $3-5 each |
| 650nm Laser Module | 4-8 | $2-4 each |
| OLED Display 128x64 | 1 | $5-8 |
| Photoresistor/Photodiode | 4-8 | $1-2 each |
| Passive Buzzer | 1 | $1-2 |
| Push Buttons | 4 | $0.50 each |
| Resistors/Capacitors | Various | $5-10 |
| Power Supplies | 5 | $3-5 each |
| **Total (basic 4-beam setup)** | - | **$70-130** |

## 🚀 Setup Instructions

### 1. Software Prerequisites

#### ESP-IDF Installation

This project requires ESP-IDF 5.1 or later. Follow the official installation guide:

```bash
# Install ESP-IDF prerequisites (Linux/macOS)
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Clone ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone -b v5.1 --recursive https://github.com/espressif/esp-idf.git

# Install ESP-IDF
cd ~/esp/esp-idf
./install.sh esp32c3

# Set up the environment (add to ~/.bashrc for persistence)
. ~/esp/esp-idf/export.sh
```

For Windows or detailed instructions, visit: https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32c3/get-started/

### 2. Clone the Repository

```bash
git clone https://github.com/ninharp/esp32-laser-parcour.git
cd esp32-laser-parcour
```

### 3. Configure the Project

Use the ESP-IDF configuration menu to customize settings:

```bash
idf.py menuconfig
```

Navigate to **"Laser Parcour Configuration"** to set:

- Module type (Main Unit or Laser Unit)
- Module ID (unique identifier 1-255)
- WiFi credentials (for web interface)
- Game parameters (timing, scoring)
- ESP-NOW network settings
- Pin assignments

Or edit `sdkconfig.defaults` directly for default values.

### 4. Build the Firmware

```bash
# Set target to ESP32-C3
idf.py set-target esp32c3

# Build the project
idf.py build
```

### 5. Flash and Monitor

```bash
# Flash to device and open serial monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Press Ctrl+] to exit monitor
```

Replace `/dev/ttyUSB0` with your serial port:
- Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
- macOS: `/dev/cu.usbserial-*`
- Windows: `COM3`, `COM4`, etc.

### 6. Hardware Wiring

#### Main Unit

| ESP32-C3 Pin | Component | Description |
|--------------|-----------|-------------|
| GPIO8 | OLED SDA | I2C Data Line |
| GPIO9 | OLED SCL | I2C Clock Line |
| GPIO5 | Buzzer | Audio Output (PWM) |
| GPIO2 | Button 1 | Start/Stop |
| GPIO3 | Button 2 | Mode Select |
| GPIO4 | Button 3 | Reset |
| GPIO6 | Button 4 | Confirm |
| 5V | VCC | Power Supply |
| GND | GND | Ground |

#### Laser Unit (Combined Laser + Sensor)

| ESP32-C3 Pin | Component | Description |
|--------------|-----------|-------------|
| GPIO10 | Laser Module | Laser Control (PWM) |
| GPIO0 | Photoresistor | Analog Input (ADC1_CH0) |
| GPIO1 | LED Green | Beam Detected Indicator |
| GPIO2 | LED Red | Beam Broken / Status LED |
| 5V | VCC | Power Supply |
| GND | GND | Ground |

**⚠️ Laser Safety Warning**: Always use appropriate laser safety glasses. Never point lasers at people or reflective surfaces. Use only Class 2 lasers (<1mW, 650nm). Follow local regulations for laser devices.

### 7. Module Configuration

Each ESP32 module needs to be flashed with the appropriate role configuration:

**For Main Unit:**
```bash
idf.py menuconfig
# Navigate to: Laser Parcour Configuration → Module Settings
# Set Module Role: Main Unit
# Set Module ID: 1
idf.py build flash
```

**For Laser Units:**
```bash
idf.py menuconfig
# Set Module Role: Laser Unit
# Set Module ID: 2, 3, 4... (unique for each laser unit)
idf.py build flash
```

### 8. Initial System Setup

1. **Power on the Main Unit** - It will create WiFi AP: `ESP32-LaserParcour`
2. **Connect to the AP** - Password: `lasergame` (configurable in menuconfig)
3. **Access Web Interface** - Navigate to http://192.168.4.1
4. **Power on Laser Units** - They will auto-pair via ESP-NOW
5. **Verify connectivity** - Check web interface for connected modules
6. **Align hardware** - Position laser units, test beam detection

## 🏗️ Architecture Overview

### System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                      Main Unit                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │ Game     │  │ ESP-NOW  │  │ Web      │             │
│  │ Logic    │←→│ Manager  │  │ Server   │             │
│  └──────────┘  └──────────┘  └──────────┘             │
│       ↓              ↓              ↓                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │ Display  │  │ Audio    │  │ Storage  │             │
│  └──────────┘  └──────────┘  └──────────┘             │
└─────────────────────────────────────────────────────────┘
                          ↓ ESP-NOW
                    ┌─────┴─────┐
                    ↓           ↓
            ┌───────────────────────────┐
            │    Laser Units (1-N)      │
            │  ┌─────────┐  ┌─────────┐ │
            │  │ Laser   │  │ Photo   │ │
            │  │ Control │  │ Detector│ │
            │  └─────────┘  └─────────┘ │
            │  ┌─────────┐  ┌─────────┐ │
            │  │ Safety  │  │ Signal  │ │
            │  │ Monitor │  │ Process │ │
            │  └─────────┘  └─────────┘ │
            └───────────────────────────┘
```

### Communication Protocol

The system uses ESP-NOW for fast, peer-to-peer communication:

```c
// Message Structure
typedef struct {
    uint8_t msg_type;      // Command type
    uint8_t module_id;     // Source module ID
    uint32_t timestamp;    // Message timestamp (ms)
    uint8_t data[32];      // Payload data
    uint16_t checksum;     // CRC16 data integrity check
} game_message_t;
```

**Message Types:**

- `MSG_GAME_START` - Initiate game session
- `MSG_GAME_STOP` - End game session
- `MSG_BEAM_BROKEN` - Laser beam interruption detected
- `MSG_STATUS_UPDATE` - Periodic status report (every 100ms)
- `MSG_CONFIG_UPDATE` - Configuration changes
- `MSG_HEARTBEAT` - Keep-alive signal (every 1s)
- `MSG_PAIRING_REQUEST` - Module wants to join network
- `MSG_PAIRING_RESPONSE` - Pairing accepted/rejected

### State Machine

```
     [IDLE] ──start──→ [READY] ──begin──→ [RUNNING]
       ↑                                       │
       │                                       │ beam_broken
       │                                       ↓
       │                                   [PENALTY]
       │                                       │
       │                                       │ resume
       │                                       ↓
       └──finish/timeout──────────────────[COMPLETE]
```

## 📁 Project Structure

```
esp32-laser-parcour/
├── CMakeLists.txt            # Main build configuration
├── sdkconfig.defaults        # Default ESP-IDF configuration
├── partitions.csv            # Flash partition table
├── main/
│   ├── CMakeLists.txt        # Main component build config
│   ├── main.c                # Application entry point
│   ├── Kconfig.projbuild     # Configuration menu definitions
│   └── idf_component.yml     # Component dependencies
├── components/
│   ├── game_logic/           # Game state management
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── game_logic.h
│   │   └── game_logic.c
│   ├── espnow_manager/       # ESP-NOW communication layer
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── espnow_manager.h
│   │   └── espnow_manager.c
│   ├── laser_control/        # Laser module control
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── laser_control.h
│   │   └── laser_control.c
│   ├── sensor_manager/       # Sensor data processing
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── sensor_manager.h
│   │   └── sensor_manager.c
│   └── display_manager/      # OLED display and UI
│       ├── CMakeLists.txt
│       ├── include/
│       │   └── display_manager.h
│       └── display_manager.c
├── docs/
│   ├── hardware/             # Schematics and PCB files
│   ├── api/                  # API documentation
│   └── images/               # Photos and diagrams
└── README.md                 # This file
```

## 🔧 Configuration

### Module Configuration via menuconfig

Access configuration menu:

```bash
idf.py menuconfig
```

Key configuration options under **"Laser Parcour Configuration"**:

#### Module Settings
- **Module Role**: Main Unit / Laser Unit
- **Module ID**: 1-255 (must be unique)
- **Device Name**: Custom name for identification

#### Network Settings
- **WiFi SSID**: Access point name (Main Unit only)
- **WiFi Password**: AP password (minimum 8 characters)
- **ESP-NOW Channel**: 1-13 (all modules must match)
- **Max Paired Devices**: Maximum connected modules (default: 20)

#### Game Parameters
- **Game Duration**: Default game time in seconds (180)
- **Penalty Time**: Time penalty per beam break in seconds (5)
- **Countdown Duration**: Pre-game countdown in seconds (5)
- **Base Score**: Starting score (1000)
- **Time Bonus Multiplier**: Points per second remaining (10)
- **Penalty Points**: Points deducted per beam break (-50)

#### Hardware Configuration
- **OLED Display Type**: SSD1306 / SH1106
- **I2C SDA Pin**: GPIO for I2C data (default: 8)
- **I2C SCL Pin**: GPIO for I2C clock (default: 9)
- **Buzzer Pin**: GPIO for audio output (default: 5)
- **Laser Pin**: GPIO for laser control (default: 10)
- **Sensor Pin**: ADC channel for photoresistor (default: GPIO0)
- **Sensor Threshold**: ADC threshold for beam detection (0-4095, default: 500)
- **Debounce Time**: Debounce delay in ms (default: 100)

### Configuration via sdkconfig.defaults

Alternatively, edit `sdkconfig.defaults` for persistent settings:

```ini
# Module Configuration
CONFIG_MODULE_ROLE_CONTROL=y
CONFIG_MODULE_ID=1

# Network Settings
CONFIG_WIFI_SSID="ESP32-LaserParcour"
CONFIG_WIFI_PASSWORD="lasergame"
CONFIG_ESPNOW_CHANNEL=1

# Game Parameters
CONFIG_GAME_DURATION=180
CONFIG_PENALTY_TIME=5
CONFIG_BASE_SCORE=1000

# Hardware Configuration  
CONFIG_I2C_SDA_PIN=8
CONFIG_I2C_SCL_PIN=9
CONFIG_BUZZER_PIN=5
CONFIG_LASER_PIN=10
CONFIG_SENSOR_PIN=0
CONFIG_SENSOR_THRESHOLD=500
```

## 🎮 Usage

### Starting a Game

1. **Power On**: Ensure all modules are powered and paired (check web interface)
2. **Mode Selection**: Press Mode button (Button 2) to cycle through:
   - Single Player Speed Run
   - Multi-Player Challenge
   - Training Mode (no penalties)
   - Custom Game (configurable duration and rules)
3. **Start**: Press Start button (Button 1) to begin countdown
4. **Play**: Navigate through the laser beams without breaking them
5. **Results**: View time, penalties, and final score on the OLED display

### Game Modes

#### Single Player Speed Run
- **Objective**: Complete the course as fast as possible
- **Penalty**: 5-second time penalty for each beam break
- **Scoring**: Based on completion time minus penalties
- **Leaderboard**: Best times saved to flash memory

#### Multi-Player Challenge
- **Objective**: Lowest score wins (time + penalties)
- **Players**: Up to 8 players can compete
- **Turns**: Each player takes turns on the course
- **Scoring**: Time in seconds + (beam breaks × 5)

#### Training Mode
- **Objective**: Practice without pressure
- **Penalties**: No time penalties or score deductions
- **Feedback**: Visual and audio feedback on beam breaks
- **Stats**: Track progress and improvement over time

#### Custom Game
- **Duration**: Set custom game duration (60-600 seconds)
- **Penalties**: Configure custom penalty values
- **Difficulty**: Adjust sensor sensitivity
- **Rules**: Enable/disable various game mechanics

### Button Controls

| Button | Short Press | Long Press (2s) |
|--------|-------------|-----------------|
| Button 1 (GPIO2) | Start/Pause Game | Stop Game & Return to Menu |
| Button 2 (GPIO3) | Next Mode | Previous Mode |
| Button 3 (GPIO4) | Reset Current Score | Clear All Scores |
| Button 4 (GPIO6) | Confirm Selection | System Reset |

### LED Status Indicators

**Control Module:**
- No dedicated status LED (uses OLED display)

**Laser Units:**

| LED State | Meaning |
|-----------|---------|
| 🔴 Solid Red | Pairing mode / Not connected |
| 🟢 Solid Green | Connected and ready |
| 🟡 Blinking Yellow | Active game in progress |
| 🔵 Fast Blink Blue | Receiving configuration |

The green LED on the Laser Unit also indicates when the beam is detected (normal operation), while the red LED indicates when the beam is broken (triggered).

## 📡 Web Interface

Access the web interface by connecting to the Main Unit's WiFi network and navigating to `http://192.168.4.1`.

### Features

**Dashboard:**
- Real-time game status
- Connected modules overview
- Current scores and times
- Active player information

**Module Management:**
- View all paired modules
- Module health monitoring
- Signal strength (RSSI)
- Battery status (if applicable)
- Assign module roles and IDs

**Game Configuration:**
- Select game modes
- Adjust timing parameters
- Configure scoring rules
- Set difficulty levels

**Statistics:**
- Historical game data
- Player leaderboards
- Average completion times
- Beam break statistics
- Performance graphs

**System:**
- Firmware version information
- OTA firmware updates
- Factory reset
- Export/import configuration
- System logs and diagnostics

## 🔧 Troubleshooting

### Module Not Pairing

**Problem**: Laser unit won't connect to main unit

**Solutions**:
- Verify all modules are on the same ESP-NOW channel (check menuconfig)
- Check that main unit WiFi AP is active
- Ensure modules are within ESP-NOW range (< 200m outdoor, < 100m indoor)
- Reset both devices (long press Button 4)
- Check serial monitor for pairing errors: `idf.py monitor`
- Verify MAC addresses are being exchanged correctly

### Inconsistent Beam Detection

**Problem**: Sensor triggers randomly or doesn't detect breaks

**Solutions**:
- Adjust sensor threshold in menuconfig or web interface
- Ensure laser is properly aligned with photoresistor
- Check for ambient light interference (add light shield to sensor)
- Verify ADC readings: use `idf.py monitor` and check sensor values
- Clean laser lens and sensor window
- Verify power supply is stable (5V ±5%, minimum 2A total)
- Increase debounce time in configuration

### OLED Display Not Working

**Problem**: Display remains blank or shows garbage

**Solutions**:
- Check I2C connections (ensure SDA/SCL not swapped)
- Verify I2C address (default 0x3C, some displays use 0x3D)
- Test I2C bus: `idf.py monitor` and check for I2C init messages
- Ensure pull-up resistors are present (usually on-board, or add 4.7kΩ)
- Try different OLED display type in menuconfig (SSD1306 vs SH1106)
- Check power supply to display (3.3V or 5V depending on model)
- Verify I2C clock speed (default 100kHz, try lowering if issues persist)

### WiFi Configuration Issues

**Problem**: Can't connect to web interface

**Solutions**:
- Verify AP mode is enabled in menuconfig (Main Unit only)
- Check serial monitor for WiFi initialization logs
- Restart the main unit (power cycle)
- Manually connect to WiFi SSID from menuconfig
- Verify WiFi channel is not congested (try different channel)
- Check if another device is using IP 192.168.4.1
- Try factory reset: Hold Button 3 + Button 4 during boot

### Build Errors

**Problem**: Compilation fails or dependency issues

**Solutions**:
```bash
# Clean build directory
idf.py fullclean

# Reconfigure target
idf.py set-target esp32c3

# Reconfigure project
idf.py reconfigure

# Rebuild
idf.py build
```

**Common issues:**
- ESP-IDF version mismatch: This project requires v5.1 or later
- Environment not sourced: Run `. ~/esp/esp-idf/export.sh`
- Missing dependencies: Run `cd ~/esp/esp-idf && ./install.sh esp32c3`
- Corrupted sdkconfig: Delete `sdkconfig` and run `idf.py menuconfig`

### ESP-NOW Communication Failures

**Problem**: Modules don't receive messages or high packet loss

**Solutions**:
- Check ESP-NOW channel matches across all modules
- Verify WiFi isn't in use simultaneously (ESP-NOW and WiFi share radio)
- Reduce distance between modules or add better antennas
- Check for WiFi interference on the selected channel
- Monitor RSSI values in web interface
- Increase ESP-NOW retry count in code
- Verify MAC addresses are correctly stored
- Check that maximum peer limit isn't exceeded (default 20)

### Laser Safety Cutoff Triggering

**Problem**: Laser shuts off unexpectedly

**Solutions**:
- This is a safety feature - check why it's triggering
- Verify laser current isn't exceeding limits
- Check for overheating (add heatsink if needed)
- Ensure proper ventilation around laser unit
- Verify power supply can handle current spikes
- Check safety circuit connections and thresholds
- Review system logs for safety event details

## 🔒 Safety Considerations

### ⚠️ IMPORTANT SAFETY INFORMATION

**Laser Safety:**

- **Use only Class 2 lasers** (<1mW, 650nm) - Safe for accidental brief exposure
- **Never point lasers at eyes or reflective surfaces** that could redirect the beam
- **Post warning signs** in the play area: "LASER IN USE - DO NOT STARE INTO BEAM"
- **Install emergency stop button** connected to all laser modules
- **Use laser safety glasses** (OD 3+ for 650nm) during setup and maintenance
- **Keep lasers at least 2m above ground** to avoid eye-level exposure
- **Implement automatic timeout** - lasers turn off after 10 minutes
- **Add beam terminators** at the end of each laser path

**Electrical Safety:**

- Use **proper power supplies with overcurrent protection** (minimum 2A, 5V regulated)
- **Insulate all exposed connections** with heat shrink tubing or electrical tape
- **Keep electronics away from water** - use weatherproof enclosures if outdoor
- **Add fuses** on all power supply lines (1A fast-blow recommended)
- **Use proper wire gauges** - minimum 22 AWG for power, 24 AWG for signals
- **Implement reverse polarity protection** on all power inputs
- **Ground all metal enclosures** properly to prevent static discharge

**Physical Safety:**

- **Ensure stable mounting** of all equipment - use proper stands or wall mounts
- **Mark laser paths clearly** with caution tape or barriers
- **Provide adequate lighting** in play area for safe navigation
- **Install padding** around obstacles or sharp edges
- **Supervise all gameplay sessions** - do not leave system unattended when active
- **Create clear entry/exit paths** that don't cross active beams
- **Implement motion sensors** to detect people outside play area
- **Secure all cables** to prevent tripping hazards

**Software Safety:**

- **Watchdog timers** monitor all modules and reset if frozen
- **Fail-safe defaults** - system defaults to safe state on error
- **Timeout mechanisms** - automatic shutdown after inactivity
- **Health monitoring** - continuous checks of all modules
- **Emergency stop protocol** - immediate shutdown on any safety trigger

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

### How to Contribute

1. **Fork the repository**
2. **Create your feature branch** (`git checkout -b feature/AmazingFeature`)
3. **Commit your changes** (`git commit -m 'Add some AmazingFeature'`)
4. **Push to the branch** (`git push origin feature/AmazingFeature`)
5. **Open a Pull Request**

### Development Guidelines

- Follow ESP-IDF coding standards and style guide
- Add comprehensive comments for complex logic
- Test on actual hardware before submitting PR
- Update documentation for new features
- Keep commits atomic and well-described
- Run `idf.py build` to ensure no compilation errors
- Check for memory leaks with `idf.py monitor`
- Follow existing code structure and patterns

### Areas for Contribution

- Additional game modes
- Mobile app development
- Web interface improvements
- Hardware schematics and PCB designs
- 3D printable enclosures
- Documentation and tutorials
- Translations
- Bug fixes and optimizations

## 📄 License

This project is licensed under the MIT License:

```
MIT License

Copyright (c) 2025 ninharp

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## 👤 Author

**ninharp**

- GitHub: [@ninharp](https://github.com/ninharp)
- Email: sauer.uetersen@gmail.com

## 🙏 Acknowledgments

- **Espressif Systems** - For the ESP-IDF framework and ESP32-C3 platform
- **ESP32 Community** - For examples, libraries, and support
- **Open Source Contributors** - For OLED libraries, ESP-NOW examples, and more
- **Testers and Early Adopters** - For feedback and bug reports

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/ninharp/esp32-laser-parcour/issues)
- **Discussions**: [GitHub Discussions](https://github.com/ninharp/esp32-laser-parcour/discussions)
- **Email**: sauer.uetersen@gmail.com
- **Documentation**: Check the `/docs` folder for additional resources

## 🗺️ Roadmap

### Version 1.0 (Current)
- [x] Basic game logic implementation
- [x] ESP-NOW communication
- [x] OLED display support
- [x] Multiple game modes
- [x] Web interface for configuration

### Version 1.1 (Planned)
- [ ] Mobile app for remote control (Android/iOS)
- [ ] Additional game modes (time trial, survival, obstacle course)
- [ ] Sound effects and music support
- [ ] Improved LED animations
- [ ] Battery monitoring and low-power modes

### Version 2.0 (Future)
- [ ] Cloud leaderboard integration
- [ ] Tournament mode with bracket management
- [ ] Advanced pattern programming for lasers
- [ ] RGB laser support for difficulty levels
- [ ] Multi-language support
- [ ] Replay system with video recording
- [ ] AI-powered difficulty adjustment
- [ ] Integration with gaming platforms

### Hardware Roadmap
- [ ] Custom PCB designs for Main Unit and Laser Units
- [ ] 3D printable enclosures and mounts
- [ ] Integrated battery packs with charging
- [ ] Weatherproof outdoor versions
- [ ] Modular expansion system

---

**Last Updated:** January 2025

**Status:** Active Development

**Made with ❤️ by ninharp**
