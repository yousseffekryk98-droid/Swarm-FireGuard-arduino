// ============================================================
// CARS 2 & 3: FIRE EXTINGUISHING UNITS
// FireGuard Swarm - Active Suppression Platform
// ============================================================
// Mission: Detect & suppress active fires
// Includes dual pump systems and advanced spray control
// Configurable for Car 2 (standard) or Car 3 (advanced)

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

// Include common libraries
#include "../common_ai/network_config.h"
#include "../common_ai/motor_control.h"
#include "../common_ai/sensor_control.h"
#include "../common_ai/pump_control.h"

// ============================================================
//                    UNIT CONFIGURATION
// ============================================================
// Set UNIT_TYPE to either UNIT_CAR2 or UNIT_CAR3

#define UNIT_CAR2 2
#define UNIT_CAR3 3
#define UNIT_TYPE UNIT_CAR2  // Change to UNIT_CAR3 for advanced unit

#if UNIT_TYPE == UNIT_CAR2
  #define UNIT_NAME "FireGuard_Car2"
  uint8_t local_IP[] = UNIT_CAR2_IP;
  const int localUdpPort = UNIT_CAR2_LOCAL_PORT;
  const int remoteUdpPort = UNIT_CAR2_REMOTE_PORT;
  
  // Car 2 pins (standard)
  #define IN1 25
  #define IN2 26
  #define IN3 14
  #define IN4 13
  #define ENA 27
  #define ENB 16
  #define TRIG_PIN 18
  #define ECHO_PIN 19
  #define SERVO_PIN 15
  #define FLAME_LEFT 32
  #define FLAME_RIGHT 33
  #define PUMP_PIN 4
  #define MOTOR_SPEED 150
  
#elif UNIT_TYPE == UNIT_CAR3
  #define UNIT_NAME "FireGuard_Car3"
  uint8_t local_IP[] = UNIT_CAR3_IP;
  const int localUdpPort = UNIT_CAR3_LOCAL_PORT;
  const int remoteUdpPort = UNIT_CAR3_REMOTE_PORT;
  
  // Car 3 pins (advanced with dual servos + LEDC)
  #define IN1 18
  #define IN2 5
  #define IN3 17
  #define IN4 16
  #define ENA 19
  #define ENB 4
  #define TRIG_PIN 27
  #define ECHO_PIN 26
  #define SENSOR_SERVO_PIN 33
  #define SPRAY_SERVO_PIN 32
  #define FLAME_LEFT 13
  #define FLAME_RIGHT 15
  #define PUMP_PIN 25
  #define MOTOR_SPEED 200
#endif

// ============================================================
//                      OBJECTS & INSTANCES
// ============================================================

MotorController motor(IN1, IN2, IN3, IN4, ENA, ENB, MOTOR_SPEED);
UltrasonicSensor ultrasonic(TRIG_PIN, ECHO_PIN);
FlameSensor flameSensor(FLAME_LEFT, FLAME_RIGHT);
PumpController pump(PUMP_PIN);

#if UNIT_TYPE == UNIT_CAR3
  Servo sensorServo;
  Servo sprayServo;
  const int FIRE_SPRAY_ANGLE = 45;
  const int IDLE_SPRAY_ANGLE = 90;
#else
  Servo scanServo;
#endif

WiFiUDP udp;
const char* centralIP = CENTRAL_IP;
char packetBuffer[255];

// ============================================================
//                     OPERATION MODES & STATE
// ============================================================

#define MODE_HOLD 0
#define MODE_AUTO 1
#define MODE_MANUAL 2

int currentMode = MODE_HOLD;
String lastCommand = "stop";
bool fireWasDetected = false;

#if UNIT_TYPE == UNIT_CAR2
  int servoPos = 90;
  int servoDir = 1;
  unsigned long lastServoMove = 0;
#else
  int sensorServoPos = 90;
  int sensorServoDir = 1;
  unsigned long lastServoMove = 0;
  int currentSpeed = MOTOR_SPEED;
#endif

// Scan state machine
enum ScanState { NOT_SCANNING, SCAN_LEFT, SCAN_CENTER, SCAN_RIGHT, SCAN_COMPLETE };
ScanState currentScanState = NOT_SCANNING;
unsigned long lastScanTime = 0;
String scanResult = "";

// ============================================================
//                         SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);
  
  motor.init();
  
  #if UNIT_TYPE == UNIT_CAR2
    scanServo.attach(SERVO_PIN);
    scanServo.write(90);
  #else
    sensorServo.attach(SENSOR_SERVO_PIN);
    sensorServo.write(90);
    sprayServo.attach(SPRAY_SERVO_PIN);
    sprayServo.write(IDLE_SPRAY_ANGLE);
  #endif
  
  configureWiFi(local_IP, localUdpPort);
  udp.begin(localUdpPort);
  
  Serial.print("\n[");
  Serial.print(UNIT_NAME);
  Serial.println("] System initialized - MODE: HOLD");
}

// ============================================================
//                    NETWORK COMMAND HANDLER
// ============================================================

