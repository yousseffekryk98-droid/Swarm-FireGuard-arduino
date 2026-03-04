# FireGuard Swarm Firmware Structure

## Overview
All robot firmware is organized in the `firmware/` directory following a modular architecture:
- **common_ai/**: Shared libraries and utilities (reusable across all units)
- **car1_search/**: Search & Fire Detection unit firmware
- **car2_3_extinguish/**: Unified firmware for Cars 2 & 3 (configurable)
- **car4_rescue/**: Search & Rescue unit firmware with audio beacon

## Directory Structure

```
firmware/
├── common_ai/
│   ├── motor_control.h          # L298N Motor Driver abstraction
│   ├── sensor_control.h         # Ultrasonic & Flame sensor interface
│   ├── pump_control.h           # Water pump controller
│   └── network_config.h         # WiFi/UDP centralized configuration
│
├── car1_search/
│   ├── car1_search.ino          # Main firmware file
│   └── README.md                # Unit-specific documentation
│
├── car2_3_extinguish/
│   ├── car2_3_extinguish.ino    # Configurable firmware (set UNIT_TYPE)
│   └── README.md                # Configuration guide
│
└── car4_rescue/
    ├── car4_rescue.ino          # Main firmware file
    └── README.md                # Unit-specific documentation
```

## Quick Start

### 1. Upload to Car 1 (Search & Fire Detection)
```
1. Open: firmware/car1_search/car1_search.ino
2. Select: ESP32 Dev Board
3. Configure: Motor pins at top of file (if needed)
4. Upload
```

### 2. Upload to Cars 2 & 3 (Fire Extinguishing)
```
1. Open: firmware/car2_3_extinguish/car2_3_extinguish.ino
2. Find: Line with #define UNIT_TYPE
3. Select: UNIT_CAR2 or UNIT_CAR3
4. Upload
```

### 3. Upload to Car 4 (Search & Rescue)
```
1. Open: firmware/car4_rescue/car4_rescue.ino
2. Ensure: DFPlayer connected to UART2 (GPIO16/17)
3. Configure: SD card with MP3 files
4. Upload
```

## Common Libraries Usage

### motor_control.h
Simple motor abstraction:
```cpp
MotorController motor(IN1, IN2, IN3, IN4, ENA, ENB, 150);
motor.forward();
motor.backward();
motor.left();
motor.right();
motor.stop();
motor.setSpeed(200);
```

### sensor_control.h
Sensor interfaces:
```cpp
UltrasonicSensor ultrasonic(TRIG, ECHO);
long distance = ultrasonic.getDistance();

FlameSensor flame(PIN1, PIN2);
if (flame.isFlameDetected()) { /* ... */ }
```

### pump_control.h
Pump control:
```cpp
PumpController pump(PUMP_PIN);
pump.start();
pump.pulse(5000);  // 5-second pulse
pump.stop();
```

### network_config.h
Unified WiFi setup:
```cpp
uint8_t ip[] = UNIT_CAR1_IP;
configureWiFi(ip, UNIT_CAR1_LOCAL_PORT);
```

## Network Configuration

All units connect to:
- **SSID**: `SwarmRobotics_ECU`
- **Password**: `ECU@2025`
- **Gateway**: `192.168.1.1`

Unit IP Assignments:
| Unit | IP | Local Port | Remote Port |
|------|----|----|---|
| Car 1 | 192.168.1.111 | 5001 | 6001 |
| Car 2 | 192.168.1.112 | 5002 | 6002 |
| Car 3 | 192.168.1.113 | 5003 | 6003 |
| Car 4 | 192.168.1.114 | 5004 | 6004 |

Central PC listens at: `192.168.1.100`

## UDP Command Protocol

Simple text-based commands sent to each unit's local port:

### Movement
- `forward` - Move forward
- `backward` - Move backward
- `left` - Turn left
- `right` - Turn right
- `stop` - Stop motors

### Modes
- `mode_auto` - Autonomous operation
- `mode_manual` - Manual remote control
- `mode_hold` - Complete stop/hold

### Special
- `scan` - Perform distance scan (3-point: L/C/R)
- `pump_on` / `pump_off` - Pump control (Cars 2/3)
- `audio_on` / `audio_off` - Audio control (Car 4)

### Responses
Units send telemetry data (distance readings) to central station every 100ms.
Scan results format: `L:distance,C:distance,R:distance`

## Emergency Response Priority

**Flame Detection** > All other commands

When flame is detected:
1. Motors stop immediately
2. Pump activates (Cars 2/3)
3. Spray servo aims downward (Car 3)
4. Alert sent to central: `FIRE_DETECTED` or `FIRE_ALERT_CAR1`
5. Unit backs away and turns left

## Customization Guide

### Changing Motor Speed
In each .ino file, modify:
```cpp
#define MOTOR_SPEED 150  // 0-255
```

### Changing Pin Assignments
Update `#define` statements at the beginning of each sketch.

### Adjusting Obstacle Detection
Modify in main loop:
```cpp
const int SAFE_DISTANCE_CM = 20;  // Change threshold
```

### Adding New Commands
In `handleNetworkCommand()` function:
```cpp
else if (cmd == "mycommand") {
  // Your code here
}
```

## Troubleshooting

### Unit won't connect to WiFi
- Check SSID/password in `network_config.h`
- Verify router is broadcasting
- Check IP configuration doesn't conflict

### Motors not responding
- Verify IN1/IN2/IN3/IN4 pin assignments
- Check motor power supply
- Test with `digitalWrite()` directly

### No distance readings
- Verify ultrasonic TRIG/ECHO pins
- Check sensor isn't blocked
- Test with `Serial.println()` debug output

### Car 4 audio not playing
- Verify DFPlayer power (3.3V or 5V depending on module)
- Check SD card format (FAT32)
- Verify MP3 names (0001.mp3, 0002.mp3, etc.)
- Check RX/TX on UART2 (GPIO16/17)

## Advanced: Integrating ESP32-CAM

Each unit can optionally include an ESP32-CAM module for vision:

1. Add ESP32-CAM to SPI/I2C bus
2. Create `camera_control.h` in `common_ai/`
3. Import and use in main firmware
4. Add CV model calls for fire/human detection

See individual README files for specific integration guides.

## Version Control Notes

- Keep common libraries (`common_ai/*`) synchronized across all units
- Each car folder is independently deployable
- Use git tags for versioned releases
- Test on hardware before committing

## Performance Targets

- **WiFi latency**: <50ms (local network)
- **Telemetry frequency**: 10Hz (100ms intervals)
- **Command response**: <20ms
- **Servo sweep speed**: ~100ms per 5 degrees
- **Motor acceleration**: Immediate (can be softened)

## Support & Documentation

Refer to individual README.md files in each car directory for detailed information:
- `firmware/car1_search/README.md`
- `firmware/car2_3_extinguish/README.md`
- `firmware/car4_rescue/README.md`
