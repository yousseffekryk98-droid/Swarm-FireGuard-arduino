# Car 2: Fire Detection and Suppression Robot

## Project Overview
Autonomous firefighting robot with ultrasonic obstacle detection, directional servo scanning, and water pump control.

## Features
- **Autonomous Fire Detection**: Dual flame sensors with priority emergency response
- **Water Pump System**: Automated suppression when fire is detected
- **Obstacle Avoidance**: Ultrasonic scanning left/right before turns
- **Servo Scanning**: Look left (150°) and right (30°) to determine best direction
- **Motor Control**: L298N with adjustable speed via analogWrite
- **WiFi/UDP Control**: Remote command control with non-blocking scan operations

## Hardware Requirements
- ESP32 Development Board
- L298N Motor Driver
- HC-SR04 Ultrasonic Sensor
- SG90 Servo Motor
- Flame Sensors (2x, digital input with pull-up)
- Water Pump + Motor transistor
- Motor wheels and chassis

## Pin Configuration
| Component | Pin |
|-----------|-----|
| Motor A In1 | GPIO 25 |
| Motor A In2 | GPIO 26 |
| Motor B In3 | GPIO 14 |
| Motor B In4 | GPIO 13 |
| Motor A Enable | GPIO 27 |
| Motor B Enable | GPIO 16 |
| Ultrasonic TRIG | GPIO 18 |
| Ultrasonic ECHO | GPIO 19 |
| Servo | GPIO 15 |
| Flame Sensor 1 | GPIO 32 |
| Flame Sensor 2 | GPIO 33 |
| Water Pump | GPIO 4 |

## Code Versions

### v1: Autonomous (Car2_Fire_v1_Autonomous.ino)
- Serial-based autonomous operation
- Ultrasonic obstacle detection at 15cm threshold
- Dual servo scanning for best direction
- Water pump activation (5 seconds) on flame detection
- Backup and turn left after suppression
- No network connectivity

### v2: WiFi UDP (Car2_Fire_v2_WiFiUDP.ino)
- Full WiFi connectivity with UDP communication
- Three operational modes: HOLD, AUTO, MANUAL
- Non-blocking servo sweep (3-point scan)
- Real-time distance telemetry to PC
- Remote pump control via commands
- Swarm-ready architecture

## Network Configuration (v2)
```
WiFi SSID: SwarmRobotics_ECU
Password: ECU@2025
Local IP: 192.168.1.112
Local UDP Port: 5002
Remote IP: 192.168.1.100
Remote UDP Port: 6002
```

## Control Commands (UDP)
- `forward` - Move forward
- `backward` - Move backward
- `left` - Turn left
- `right` - Turn right
- `stop` - Stop motors
- `pump_on` - Activate water pump
- `pump_off` - Deactivate water pump
- `mode_auto` - Switch to automatic mode
- `mode_manual` - Switch to manual mode
- `mode_hold` - Switch to hold/stop mode
- `scan` - Perform 3-point distance scan

## Behavior Notes
- Flame sensors are active LOW (normally HIGH)
- In fire scenario: pump runs for 2 seconds then backs up
- Servo scans happen only during obstacle avoidance (not continuous)
- Default forward speed: 150 units
