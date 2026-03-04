# Cars 2 & 3: Fire Extinguishing Units

## Mission Profile

### Car 2: Standard Fire Extinguisher
**Primary Role**: Active fire suppression with standard setup
**Water System**: Single pump with basic spray control
**Specialization**: Cost-effective, proven reliability

### Car 3: Advanced Fire Extinguisher  
**Primary Role**: Heavy-duty fire suppression with enhanced control
**Water System**: Dual servo (scanning + spray angle control)
**Specialization**: Precision spray targeting, LEDC PWM for smooth control

## Firmware Configuration

**File**: `car2_3_extinguish.ino` (unified, configurable)

### Selecting Your Unit

At the top of the sketch, set:
```cpp
#define UNIT_TYPE UNIT_CAR2   // For Car 2
// OR
#define UNIT_TYPE UNIT_CAR3   // For Car 3
```

The firmware automatically configures pins, IP address, and behavior based on this setting.

## Car 2: Standard Setup

### Pin Assignment
| Component | GPIO |
|-----------|------|
| Motor A Forward (IN1) | 25 |
| Motor A Reverse (IN2) | 26 |
| Motor B Forward (IN3) | 14 |
| Motor B Reverse (IN4) | 13 |
| Motor A Enable (ENA) | 27 |
| Motor B Enable (ENB) | 16 |
| Ultrasonic TRIG | 18 |
| Ultrasonic ECHO | 19 |
| Servo Scan | 15 |
| Flame Sensor Left | 32 |
| Flame Sensor Right | 33 |
| Water Pump Relay | 4 |

### Motor Controller
- **Type**: L298N Dual Motor Driver
- **Speed**: PWM 150 units (0-255 range)
- **Power**: 12V supply

### Water System
- **Pump Control**: GPIO 4 digital relay output
- **Pressure**: Standard municipal/pump pressure
- **Flow Rate**: ~2-5 L/min (typical)

### Communication
- **WiFi UDP**: Central control
- **IP**: 192.168.1.112
- **Ports**: 5002 (local), 6002 (remote)

## Car 3: Advanced Setup

### Pin Assignment
| Component | GPIO |
|-----------|------|
| Motor A Forward (IN1) | 18 |
| Motor A Reverse (IN2) | 5 |
| Motor B Forward (IN3) | 17 |
| Motor B Reverse (IN4) | 16 |
| Motor A Enable (ENA) | 19 (LEDC) |
| Motor B Enable (ENB) | 4 (LEDC) |
| Ultrasonic TRIG | 27 |
| Ultrasonic ECHO | 26 |
| Sensor Servo | 33 |
| Spray Servo | 32 |
| Flame Sensor Left | 13 |
| Flame Sensor Right | 15 |
| Water Pump Relay | 25 |

### Motor Controller
- **Type**: L298N Dual Motor Driver
- **PWM**: LEDC channels 0 & 1 (5kHz, 8-bit precision)
- **Speed**: PWM 200 units
- **Power**: 12V supply

### Water System
- **Pump Control**: GPIO 25 digital relay output
- **Spray Servo**: GPIO 32 for spray angle control
  - 45°: Fire suppression angle
  - 90°: Idle position
- **Pressure**: Standard to high pressure capable

### Servo Configuration
Two independent servos:
1. **Sensor Servo** (GPIO 33): Continuous 30-150° sweep for scanning
2. **Spray Servo** (GPIO 32): Directional spray control

### Communication
- **WiFi UDP**: Central control
- **IP**: 192.168.1.113
- **Ports**: 5003 (local), 6003 (remote)

## Operating Modes

Both units support three modes:

### HOLD Mode
- Motors: OFF
- Servo: Centered (90°)
- Pump: OFF
- Use: Standby, charging, manual control disabled

### MANUAL Mode
- Motors: Remote-controlled via UDP
- Servo: Centered (90°)
- Pump: Controllable via `pump_on` / `pump_off` commands
- Use: Operator-directed suppression

### AUTO Mode
- Motors: Autonomous forward movement
- Servo: Continuous sweep (scanning)
- Pump: Auto-triggers on flame detection
- Use: Autonomous fire hunting and suppression

## Network Commands

### Movement Commands
- `forward` - Drive forward
- `backward` - Reverse
- `left` - Pivot left
- `right` - Pivot right
- `stop` - Stop motors

### Pump Commands
- `pump_on` - Activate water pump
- `pump_off` - Deactivate water pump

### Mode Commands
- `mode_auto` - Autonomous operation
- `mode_manual` - Manual remote control
- `mode_hold` - Complete stop/hold

### Sensor Commands
- `scan` - Perform 3-point distance scan (L/C/R)

## Emergency Response: Automatic Fire Suppression

When flame is detected via dual flame sensors:

1. **Immediate Actions**:
   - Motor: Full stop
   - Servo: Centers immediately
   - Pump: Activates (2-second burst)
   - Spray Servo (Car 3): Points to 45° (attack angle)

2. **Central Alert**:
   - Message sent: `FIRE_DETECTED`
   - Timestamp: Recorded
   - Location: Based on telemetry

3. **Recovery Sequence**:
   - Pump stops after set duration
   - Unit backs away 1 second
   - Turns left 1 second
   - Returns to centering

4. **Resume**:
   - Waits for next command
   - Resumes previous mode if not overridden

## Telemetry & Reporting

### Continuous Telemetry
Sent every 100ms to central (192.168.1.100:600X):
- **Distance Reading**: Current ultrasonic distance in cm
- **Format**: Single integer (e.g., "35")

