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

#### ESP-IDF Installation

This project uses ESP-IDF 5.1 or later. Follow the official installation guide:

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

### 3. Hardware Wiring

#### Laser Station (Control Module)

| ESP32-C3 Pin | Component | Description |
|--------------|-----------|-------------|
| GPIO4 | OLED SDA | I2C Data Line |
| GPIO5 | OLED SCL | I2C Clock Line |
| GPIO6 | Buzzer | Audio Output |
| GPIO7 | Button 1 | Start/Stop |
| GPIO8 | Button 2 | Mode Select |
| GPIO9 | Button 3 | Reset |
| 5V | VCC | Power Supply |
| GND | GND | Ground |

#### Laser Unit (Transmitter/Receiver Module)

| ESP32-C3 Pin | Component | Description |
|--------------|-----------|-------------|
| GPIO2 | Laser Module | Laser Control (PWM) |
| GPIO3 | Photoresistor | Analog Input (ADC) |
| GPIO10 | LED Red | Status Indicator |
| GPIO18 | LED Green | Ready Indicator |
| 5V | VCC | Power Supply |
| GND | GND | Ground |

**⚠️ Laser Safety Warning**: Always use appropriate laser safety glasses. Never point lasers at people or reflective surfaces. Ensure proper ventilation and follow local regulations for Class 2 laser devices.

### 4. Building the Firmware

#### Build Laser Station Firmware

```bash
cd laser_station
idf.py set-target esp32c3
idf.py build
```

#### Build Laser Unit Firmware

```bash
cd laser_unit
idf.py set-target esp32c3
idf.py build
```

### 5. Flashing the Firmware

#### Flash Laser Station

```bash
cd laser_station
idf.py -p /dev/ttyUSB0 flash monitor
# Replace /dev/ttyUSB0 with your serial port (COM3 on Windows)
```

Press `Ctrl+]` to exit the monitor.

#### Flash Laser Units

Repeat for each laser unit module:

```bash
cd laser_unit
idf.py -p /dev/ttyUSB0 flash
```

**Note**: Label each programmed module (Unit 1, 2, 3, etc.) to track them during setup.

### 6. Configuration

#### ESP-NOW Network Setup

On first boot, each module will:
1. Generate a unique MAC address identifier
2. Enter pairing mode (LED flashing)
3. Wait for network discovery

**Pairing Process**:
1. Power on the Laser Station first
2. Power on each Laser Unit one at a time
3. The station will automatically discover and pair with units
4. Green LED indicates successful pairing
5. Repeat for all units

#### Module Configuration

Access the web interface for advanced configuration:

```bash
# Connect to the Laser Station's WiFi AP
SSID: ESP32-Laser-Station
Password: laser12345

# Open browser and navigate to:
http://192.168.4.1
```

Available settings:
- Beam sensitivity threshold
- Game timer duration
- Sound effect volume
- LED brightness
- Network channel
- Module assignment (which unit controls which beam)

## 🎮 Usage

### Starting a Game

1. **Power On**: Ensure all modules are powered and paired (green LEDs)
2. **Mode Selection**: Press Mode button to cycle through:
   - Single Player Speed Run
   - Multi-Player Challenge
   - Training Mode (no penalties)
   - Custom Game (configurable)
3. **Start**: Press Start button to begin countdown
4. **Play**: Navigate through the laser beams without breaking them
5. **Results**: View time and penalties on the OLED display

### Game Modes

#### Single Player Speed Run
- Objective: Complete the course as fast as possible
- 5-second penalty for each beam break
- Best times saved to leaderboard

#### Multi-Player Challenge
- Objective: Lowest score wins (time + penalties)
- Each player takes turns
- Supports up to 8 players

#### Training Mode
- Objective: Practice without pressure
- No penalties for beam breaks
- Visual feedback only

### Button Controls

| Button | Function | Long Press |
|--------|----------|------------|
| Button 1 | Start/Pause | Stop Game |
| Button 2 | Next Mode | Previous Mode |
| Button 3 | Reset Score | System Reset |

### LED Status Indicators

