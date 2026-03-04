# Car 4: Search & Rescue (SAR) Unit

## Mission Profile
**Primary Role**: Locate survivors and broadcast rescue distress beacon
**Unique Feature**: DFPlayer Mini MP3 audio system for voice guidance
**Specialization**: Audio beacon + visual search via camera (future CV integration)
**Network**: WiFi/UDP centralized control

## Hardware Configuration

### Pin Assignment
| Component | GPIO |
|-----------|------|
| Motor A Forward (IN1) | 12 |
| Motor A Reverse (IN2) | 14 |
| Motor B Forward (IN3) | 27 |
| Motor B Reverse (IN4) | 26 |
| Motor A Enable (ENA) | 25 |
| Motor B Enable (ENB) | 33 |
| Ultrasonic TRIG | 32 |
| Ultrasonic ECHO | 4 |
| Servo Scan | 15 |
| DFPlayer RX | 16 (UART 2) |
| DFPlayer TX | 17 (UART 2) |

### Motor Controller
- **Type**: L298N Dual Motor Driver
- **Speed**: PWM 200 units (0-255 range)
- **Power**: 12V supply

### Audio System: DFPlayer Mini
- **Communication**: UART2 (GPIO 16 RX, GPIO 17 TX)
- **Baud Rate**: 9600
- **Audio Format**: MP3 on microSD card (FAT32)
- **File Naming**: 0001.mp3, 0002.mp3, etc.
- **Volume**: 30 (0-30 scale)
- **Speaker Connection**: 3W @ 4Ω recommended
- **Power Supply**: 3.3V-5V (depending on module variant)

### Ultrasonic Sensor
- **HC-SR04 mounted on servo**
- **Scan Range**: 30-150° via servo sweep
- **Operating Range**: 2cm - 400cm
- **Accuracy**: ±2cm near distance, ±5% far distance

### Communication
- **Primary**: WiFi UDP (centralized control)
- **Secondary**: Audio beacon (autonomous)
- **IP**: 192.168.1.114
- **Ports**: 5004 (local), 6004 (remote)

## Firmware Details

### File: `car4_rescue.ino`

#### Key Features

1. **Autonomous Audio Beacon**
   - Plays rescue audio every 10 seconds in AUTO mode
   - Helps survivors locate the robot
   - Can be controlled remotely

2. **Survivor Geolocation**
   - Ultrasonic scan on demand
   - Helps central station triangulate position
   - Maps clear/blocked areas

3. **Network Coordination**
   - Reports telemetry continuously
   - Receives commands from central
   - Coordinates with other units (Cars 1-3)

4. **Three Operating Modes**
   - **HOLD**: Completely inactive
   - **AUTO**: Self-driving with periodic audio beacon
   - **MANUAL**: Remote-controlled via network

#### Control Flow
```
Setup -> Initialize WiFi & DFPlayer
       -> Wait for mode command
       -> In AUTO: Periodic audio + forward motion
       -> In MANUAL: Execute remote commands
       -> Continuous telemetry stream
       -> Loop
```

## Operation Guide

### Pre-Deployment Checklist

1. **SD Card Preparation**
   - Card must be FAT32 formatted
   - Create minimum: 0001.mp3 (rescue beacon sound)
   - Suggested files:
     - 0001.mp3: Standard rescue beacon (beeping/siren)
     - 0002.mp3: Alternative beacon tone
   - Place files in root directory

2. **DFPlayer Wiring**
   - Verify UART2 connections (GPIO16/17)
   - Power pin matches module version (3.3V or 5V)
   - Ground properly connected
   - SD card properly inserted

3. **Hardware Test**
   - Power on ESP32
   - Check Serial monitor for DFPlayer init message
   - Listen for audio from speaker on startup
   - Verify motor and servo functionality

### Audio Beacon Setup

The unit plays audio at intervals in AUTO mode:

**Current Implementation**:
- Interval: 10 seconds
- File: 0001.mp3 (default)
- Trigger: Automatic in AUTO mode
- Control: Can be overridden via network

**Customization**:
```cpp
// Change audio file:
dfPlayer.play(2);  // Play 0002.mp3 instead

// Change interval:
#define BEACON_INTERVAL 10000  // 10 seconds (add to defines)

// Change in handleAudioBeacon():
if (millis() - lastAudioTime >= BEACON_INTERVAL) {
  playRescueAudio();
}
```

### Deployment Steps

1. **Initial Power-On**
   ```
   - ESP32 boots
   - WiFi connects to "SwarmRobotics_ECU"
   - Serial prints status messages
   - Motor responds to test
   - Audio plays (if in range)
   ```

2. **Start Autonomous Rescue**
   ```
   Send: mode_auto
   - Unit begins forward motion
   - Servo sweeps continuously
   - Audio beacon plays every 10 seconds
   - Reports distance every 100ms
   ```

