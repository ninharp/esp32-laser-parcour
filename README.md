# ESP32 Laser Obstacle Course Game System

A modular, ESP32-C3 based laser obstacle course game system featuring wireless control, real-time monitoring, and an interactive gaming experience. Perfect for events, arcades, or DIY gaming setups.

## 🎯 Overview

This project implements a distributed laser obstacle course system where players must navigate through laser beams without breaking them. The system consists of multiple ESP32-C3 modules working together:

- **Control Module**: Central game controller managing game state and coordination
- **Laser Modules**: Emit laser beams across the course
- **Sensor Modules**: Detect beam interruptions and player movements
- **Display Module**: Shows game status, timer, and scores

All modules communicate wirelessly using ESP-NOW protocol for low-latency, reliable communication.

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

### Control Module
- **Microcontroller**: ESP32-C3-DevKitM-1 or compatible
- **Display**: 128x64 OLED (SSD1306) via I2C
- **Audio**: Passive buzzer or small speaker
- **Input**: 2-4 push buttons for control
- **Power**: USB-C or 5V power supply (500mA minimum)

### Laser Module (per beam)
- **Microcontroller**: ESP32-C3-DevKitM-1
- **Laser**: 5V laser diode module (650nm red, <5mW Class 2)
- **Driver**: Laser driver circuit with safety cutoff
- **Power**: 5V power supply (200mA per laser)

### Sensor Module (per detection point)
- **Microcontroller**: ESP32-C3-DevKitM-1
- **Sensor**: Photoresistor or photodiode with amplifier
- **Analog Circuit**: Op-amp based signal conditioning
- **LEDs**: Status indicator LEDs (red/green)
- **Power**: 5V power supply (150mA)

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
| ESP32-C3-DevKitM-1 | 5-10 | $3-5 each |
| 650nm Laser Module | 4-8 | $2-4 each |
| OLED Display 128x64 | 1 | $5-8 |
| Photoresistor/Photodiode | 4-8 | $1-2 each |
| Passive Buzzer | 1 | $1-2 |
| Push Buttons | 4 | $0.50 each |
| Resistors/Capacitors | Various | $5-10 |
| Power Supplies | 5-10 | $3-5 each |
| **Total (basic 4-beam setup)** | - | **$80-150** |

## 🚀 Setup Instructions

### 1. Software Prerequisites

```bash
# Install PlatformIO Core
pip install platformio

# Or use PlatformIO IDE (VSCode extension)
# https://platformio.org/install/ide?install=vscode
```

### 2. Clone the Repository

```bash
git clone https://github.com/ninharp/esp32-laser-parcour.git
cd esp32-laser-parcour
```

### 3. Configure the Project

Edit `include/config.h` to customize:
- WiFi credentials (for web interface)
- Module types and roles
- Game parameters (timing, scoring)
- Pin assignments
- ESP-NOW network settings

```cpp
// Example configuration
#define MODULE_TYPE MODULE_CONTROL
#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourPassword"
#define NUM_LASERS 6
#define GAME_TIMEOUT 300  // 5 minutes
```

### 4. Build and Upload

```bash
# For Control Module
pio run -e control -t upload

# For Laser Module
pio run -e laser -t upload

# For Sensor Module
pio run -e sensor -t upload
```

### 5. Hardware Assembly

1. **Connect the Control Module**:
   - OLED Display → I2C (GPIO 8: SDA, GPIO 9: SCL)
   - Buzzer → GPIO 5
   - Buttons → GPIO 2, 3, 4, 6
   - Power via USB-C

2. **Setup Laser Modules**:
   - Connect laser diode to GPIO 10 via driver circuit
   - Add safety resistor and current limiting
   - Status LED on GPIO 2

3. **Install Sensor Modules**:
   - Photoresistor/photodiode on ADC (GPIO 0)
   - Signal conditioning circuit
   - Indicator LEDs on GPIO 1 and GPIO 2

4. **Position Equipment**:
   - Mount lasers at various heights and angles
   - Align sensors with laser beams
   - Ensure stable mounting to prevent drift

### 6. Initial Configuration

1. Power on the Control Module
2. It will create an Access Point: `ESP32-LaserParcour`
3. Connect to it (password: `lasergame`)
4. Navigate to `http://192.168.4.1`
5. Configure module addresses and game settings
6. Power on Laser and Sensor modules
7. They will auto-pair with the Control Module

## 🏗️ Architecture Overview

### System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Control Module                        │
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
        ┌─────────────────┴─────────────────┐
        ↓                                    ↓
