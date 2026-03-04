# Car 3: Advanced Fire Detection and Suppression Robot (Fire 2)

## Project Overview
Advanced firefighting robot with dual servo system, LEDC PWM motor control, intelligent spray angle control, and sophisticated obstacle avoidance.

## Features
- **Dual Servo System**: One for sensor scanning, one for spray direction control
- **LEDC PWM Control**: Advanced ESP32 PWM for precise motor speed management
- **Spray Angle Control**: Dynamic spray nozzle positioning (45° attack, 90° idle)
- **Soft Deceleration**: Gradual speed reduction for smoother transitions
- **Smart Speed Control**: Distance-based adaptive speed algorithm
- **Advanced Obstacle Avoidance**: 4-point directional scanning (30°, 45°, 135°, 150°)
- **Random Turn Decision**: Prevents stuck patterns during extended exploration
- **WiFi/UDP Control**: Full remote command capability

## Hardware Requirements
- ESP32 Development Board with LEDC capability
- L298N Motor Driver
- HC-SR04 Ultrasonic Sensor
- 2x SG90 Servo Motors (scanning + spray control)
- Flame Sensors (2x, digital input with pull-up)
- Water Pump + Motor transistor
- Motor wheels and chassis

## Pin Configuration (v1: Advanced)
| Component | Pin |
|-----------|-----|
| Motor A In1 | GPIO 18 |
| Motor A In2 | GPIO 5 |
| Motor B In3 | GPIO 17 |
| Motor B In4 | GPIO 16 |
| Motor A Enable | GPIO 19 |
| Motor B Enable | GPIO 4 |
| Sensor Servo | GPIO 33 |
| Spray Servo | GPIO 32 |
| Ultrasonic TRIG | GPIO 27 |
| Ultrasonic ECHO | GPIO 26 |
| Flame Sensor 1 | GPIO 13 |
| Flame Sensor 2 | GPIO 15 |
| Water Pump | GPIO 25 |

## Pin Configuration (v2: WiFi UDP)
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

### v1: Advanced (Car3_Fire2_v1_Advanced.ino)
- Serial console control (w/W, s/S, a/A, d/D, x/X commands)
- LEDC PWM for ultra-smooth motor control
- Dual servo independent control
- Spray angle management (45° fire, 90° idle)
- Soft deceleration ramp for smooth stops
- Manual timeout (5 seconds back to auto)
- Dual-point scanning per direction (30°+45° right, 135°+150° left)
- Random turn decision after 10s of forward movement
- Pulse pump for periodic suppression

### v2: WiFi UDP (Car3_Fire2_v2_WiFiUDP.ino)
- Full WiFi UDP protocol (same as Car 2 v2)
- Three operational modes: HOLD, AUTO, MANUAL
- Non-blocking 3-point servo scan
- Real-time distance telemetry
- Simplified single servo control for mass production

## Network Configuration (v2)
```
WiFi SSID: SwarmRobotics_ECU
Password: ECU@2025
Local IP: 192.168.1.113
Local UDP Port: 5003
Remote IP: 192.168.1.100
Remote UDP Port: 6003
```

## Control Commands (v1 - Serial)
```
w or W - Move Forward (W = turbo)
s or S - Move Backward (S = turbo)
a or A - Turn Left (A = turbo)
d or D - Turn Right (D = turbo)
x or X - Soft Stop
```

## Control Commands (v2 - UDP)
- `forward`, `backward`, `left`, `right`, `stop`
- `pump_on`, `pump_off`
- `mode_auto`, `mode_manual`, `mode_hold`
- `scan`

## Behavior Notes (v1)
- Default speed: 200 units
- Turbo speed: 255 units
- Safe distance threshold: 10 cm
- Soft deceleration: 20 units per 50ms
- Auto move duration: 10 seconds
- Pulse pump duration: 300 ms
- Spray servo angles: 45° (fire), 90° (idle)

## LEDC PWM Configuration
- Channels: 0 (Motor A), 1 (Motor B)
- Frequency: 5000 Hz
- Resolution: 8-bit (0-255)

## Advanced Features
1. **Directional Preference**: Chooses between max(left1, left2) and max(right1, right2) for better obstacle avoidance
2. **Adaptive Speed**: autoSpeedByDistance() provides smooth acceleration control
3. **Soft Stop**: Gradient deceleration prevents sudden jerking
4. **Randomized Exploration**: Prevents repetitive patterns in featureless environments
5. **Dual Servo Freedom**: Independent control enables advanced spray mechanisms