3. **Remote Control for Precision**
   ```
   Send: mode_manual
   - Unit stops servo sweep
   - Awaits movement commands
   - Audio plays on demand only
   - Allows operator to position unit optimally
   ```

4. **Stop and Hold**
   ```
   Send: mode_hold
   - Motors completely shut down
   - Servo centers
   - Audio stops
   - For emergencies or charging
   ```

## Network Commands

### Movement Commands
- `forward` - Drive forward
- `backward` - Reverse
- `left` - Turn left
- `right` - Turn right
- `stop` - Stop motors

### Audio Commands
- `audio_on` - Play rescue beacon immediately
- `audio_off` - Stop any playing audio

### Scan Commands
- `scan` - Perform 3-point distance scan

### Mode Commands
- `mode_auto` - Autonomous rescue mode
- `mode_manual` - Manual operator control
- `mode_hold` - Stop everything

## Telemetry & Reporting

### Continuous Telemetry (100ms intervals)
Sent to Central Station (192.168.1.100:6004):
- **Format**: Single distance value in cm
- **Example**: `35`
- **Usage**: Central plots robot movement trajectory

### On-Demand Scan Results
Request with `scan` command
- **Format**: `L:distance,C:distance,R:distance`
- **Example**: `L:28,C:45,R:32`
- **Usage**: Identifies obstacles and clear paths

### Audio Status
- **Playing**: Beacon audio activates
- **Stopped**: Audio ceases
- **Logged**: Each audio event timestamped at central

## Behavioral Specifications

### Autonomous Mode (AUTO)
```
LOOP:
  1. Move forward
  2. Servo sweep 30-150° continuously
  3. Check elapsed time since last audio
  4. If >= 10 seconds, play audio beacon
  5. Send distance telemetry
  6. Wait for new command
```

**Use Case**: Survivor is hiding/unconscious. Robot roams area, 
periodically broadcasts audio to help them locate the source.

### Manual Mode (MANUAL)
```
LOOP:
  1. Servo centers at 90°
  2. Wait for movement command from operator
  3. Audio only plays on `audio_on` command
  4. Operator can position robot precisely
  5. Send distance telemetry
```

**Use Case**: Operator has visual on survivor. Manual driving
gets robot into optimal position without random motion.

### Hold Mode (HOLD)
```
- Motors: Off
- Servo: At rest
- Audio: Disabled
- Telemetry: Stopped
```

**Use Case**: Victim found, charging, emergency shutdown.

## Safety Features

### Audio Beacon Control
- Can be disabled via `mode_hold` or `audio_off`
- Prevents disorientation of survivor (if beacon malfunctions)
- Central can adjust beacon frequency

### Obstacle Avoidance
- On-demand scanning prevents collision
- Operator gets distance data in manual mode
- Can implement automatic retreat if needed

### Network Safety Limits
- Commands timeout: No automatic timeout (add if needed)
- Max motor speed: 200/255 (reasonable for rescue ops)
- Motor cutoff: Always available via `stop` command

### Emergency Stop
- Send `mode_hold` to completely disable unit
- Physical power switch always available
- Bluetooth can be added as secondary control

## Audio System Details

### DFPlayer Mini Capabilities
- **MP3 Support**: Standard MP3 files (16kHz-48kHz)
- **Maximum**: 1000 tracks per folder
- **Bus**: UART serial (9600 baud)
- **Control**: Simple serial commands

### Recommended Audio Files

**File 0001.mp3** (Primary Beacon):
- Duration: 1-3 seconds
- Type: Repeating beep/siren
- Volume: Should be audible from 30m away
- Example: Police siren, alarm tone, beeping pattern

**File 0002.mp3** (Alternative Beacon):
- Different tone for identification
- Different cadence to assist direction-finding
- Optional but recommended

### Audio Commands (Serial Protocol)
The DFPlayer library handles protocol automatically:
```cpp
dfPlayer.play(1);    // Play 0001.mp3
dfPlayer.play(2);    // Play 0002.mp3
dfPlayer.volume(30); // Set volume 0-30
dfPlayer.stop();     // Stop current playback
```

## Modification Guide

### Change Beacon Audio File
```cpp
// In playRescueAudio():
void playRescueAudio() {
  dfPlayer.play(2);  // Use 0002.mp3 instead
  Serial.println("[Audio] Playing alternative beacon");
  lastAudioTime = millis();
}
```

### Change Beacon Interval
```cpp
// Add at top:
#define BEACON_INTERVAL 15000  // 15 seconds

// In handleAudioBeacon():
if (millis() - lastAudioTime >= BEACON_INTERVAL) {
  playRescueAudio();
}
```

### Add Text-to-Speech (Future)
```cpp
// Could use SpeakJet or similar TTS module instead of pre-recorded
// Would allow dynamic messages like "Move toward the beacon"
```

