# Car 1: Search and Fire Detection

## Project Overview
Smart ESP32 RC Car with autonomous navigation, fire detection, and flame suppression.

## Features
- **Ultrasonic Sensor**: 1-180° servo sweep for obstacle detection
- **Flame Sensors**: Left and right flame detection with emergency stop
- **Bluetooth Control**: Manual override via Bluetooth serial
- **WiFi Dashboard**: Web-based control interface
- **Obstacle Avoidance**: Dynamic navigation around obstacles
- **Multi-Motor Control**: L298N driver with PWM speed control

## Hardware Requirements
- ESP32 Development Board
- L298N Motor Driver
- HC-SR04 Ultrasonic Sensor
- SG90 Servo Motor
- Flame Sensors (2x, digital input)
- Motor wheels and chassis

## Pin Configuration
| Component | Pin |
|-----------|-----|
| Ultrasonic TRIG | GPIO 26 |
| Ultrasonic ECHO | GPIO 25 |
| Servo | GPIO 18 |
| Motor IN1 | GPIO 14 |
| Motor IN2 | GPIO 27 |
| Motor IN3 | GPIO 12 |
| Motor IN4 | GPIO 13 |
| Motor ENA | GPIO 32 |
| Motor ENB | GPIO 33 |
| Flame Left | GPIO 34 |
| Flame Right | GPIO 35 |

## Code Versions

### v1: Basic (Car1_SearchAndFireDetection_v1_Basic.ino)
- Simple WiFi AP with web dashboard control
- Basic motor control without PWM
- Digital-only motor enable/disable
- Flame sensor emergency stop
- Servo sweep obstacle detection

### v2: Full System (Car1_SearchAndFireDetection_v2_FullSystem.ino)
- Enhanced WiFi AP + Bluetooth control
- PWM-based motor speed control (analogWrite)
- Full servo sweep with angle tracking
- Best-direction pathfinding algorithm
- Support for manual and automatic modes

### v3: WiFi UDP (Car1_SearchAndFireDetection_v3_WiFiUDP.ino)
- Network-based communication with central control PC
- Non-blocking scan operations
- Real-time distance telemetry
- Modes: HOLD, AUTO, MANUAL
- Dedicated control protocol via UDP

## Wiring Diagram
All motors connected to L298N driver. Servo and ultrasonic require separate power considerations.
Flame sensors use 3.3V digital logic (active LOW).

## Integration Notes
- Flame sensors have HIGHEST priority in auto mode
- Servo sweeps during AUTO mode, centers during MANUAL/HOLD
- UDP version intended for swarm coordination
