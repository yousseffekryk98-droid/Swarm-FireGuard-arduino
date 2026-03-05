# FireGuard Swarm Robotics System 🤖🔥

A multi-agent autonomous swarm platform for fire detection, suppression, and search-and-rescue operations. Four specialized ESP32-based robots operate over a local WiFi network with edge AI capabilities.

## Project Structure

```
swarm_robot/
├── firmware/                          # All robot firmware (modular architecture)
│   ├── common_ai/                     # Shared libraries & utilities
│   │   ├── motor_control.h            # L298N Motor Driver abstraction
│   │   ├── sensor_control.h           # Ultrasonic & Flame detection
│   │   ├── pump_control.h             # Water pump relay control
│   │   └── network_config.h           # WiFi/UDP centralized config
│   │
│   ├── car1_search/                   # Car 1 - Search & Fire Detection
│   │   ├── car1_search.ino
│   │   └── README.md
│   │
│   ├── car2_3_extinguish/             # Cars 2 & 3 - Fire Extinguishing
│   │   ├── car2_3_extinguish.ino      # Configurable (UNIT_TYPE)
│   │   └── README.md
│   │
│   ├── car4_rescue/                   # Car 4 - Search & Rescue
│   │   ├── car4_rescue.ino
│   │   └── README.md
│   │
│   └── README.md                      # Firmware guide & quick start
│
├── Car1_SearchAndFireDetection/       # Legacy structure (kept for reference)
├── Car2_Fire/
├── Car3_Fire2/
├── Car4_SearchAndRescue/
│
└── README.md (this file)
```

## Fleet Overview

| Unit | Role | Hardware | Key Features |
|------|------|----------|--------------|
| **Car 1** | Search & Detection | SR04 Ultrasonic, Dual Flame Sensors | Reconnaissance, Fire Detection, Emergency Alert |
| **Car 2** | Standard Suppression | 1 Servo, Pump Relay | Lower cost variant, single spray servo |
| **Car 3** | Advanced Suppression | 2 Servos, LEDC PWM | Dual servo control, precise spray angle |
| **Car 4** | Search & Rescue | 1 Servo, DFPlayer Mini | Audio beacon (10s intervals), Survivor guidance |

## Quick Start - Deployment

### Hardware Requirements Per Unit
- **ESP32-WROOM** (all units)
- **L298N Motor Driver** (12V supply)
- **SG90 Servo** (50Hz, 0-180°)
- **HC-SR04 Ultrasonic** (3-400cm range)
- **Digital Flame Sensors** (dual redundancy)
- **DFPlayer Mini** (Car 4 only)

### Network Configuration
```
WiFi SSID:       SwarmRobotics_ECU
Password:        ECU@2025
Central Station: 192.168.1.100

Robot IPs:
  Car 1: 192.168.1.111 (Ports 5001/6001)
  Car 2: 192.168.1.112 (Ports 5002/6002)
  Car 3: 192.168.1.113 (Ports 5003/6003)
  Car 4: 192.168.1.114 (Ports 5004/6004)
```

### Compiling & Uploading
1. **Open Arduino IDE** with ESP32 board support
2. **Navigate to firmware directory**:
   ```
   swarm_robot/firmware/car1_search/car1_search.ino
   swarm_robot/firmware/car2_3_extinguish/car2_3_extinguish.ino  (set UNIT_TYPE)
   swarm_robot/firmware/car4_rescue/car4_rescue.ino
   ```