┌───────────────┐                   ┌───────────────┐
│ Laser Modules │                   │Sensor Modules │
│  ┌─────────┐  │                   │  ┌─────────┐  │
│  │ Laser   │  │                   │  │ Photo   │  │
│  │ Control │  │                   │  │ Detector│  │
│  └─────────┘  │                   │  └─────────┘  │
│  ┌─────────┐  │                   │  ┌─────────┐  │
│  │ Safety  │  │                   │  │ Signal  │  │
│  │ Monitor │  │                   │  │ Process │  │
│  └─────────┘  │                   │  └─────────┘  │
└───────────────┘                   └───────────────┘
```

### Communication Protocol

The system uses ESP-NOW for fast, peer-to-peer communication:

```cpp
// Message Structure
struct GameMessage {
    uint8_t msgType;      // Command type
    uint8_t moduleId;     // Source module ID
    uint32_t timestamp;   // Message timestamp
    uint8_t data[32];     // Payload data
    uint16_t checksum;    // Data integrity check
};
```

**Message Types**:
- `MSG_GAME_START` - Initiate game session
- `MSG_GAME_STOP` - End game session
- `MSG_BEAM_BROKEN` - Laser beam interruption detected
- `MSG_STATUS_UPDATE` - Periodic status report
- `MSG_CONFIG_UPDATE` - Configuration changes
- `MSG_HEARTBEAT` - Keep-alive signal

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
├── include/
│   ├── config.h              # Global configuration
│   ├── game_logic.h          # Game state management
│   ├── espnow_manager.h      # ESP-NOW communication
│   ├── laser_control.h       # Laser module interface
│   ├── sensor_manager.h      # Sensor module interface
│   └── display_manager.h     # Display and UI
├── src/
│   ├── main.cpp              # Entry point and setup
│   ├── game_logic.cpp        # Game implementation
│   ├── espnow_manager.cpp    # Communication layer
│   ├── laser_control.cpp     # Laser operations
│   ├── sensor_manager.cpp    # Sensor processing
│   └── display_manager.cpp   # UI rendering
├── lib/
│   └── custom_libraries/     # Project-specific libraries
├── test/
│   └── unit_tests/           # Unit tests
├── docs/
│   ├── hardware/             # Schematics and PCB files
│   ├── api/                  # API documentation
│   └── images/               # Photos and diagrams
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

## 🔧 Configuration

### Module Configuration

Each module must be configured with a unique ID and role:

```cpp
// In config.h
#define MODULE_ID 1
#define MODULE_ROLE ROLE_CONTROL

// Available roles:
// ROLE_CONTROL  - Central controller
// ROLE_LASER    - Laser emitter
// ROLE_SENSOR   - Beam detector
```

### Game Settings

Customize game behavior:

```cpp
// Timing
#define GAME_DURATION 180000      // 3 minutes in ms
#define PENALTY_TIME 5000         // 5 second penalty
#define COUNTDOWN_TIME 5          // 5 second countdown

// Scoring
#define BASE_SCORE 1000
#define TIME_BONUS_MULTIPLIER 10
#define PENALTY_POINTS -50

// Sensitivity
#define SENSOR_THRESHOLD 500      // ADC threshold
#define DEBOUNCE_TIME 100         // ms
```

## 📡 Web Interface

Access the web interface at `http://<module-ip>` for:
- Real-time game status monitoring
- Module configuration
- Score history and statistics
- Firmware updates (OTA)
- System diagnostics

## 🔒 Safety Considerations

⚠️ **IMPORTANT SAFETY INFORMATION**

1. **Laser Safety**:
   - Use only Class 2 lasers (<1mW, 650nm)
   - Never point lasers at eyes or reflective surfaces
   - Install emergency stop button
   - Post warning signs in play area

2. **Electrical Safety**:
   - Use proper power supplies with overcurrent protection
   - Insulate all exposed connections
   - Keep electronics away from water

3. **Physical Safety**:
   - Ensure stable mounting of all equipment
   - Mark laser paths clearly
   - Provide adequate lighting in play area
   - Supervise all gameplay sessions

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👤 Author

**ninharp**
- GitHub: [@ninharp](https://github.com/ninharp)

## 🙏 Acknowledgments

- ESP32 community for excellent documentation
- PlatformIO team for the development platform
- Contributors and testers

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/ninharp/esp32-laser-parcour/issues)
- **Discussions**: [GitHub Discussions](https://github.com/ninharp/esp32-laser-parcour/discussions)

## 🗺️ Roadmap

- [ ] Mobile app for remote control
- [ ] Multiple game modes (time trial, survival, multiplayer)
- [ ] Cloud leaderboard integration
- [ ] Advanced pattern programming for lasers
- [ ] RGB laser support for difficulty levels
- [ ] Sound effect customization
- [ ] Tournament mode with bracket management

---

**Last Updated**: January 2026

**Status**: Active Development

Made with ❤️ by ninharp
