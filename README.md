# ESP32 Laser Parcour

A laser-based timing and obstacle detection system using ESP32 microcontrollers with retroreflective architecture.

## Architecture Overview

This system uses a **retroreflective architecture** where each detection unit contains both the laser transmitter and the optical sensor in a single device. The laser beam is directed toward a passive retroreflector film mounted on the opposite side, which reflects the light directly back to the sensor in the same unit.

### How It Works

1. **Active Unit (ESP32 + Laser + Sensor)**: Each detection point consists of one ESP32-based unit that houses:
   - A laser diode that emits a focused beam
   - An optical sensor that detects the reflected laser light
   - Control electronics and communication interfaces

2. **Passive Retroreflector**: On the opposite side of the detection area:
   - Retroreflective film or tape that reflects incoming light back to its source
   - No active electronics or power required
   - Simple installation and maintenance

3. **Detection Method**: 
   - The laser continuously emits toward the retroreflector
   - The reflected beam returns to the sensor in the same unit
   - When an object (person, vehicle, etc.) breaks the beam, the sensor detects the interruption
   - The ESP32 processes the interruption and triggers appropriate actions (timing, alarms, etc.)

### Advantages of Retroreflective Architecture

- **Simplified Installation**: Only one side requires power and data connectivity
- **Cost Effective**: Passive retroreflectors are inexpensive and require no maintenance
- **Easy Alignment**: Retroreflectors have a wide angle of acceptance
- **Reliable Detection**: Less susceptible to environmental interference
- **Scalable**: Easy to add multiple detection points without complex wiring

## Features

- ESP32-based wireless connectivity
- Real-time beam interruption detection
- Low power consumption
- Easy integration with timing systems
- Configurable detection sensitivity
- Network-enabled for remote monitoring and control

## Hardware Requirements

- ESP32 development board
- Laser diode module (typically 650nm red laser, 5mW)
- Optical sensor (photodiode or phototransistor)
- Retroreflective film or tape
- Power supply (5V USB or battery)
- Optional: Enclosure for weather protection

## Applications

- Sports timing systems (running tracks, ski courses, etc.)
- Security perimeter detection
- Obstacle course monitoring
- Vehicle detection and counting
- Industrial automation

## Installation

1. Mount the ESP32 unit with laser and sensor on one side of the detection area
2. Install retroreflective film/tape on the opposite side, aligned with the laser beam
3. Adjust the laser aim to ensure maximum reflection back to the sensor
4. Configure the ESP32 software for your specific application
5. Test the system by breaking the beam and verifying detection

## Safety

- Use Class 2 lasers (< 1mW) for public applications to ensure eye safety
- Clearly mark laser operation areas
- Follow local regulations for laser device operation
- Mount lasers at appropriate heights to avoid eye-level exposure

## License

[Specify your license here]

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for improvements and bug fixes.

## Author

ninharp