3. **Include Path**: Ensure `firmware/common_ai/` headers are accessible
   - Copy common_ai/*.h to Arduino libraries, OR
   - Use relative #include paths in sketches
4. **Select Board**: Tools → Board → ESP32 → "ESP32 Dev Module"
5. **Select Port**: Tools → Port → COM[X]
6. **Upload**: Sketch → Upload (or Ctrl+U)

## Unit Details

### 🔍 Car 1: Search & Fire Detection
- **File**: `firmware/car1_search/car1_search.ino`
- **Role**: Perimeter reconnaissance with fire hazard detection
- **Hardware**: SR04 Ultrasonic (1-180° sweep), Dual flame sensors, 1 servo
- **Communication**: WiFi UDP + Bluetooth fallback ("FireGuard_Car1")
- **Key Features**:
  - Non-blocking 3-point scan state machine (Left/Center/Right)
  - Continuous servo sweep in AUTO mode
  - Emergency flame detection alert to central
  - 100ms telemetry streaming
  
**Deployment**: Primary reconnaissance unit; dispatch first when fire suspected

### 🔥 Car 2: Standard Fire Suppression
- **File**: `firmware/car2_3_extinguish/car2_3_extinguish.ino` (set `#define UNIT_TYPE UNIT_CAR2`)
- **Role**: Cost-effective fire extinguishing with standard spray control
- **Hardware**: 1 Servo + Pump relay (single spray point)
- **Key Features**:
  - Lower component cost than Car 3
  - Single servo for spray directional control
  - Automatic fire response: pump on → servo aims (45°) → reverses
  - Compatible with existing water pump systems
  
**Deployment**: Deploy in pairs (Car 2 + Car 3) for coordinated suppression

### 🚒 Car 3: Advanced Fire Suppression
- **File**: `firmware/car2_3_extinguish/car2_3_extinguish.ino` (set `#define UNIT_TYPE UNIT_CAR3`)
- **Role**: Precision fire suppression with dual servo control
- **Hardware**: 2 Servos + LEDC PWM pump control
  - Servo 1: Rotational scanning (left/right spray direction)
  - Servo 2: Vertical spray angle adjustment
- **Key Features**:
  - LEDC PWM for variable pump intensity
  - Dual servo enables 2D spray angle control
  - Automatic fire sequence: scan → lock target → adjust angle → spray
  - Higher accuracy fire suppression
  
**Deployment**: Primary suppression unit; use with Car 2 for redundancy

### 🆘 Car 4: Search & Rescue
- **File**: `firmware/car4_rescue/car4_rescue.ino`
- **Role**: Survivor location and rescue guidance
- **Hardware**: HC-SR04 Ultrasonic, 1 Servo, **DFPlayer Mini** audio system
- **Communication**: WiFi UDP with audio beacon
- **Key Features**:
  - DFPlayer Mini integration (UART2: GPIO16/17)
  - Automatic audio beacon every 10 seconds in AUTO mode
  - Plays 0001.mp3 from microSD card (44.1kHz, 128-192kbps recommended)
  - Non-blocking 3-point scan for survivor localization
  - Network commands: `audio_on`, `audio_off`, mode control
  
**Deployment**: Dispatch after fire detected; helps survivors self-locate via audio

## Architecture Overview

### Common Libraries (`firmware/common_ai/`)

**motor_control.h** - L298N Motor Driver Abstraction
```cpp
MotorController motor(pinIN1, pinIN2, pinIN3, pinIN4);
motor.forward();     // Move forward
motor.stop();        // Stop
motor.setSpeed(200); // PWM speed (0-255)
```

**sensor_control.h** - Ultrasonic & Flame Sensors
```cpp
UltrasonicSensor us(trigPin, echoPin);
FlameSensor flame(leftPin, rightPin);
int distance = us.getDistance();        // cm
bool fire = flame.isFlameDetected();    // global check
bool left_fire = flame.isLeftFlame();   // independent check
```

**pump_control.h** - Water Pump Relay Control
```cpp
PumpController pump(relayPin);
pump.start();           // Pump ON
pump.pulse(500);        // Pulse for 500ms
pump.stop();            // Pump OFF
```

**network_config.h** - Centralized Network Setup
```cpp
#define CAR_ID 1
#define CAR_IP IPAddress(192, 168, 1, 111)
#define LISTEN_PORT 5001
```

### Unified Control Protocol

**Command Format** (text, UDP):
```
forward / backward / left / right / stop / scan
mode_auto / mode_manual / mode_hold
pump_on / pump_off / audio_on / audio_off
```

**Telemetry Response** (100ms interval):
- Distance value (ultrasonic reading)
- Formatted: "DISTANCE:XX.X CM"

## Operating Modes

All units support 3 operating modes:

| Mode | Behavior | Control |
|------|----------|---------|
| **HOLD** | Motors OFF, Stop all activity | Default on startup |
| **MANUAL** | Remote control via WiFi commands | Each command processed |
| **AUTO** | Autonomous operation with flame priority | Servo sweep, continuous scanning |

Emergency interrupt: **Flame detection → immediate response** (regardless of mode)

## Safety & Constraints

⚠️ **Critical**:
1. **Flame Detection Priority**: Overrides all other operations
2. **Motor Safety**: Always test direction before deploying
3. **Servo Limits**: 0-180° with mechanical stop detection
4. **Pump Waterproofing**: Electronics must be isolated from spray
5. **WiFi Dependency**: Fallback to local control if AP unavailable

## Troubleshooting Guide

### WiFi Not Connecting
- Verify SSID: "SwarmRobotics_ECU" is broadcasting
- Check password: "ECU@2025"
- Inspect AP channel (suggest 6 or 11 for less interference)
- Verify car's IP doesn't conflict with other devices

### Motors Won't Move
- Check L298N power supply (12V minimum)
- Verify motor power connections (GND, +12V)
- Test individual pins with Serial.println(digitalRead(pin))
- Motor direction reversed? Swap IN1/IN2 pins

### Servo Jerking / Not Responding
- Verify servo power (5V+, 500mA capable PSU)
- Check control signal on correct GPIO
- Add 100µF capacitor near servo power
- Reduce servo speed in code (increase delays)

### No Ultrasonic Readings
- Verify HC-SR04 power (5V)
- Check TRIG/ECHO pin connections
- Ensure 4-7 plastic cap sensor (not damaged)
- Increase timeout in sensor code if needed

### Flame Sensors Always False
- Check sensor orientation (lens facing fire source)
- Verify GPIO pins in correct INPUT mode (not OUTPUT)
- Sensor needs direct flame IR; test with lighter
- Some sensors require external pull-up resistor (10kΩ to 5V)

### Car 4 Audio Not Playing
- Verify DFPlayer power (5V, ≥1A)
- Check TX/RX connections (GPIO16/17)
- Format microSD card as FAT32
- MP3 files must be named: 0001.mp3, 0002.mp3, etc.
- Place MP3 files in root directory (not in folders)
- Try different microSD brand (some have compatibility issues)

## Next Steps

1. **Verify Compilation**: Compile each .ino file in Arduino IDE
2. **Test Hardware**:
   - Upload Car 1 firmware
   - Verify WiFi connection (check IP in Serial Monitor)
   - Test motor movement with manual commands
   - Calibrate sensor detection
3. **Deploy Fleet**:
   - Upload all 4 units
   - Test network communication between units
   - Create central control station (listener on 192.168.1.100)
4. **Advanced Features** (future):
   - ESP32-CAM integration for fire detection
   - Obstacle avoidance with memory mapping
   - Swarm coordination protocol

## About This Project

**Developed by**: Youssef Mohamed Fekry  
**Platform**: ESP32-WROOM microcontroller swarm  
**Purpose**: Multi-agent autonomous fire detection, suppression, and rescue  
**License**: Educational/Research use.

## Support & Documentation

- **Firmware Guide**: See `firmware/README.md`
- **Car 1 Details**: See `firmware/car1_search/README.md`
- **Cars 2-3 Details**: See `firmware/car2_3_extinguish/README.md`
- **Car 4 Details**: See `firmware/car4_rescue/README.md`



![WhatsApp Image 2026-03-05 at 11 42 57 PM](https://github.com/user-attachments/assets/c5dfbe16-892d-4559-a245-4798f6bc9175)
