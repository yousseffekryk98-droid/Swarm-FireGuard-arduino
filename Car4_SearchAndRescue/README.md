# Car 4: Search and Rescue Robot

## Project Overview
Autonomous search and rescue robot with audio alert system, obstacle detection, and WiFi-enabled remote control. Uses DFRobotDFPlayerMini for MP3 audio playback to signal presence and attract survivors.

## Features
- **Audio Alert System**: Plays rescue beacon sound every 10 seconds
- **Obstacle Avoidance**: HC-SR04 ultrasonic with directional servo scanning
- **DFPlayer Mini Integration**: MP3 audio module support for audio cues
- **WiFi/UDP Control**: Remote command control with non-blocking operations
- **Direction Scanning**: Left/Right scanning to find best navigation path
- **Real-time Telemetry**: Continuous distance reporting to central station

## Hardware Requirements
- ESP32 Development Board
- L298N Motor Driver
- HC-SR04 Ultrasonic Sensor
- SG90 Servo Motor
- DFRobotDFPlayerMini module
- Micro SD card with MP3 files (numbered 0001.mp3, 0002.mp3, etc.)
- Speaker (3W, 4Ω recommended)
- Motor wheels and chassis

## Pin Configuration (v1: Autonomous)
| Component | Pin |
|-----------|-----|
| Motor In1 | GPIO 12 |
| Motor In2 | GPIO 14 |
| Motor In3 | GPIO 27 |
| Motor In4 | GPIO 26 |
| Motor ENA | GPIO 25 |
| Motor ENB | GPIO 33 |
| Ultrasonic TRIG | GPIO 32 |
| Ultrasonic ECHO | GPIO 4 |
| Servo | GPIO 15 |
| DFPlayer RX | GPIO 16 (SoftwareSerial) |
| DFPlayer TX | GPIO 17 (SoftwareSerial) |

## Pin Configuration (v2: WiFi UDP)
| Component | Pin |
|-----------|-----|
| Motor In1 | GPIO 12 |
| Motor In2 | GPIO 14 |
| Motor In3 | GPIO 27 |
| Motor In4 | GPIO 26 |
| Motor ENA | GPIO 25 |
| Motor ENB | GPIO 33 |
| Ultrasonic TRIG | GPIO 32 |
| Ultrasonic ECHO | GPIO 4 |
| Servo | GPIO 15 |
| DFPlayer RX | GPIO 16 (HardwareSerial 2) |
| DFPlayer TX | GPIO 17 (HardwareSerial 2) |

## Code Versions

### v1: Autonomous (Car4_SearchAndRescue_v1_Autonomous.ino)
- Self-contained autonomous operation
- 10-second audio alert intervals (plays audio file 1)
- Obstacle detection at 30cm threshold
- Directional scanning (150° left, 30° right)
- Simple forward/turn/stop pattern
- SoftwareSerial for DFPlayer communication
- No network connectivity

### v2: WiFi UDP (Car4_SearchAndRescue_v2_WiFiUDP.ino)
- Full WiFi UDP command protocol
- Three operational modes: HOLD, AUTO, MANUAL
- HardwareSerial 2 for DFPlayer (better reliability)
- Non-blocking 3-point distance scan
- Remote rescue audio command capability
- Real-time telemetry transmission
- Swarm-ready architecture

## Network Configuration (v2)
```
WiFi SSID: SwarmRobotics_ECU
Password: ECU@2025
Local IP: 192.168.1.114
Local UDP Port: 5004
Remote IP: 192.168.1.100
Remote UDP Port: 6004
```

## Control Commands (UDP - v2)
- `forward` - Move forward
- `backward` - Move backward
- `left` - Turn left
- `right` - Turn right
- `stop` - Stop motors
- `rescue` - Play rescue audio (audio file 1)
- `mode_auto` - Switch to automatic mode
- `mode_manual` - Switch to manual mode
- `mode_hold` - Switch to hold mode
- `scan` - Perform 3-point distance scan

## DFPlayer Configuration
- Baud Rate: 9600
- Volume Level (v1): 25
- Volume Level (v2): 30
- Default Audio File: 1 (0001.mp3)
- Format: MP3 on FAT32 microSD

## SD Card Structure
```
microSD/
├── 0001.mp3  (Rescue beacon sound)
├── 0002.mp3  (Optional)
└── ...
```

## Behavior Notes
- **v1**: Plays audio every 10 seconds continuously
- **v2**: Audio plays on command or can be programmed externally
- Obstacle distance threshold: 30cm
- Direction preference: Selects clearest path (left vs right)
- Turn duration: 600ms
- Forward default speed: 200 units

## Audio File Naming Convention
DFPlayer expects MP3 files named as:
- `0001.mp3` (File 1)
- `0002.mp3` (File 2)
- etc.

Place all MP3s in the root directory of microSD card formatted as FAT32.

## Integration with Central Control
- v2 communicates on UDP port 5004 (local) / 6004 (remote)
- Sends distance readings every 100ms
- Can be commanded remotely by control station at 192.168.1.100
- Performs full scan on demand via "scan" command
- Audio alerts can be triggered from central station

## Troubleshooting
- **No audio**: Check DFPlayer power, SD card format, file naming
- **Wrong commands**: Verify UDP payload strings match exactly
- **Audio volume**: Adjust with `player.volume(x)` (0-30)
- **Serial conflicts**: v2 uses UART2, ensure no pin conflicts

## Advanced Features
- Non-blocking audio playback (DFPlayer handles timing)
- Servo remains centered except during obstacle scan
- Telemetry helps central station track robot location via distance changes
- Can be integrated with swarm coordination algorithm