### On-Demand Scan Results
Format: `L:distance,C:distance,R:distance`
Example: `L:25,C:40,R:22`

### Emergency Alerts
- `FIRE_DETECTED`: Flame sensors activated

## Behavior Specifications

### Autonomous Mode (AUTO)
```
1. Begin forward movement
2. Servo sweeps continuously 30-150°
3. Distance data reported every 100ms
4. On flame detection:
   a. Motors stop
   b. Pump activates for 2 seconds
   c. Alert sent to central
   d. Unit backs away and turns
   e. Returns to forward motion
```

### Manual Mode (MANUAL)
```
1. Servo centers at 90°
2. Awaits UDP commands
3. Pump controlled independently
4. Enhanced operator control
5. Flame detection still active (safety)
```

### Obstacle Avoidance (On Scan Request)
```
1. Motor stops
2. Servo scans: 150° (left), 90° (center), 30° (right)
3. Reports all three distances
4. Central decides next move
5. Waits for movement command
```

## Safety Features

### Flame Detection Priority
- **Override Level**: Highest (stops all other actions)
- **Response Time**: <50ms
- **Redundancy**: Dual sensors (left + right)
- **Logic**: Active LOW (sensor pulls LOW when flame detected)

### Electrical Safety
- **Pump Relay**: Prevents backflow when power lost
- **Servo Power**: Isolated from motor supply
- **Fusing**: Recommended on each power line

### Operational Limits
- **Motor Speed**: 0-255 PWM (150-200 typical)
- **Servo Range**: 30-150° (safe mechanical limits)
- **Pump Duration**: 2-5 seconds (adjustable)
- **Idle Timeout**: None (add in production)

## Modification Guide

### Car 2: Standard Configuration

#### Adjust Pump Duration
```cpp
// In flame detection section:
pump.start();
delay(2000);  // Change 2000 to desired milliseconds
pump.stop();
```

#### Change Motor Speed
```cpp
MotorController motor(IN1, IN2, IN3, IN4, ENA, ENB, 150);
                                                       ^^^
                                                    Change here
```

### Car 3: Advanced Configuration

#### Change Spray Attack Angle
```cpp
#define FIRE_SPRAY_ANGLE 45      // Degrees (adjust as needed)
#define IDLE_SPRAY_ANGLE 90      // Neutral position

// In fire response:
sprayServo.write(FIRE_SPRAY_ANGLE);  // When suppressing
sprayServo.write(IDLE_SPRAY_ANGLE);  // When idle
```

#### Adjust Motor Sweep Speed
```cpp
if (millis() - lastServoMove > 20) {  // Increase for slower sweep
  sensorServoPos += sensorServoDir * 2;  // Increase multiplier for faster
```

#### Add Soft Deceleration (Car 3)
The firmware includes `softStop()` function - uncomment to use:
```cpp
// Before stopping in emergency:
// softStop(currentSpeed);
// motor.stop();
```

## Troubleshooting

### Pump won't activate
1. Check relay power supply (12V)
2. Test GPIO pin directly: `digitalWrite(PUMP_PIN, HIGH)`
3. Verify relay connections
4. Check for blown fuse

### Servo jerks or won't move
1. Verify servo power (5V minimum)
2. Increase delay after `servo.write()`
3. Reduce sweep speed (increase delay value)
4. Check signal pin routing (verify no interference)

### Motors unresponsive
1. Check 12V power supply
2. Verify L298N connections
3. Test IN pins with `digitalWrite()` individually
4. Check ENA/ENB PWM output

### Flame sensor false triggers
1. Check sensor calibration light level
2. Isolated sensor with multimeter
3. Verify wiring
4. Adjust sensitivity threshold with `if (digitalRead(...) == LOW)`

### WiFi connection issues
1. Verify WiFi SSID: "SwarmRobotics_ECU"
2. Check password: "ECU@2025"
3. Monitor IP assignment (should be static)
4. Check for IP conflicts on network

## Performance Specifications

| Metric | Car 2 | Car 3 |
|--------|-------|-------|
| Max Speed | 0.4 m/s | 0.5 m/s |
| Servo Sweep | 30-150° | Dual servos |
| Pump Response | <100ms | <100ms |
| WiFi Latency | ~50ms | ~50ms |
| Command Response | <20ms | <20ms |
| Flame Detection | <50ms | <50ms |

## Deployment Checklist

- [ ] Firmware uploaded (UNIT_TYPE set correctly)
- [ ] WiFi connects (check Serial monitor)
- [ ] Motors tested individually
- [ ] Pump tests (dry run at least)
- [ ] Ultrasonic reads distance correctly
- [ ] Flame sensors respond to heat source
- [ ] Servo sweeps smoothly without obstructions
- [ ] Central station can receive telemetry
- [ ] Commands from central move unit correctly
- [ ] Fire detection triggers automatic suppression

## Advanced Features (Future)

1. **Pressure Monitoring**
   - Analog sensor on pump outlet
   - Adjusts motor speed based on backpressure

2. **Temperature Sensing**
   - Thermal sensor for fire intensity
   - Adjusts suppression duration based on heat

3. **Spray Pattern Control**
   - Multiple nozzle patterns
   - Selectable via central station

4. **Collision Avoidance**
   - Automatic retreat on obstacle
   - Prevents getting stuck

5. **Swarm Coordination**
   - Hand-off between units
   - Collaborative suppression
   - Avoidance of overlapping spray

## Support

For network setup: `firmware/README.md`
For common libraries: `firmware/common_ai/`
For Car 1 integration: `firmware/car1_search/README.md`
For Car 4 integration: `firmware/car4_rescue/README.md`