| Color | Meaning |
|-------|---------|
| 🟢 Green Solid | Ready/Paired |
| 🔴 Red Solid | Beam Broken |
| 🟡 Yellow Blink | Pairing Mode |
| 🔵 Blue Pulse | Active Game |
| ⚪ White Flash | Network Error |

## 🔧 Troubleshooting

### Module Not Pairing

**Problem**: Laser unit won't connect to station

**Solutions**:
- Verify both devices are powered
- Check if both are running the same firmware version
- Reset both devices (hold Button 3 for 5 seconds)
- Verify ESP-NOW channel matches (default: channel 1)
- Check antenna connection if using external antenna

### Inconsistent Beam Detection

**Problem**: Sensor triggers randomly or doesn't detect breaks

**Solutions**:
- Adjust photoresistor sensitivity in web interface
- Ensure laser is properly aligned with sensor
- Check for ambient light interference (shield sensor)
- Verify power supply is stable (minimum 5V 2A recommended)
- Clean laser lens and sensor window

### OLED Display Not Working

**Problem**: Display remains blank or shows garbage

**Solutions**:
- Check I2C connections (SDA/SCL not swapped)
- Verify I2C address (default 0x3C, some use 0x3D)
- Test with I2C scanner: `idf.py monitor` and check logs
- Ensure 5V power supply is adequate
- Try adding pull-up resistors (4.7kΩ) to SDA/SCL

### WiFi Configuration Issues

**Problem**: Can't connect to web interface

**Solutions**:
- Verify AP mode is enabled (check serial monitor logs)
- Restart the laser station module
- Manually connect to WiFi SSID: "ESP32-Laser-Station"
- Check if another device is using IP 192.168.4.1
- Try factory reset via serial monitor menu

### Build Errors

**Problem**: Compilation fails

**Solutions**:
```bash
# Clean build directory
idf.py fullclean

# Reconfigure
idf.py set-target esp32c3
idf.py reconfigure

# Rebuild
idf.py build
```

If issues persist:
- Verify ESP-IDF version (must be v5.1 or later)
- Check that environment is properly sourced
- Update ESP-IDF: `cd ~/esp/esp-idf && git pull && ./install.sh esp32c3`

## 📊 Project Structure

```
esp32-laser-parcour/
├── README.md                 # This file
├── .gitignore               # Git ignore rules
├── laser_station/           # Control module firmware
│   ├── CMakeLists.txt       # Build configuration
│   ├── sdkconfig.defaults   # Default ESP-IDF config
│   └── main/                # Source code
│       ├── CMakeLists.txt
│       └── main.c           # Main application (to be implemented)
└── laser_unit/              # Laser transmitter/receiver firmware
    ├── CMakeLists.txt       # Build configuration
    ├── sdkconfig.defaults   # Default ESP-IDF config
    └── main/                # Source code
        ├── CMakeLists.txt
        └── main.c           # Main application (to be implemented)
```

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines

- Follow ESP-IDF coding standards
- Add comments for complex logic
- Test on actual hardware before submitting
- Update documentation for new features
- Keep commits atomic and well-described

## 📜 License

This project is licensed under the MIT License - see below for details:

```
MIT License

Copyright (c) 2026 ninharp

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

## 🙏 Acknowledgments

- **Espressif Systems** - For the ESP-IDF framework and ESP32-C3 platform
- **ESP32 Community** - For examples and support
- **Contributors** - Everyone who has contributed to this project

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/ninharp/esp32-laser-parcour/issues)
- **Discussions**: [GitHub Discussions](https://github.com/ninharp/esp32-laser-parcour/discussions)
- **Email**: sauer.uetersen@gmail.com

## 🔗 Related Resources

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/)
- [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [ESP-NOW Protocol Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/network/esp_now.html)
- [Laser Safety Standards](https://www.osha.gov/laser-hazards)

---

**⚠️ Safety Notice**: This project involves lasers and electronics. Always follow safety guidelines, use appropriate protective equipment, and comply with local regulations. The authors are not responsible for any injuries or damages resulting from building or using this system.