void handleNetworkCommand() {
  int packetSize = udp.parsePacket();
  if (packetSize == 0) return;
  
  int len = udp.read(packetBuffer, 255);
  if (len > 0) packetBuffer[len] = 0;
  
  String cmd = String(packetBuffer);
  cmd.trim();
  cmd.toLowerCase();
  
  // Mode switching
  if (cmd == "mode_auto") {
    currentMode = MODE_AUTO;
    lastCommand = "forward";
  }
  else if (cmd == "mode_manual") {
    currentMode = MODE_MANUAL;
    motor.stop();
  }
  else if (cmd == "mode_hold") {
    currentMode = MODE_HOLD;
    motor.stop();
  }
  else if (currentMode != MODE_HOLD) {
    lastCommand = cmd;
  }
  
  // Special commands
  if (cmd == "scan") startScan();
  else if (cmd == "pump_on") pump.start();
  else if (cmd == "pump_off") pump.stop();
}

// ============================================================
//                    SCAN OPERATIONS
// ============================================================

void startScan() {
  if (currentScanState == NOT_SCANNING) {
    currentScanState = SCAN_LEFT;
    scanResult = "";
    motor.stop();
  }
}

void performScan() {
  if (currentScanState == NOT_SCANNING) return;
  
  unsigned long currentTime = millis();
  if (currentTime - lastScanTime > 500) {
    lastScanTime = currentTime;
    long d;
    
    switch (currentScanState) {
      case SCAN_LEFT:
        #if UNIT_TYPE == UNIT_CAR2
          scanServo.write(150);
        #else
          sensorServo.write(150);
        #endif
        delay(200);
        d = ultrasonic.getDistance();
        scanResult += "L:" + String(d) + ",";
        currentScanState = SCAN_CENTER;
        break;
        
      case SCAN_CENTER:
        #if UNIT_TYPE == UNIT_CAR2
          scanServo.write(90);
        #else
          sensorServo.write(90);
        #endif
        delay(200);
        d = ultrasonic.getDistance();
        scanResult += "C:" + String(d) + ",";
        currentScanState = SCAN_RIGHT;
        break;
        
      case SCAN_RIGHT:
        #if UNIT_TYPE == UNIT_CAR2
          scanServo.write(30);
        #else
          sensorServo.write(30);
        #endif
        delay(200);
        d = ultrasonic.getDistance();
        scanResult += "R:" + String(d);
        currentScanState = SCAN_COMPLETE;
        break;
        
      case SCAN_COMPLETE:
        #if UNIT_TYPE == UNIT_CAR2
          scanServo.write(90);
        #else
          sensorServo.write(90);
        #endif
        
        udp.beginPacket(centralIP, remoteUdpPort);
        udp.print(scanResult);
        udp.endPacket();
        currentScanState = NOT_SCANNING;
        break;
    }
  }
}

// ============================================================
//                    SERVO MANAGEMENT
// ============================================================

void handleServo() {
  #if UNIT_TYPE == UNIT_CAR2
    if (currentMode == MODE_HOLD || currentMode == MODE_MANUAL) {
      if (servoPos != 90) {
        servoPos = 90;
        scanServo.write(servoPos);
      }
      return;
    }
    
    if (millis() - lastServoMove > 20) {
      servoPos += servoDir * 2;
      if (servoPos >= 150) servoDir = -1;
      if (servoPos <= 30) servoDir = 1;
      scanServo.write(servoPos);
      lastServoMove = millis();
    }
  #else
    // Car 3: Advanced servo control
    if (currentMode == MODE_HOLD || currentMode == MODE_MANUAL) {
      if (sensorServoPos != 90) {
        sensorServoPos = 90;
        sensorServo.write(sensorServoPos);
      }
      return;
    }
    
    if (millis() - lastServoMove > 20) {
      sensorServoPos += sensorServoDir * 2;
      if (sensorServoPos >= 150) sensorServoDir = -1;
      if (sensorServoPos <= 30) sensorServoDir = 1;
      sensorServo.write(sensorServoPos);
      lastServoMove = millis();
    }
  #endif
}

// ============================================================
//                      MAIN LOOP
// ============================================================

void loop() {
  handleServo();
  performScan();
  handleNetworkCommand();
  pump.update();  // Pump pulse timeout handling
  
  // FLAME DETECTION - EMERGENCY PRIORITY
  if (flameSensor.isFlameDetected()) {
    motor.stop();
    pump.start();
    
    #if UNIT_TYPE == UNIT_CAR3
      sprayServo.write(FIRE_SPRAY_ANGLE);
    #endif
    
    Serial.println("[FIRE] DETECTED - Suppression activated!");
    
    udp.beginPacket(centralIP, remoteUdpPort);
    udp.print("FIRE_DETECTED");
    udp.endPacket();
    
    fireWasDetected = true;
    delay(2000);
    
    pump.stop();
    #if UNIT_TYPE == UNIT_CAR3
      sprayServo.write(IDLE_SPRAY_ANGLE);
    #endif
    
    // Back away and turn
    motor.backward();
    delay(1000);
    motor.left();
    delay(1000);
    motor.stop();
    
    fireWasDetected = false;
  }
  
  // MODE-BASED EXECUTION
  if (currentMode != MODE_HOLD) {
    if (lastCommand == "forward") motor.forward();
    else if (lastCommand == "backward") motor.backward();
    else if (lastCommand == "left") motor.left();
    else if (lastCommand == "right") motor.right();
    else if (lastCommand == "stop") motor.stop();
  }
  
  // TELEMETRY (every 100ms)
  static unsigned long lastTelemetry = 0;
  if (millis() - lastTelemetry > 100) {
    long distance = ultrasonic.getDistance();
    udp.beginPacket(centralIP, remoteUdpPort);
    udp.print(distance);
    udp.endPacket();
    lastTelemetry = millis();
  }
  
  delay(20);
}