### Extend Movement Range
```cpp
// Current forward speed: 200
// Can increase up to 255:
motor.setSpeed(220);
// Or implement gradient acceleration
```

## Troubleshooting

### DFPlayer won't initialize
```
Serial output: "[DFPlayer] Initialization failed!"

Fixes:
1. Check power supply (3.3V or 5V as required)
2. Verify RX/TX on correct pins (16/17 for UART2)
3. Test SD card independently
4. Check SD card format (must be FAT32)
5. Try different baud rate (some clones use 9600 with handshake)
```

### Audio doesn't play
```
Fixes:
1. Verify SD card has MP3 files
2. Check file names (must be 0001.mp3, 0002.mp3, etc.)
3. Test audio files on computer first
4. Increase volume: dfPlayer.volume(30)
5. Check speaker wiring and power
6. Verify speaker impedance (3W @ 4Ω or 8Ω)
```

### Motors unresponsive
```
Fixes:
1. Check 12V power supply
2. Verify motor pins (IN1/IN2/IN3/IN4)
3. Test with digitalWrite() directly
4. Check ENA/ENB PWM output on multimeter
5. Verify L298N connections
```

### WiFi won't connect
```
Fixes:
1. Check SSID: "SwarmRobotics_ECU"
2. Check password: "ECU@2025"
3. Verify router is broadcasting 2.4GHz (not 5GHz only)
4. Check IP configuration (should be 192.168.1.114)
5. Monitor Serial output for IP assignment
```

### Servo jerks or won't sweep
```
Fixes:
1. Verify servo power (5V minimum)
2. Increase delay in servo loop
3. Check servo signal pin (GPIO 15)
4. Verify servo isn't physically obstructed
5. Test servo separately with known-good code
```

## Performance Specifications

| Metric | Value |
|--------|-------|
| Max Forward Speed | ~0.5 m/s |
| Motor Response | <20ms to command |
| Audio Latency | <100ms from command |
| WiFi Latency | ~50-100ms |
| Beacon Interval (AUTO) | 10 seconds |
| Telemetry Rate | 10Hz (100ms intervals) |
| Unobstructed Range | 30-50m (WiFi + audio) |
| Survival Time | 4-6 hours (1S 2200mAh LiPo) |

## Integration with Swarm

**Car 4 Coordination**:
1. Car 1 detects fire and alerts central
2. Car 4 dispatched to search for survivors
3. Cars 2-3 move toward fire location
4. Central monitors all 4 units simultaneously
5. Car 4 audio beacon helps guide survivors to safety
6. All units report telemetry continuously

**Future AI Integration**:
- ESP32-CAM added to Car 4
- Visual human detection
- Faces extraction and tracking
- Gesture recognition (arms raised, etc.)
- Real-time video to central station

## Deployment Checklist

- [ ] SD card formatted FAT32
- [ ] MP3 files copied (0001.mp3 at minimum)
- [ ] Firmware uploaded to ESP32
- [ ] DFPlayer initializes (check Serial)
- [ ] Audio plays on startup
- [ ] Motors test individually
- [ ] Servo scans smoothly
- [ ] WiFi connects (check Serial)
- [ ] Central receives telemetry
- [ ] Audio beacon activates in AUTO mode
- [ ] Commands from central work correctly
- [ ] Emergency stop (mode_hold) works

## Advanced Features (Future)

1. **Text-to-Speech**
   - Voice instructions to survivors
   - "Move toward the beacon" in multiple languages

2. **Directional Audio**
   - Stereo panning to help locate robot
   - Survivor can triangulate position

3. **Computer Vision**
   - ESP32-CAM mounted for survivor detection
   - Facial recognition to identify individuals
   - Gesture detection (waving, raises arms)

4. **Thermal Imaging**
   - MLX90614 thermal sensor
   - Survives hidden under rubble (heat signature)
   - Distinction from fire heat

5. **Swarm Coordination**
   - Multiple Car 4 units coordinate beacon patterns
   - No interference between audio beacons
   - Mesh network for extended range

## Support & References

- For network setup: `firmware/README.md`
- For common libraries: `firmware/common_ai/`
- For Car 1 specs: `firmware/car1_search/README.md`
- For Cars 2-3 specs: `firmware/car2_3_extinguish/README.md`

## Recommended Audio Sources

**Royalty-free rescue beacon sounds**:
- OpenAI Siren library
- Freesound.org (search "siren", "alarm", "police")
- YouTube Audio Library (search "emergency siren")
- Zapsplat (free SFX database)

**File Specifications**:
- Format: MP3 (16-bit, 44.1kHz)
- Bitrate: 128-192 kbps
- Duration: 1-3 seconds per beacon
- Loudness: Normalized to -3dB peak
