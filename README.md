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
