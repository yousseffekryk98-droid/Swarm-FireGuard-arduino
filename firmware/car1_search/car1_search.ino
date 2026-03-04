// ============================================================
// CAR 1: SEARCH & FIRE DETECTION
// FireGuard Swarm - Reconnaissance Unit (WiFi UDP Version)
// ============================================================
// Primary Mission: High-sensitivity scanning for fire & obstacles
// Includes Bluetooth override for emergency manual control

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include "BluetoothSerial.h"

// Include common libraries
#include "../common_ai/network_config.h"
#include "../common_ai/motor_control.h"
#include "../common_ai/sensor_control.h"

// ============================================================
//                    HARDWARE CONFIGURATION
// ============================================================

// Motor pins (L298N)
#define IN1 14
#define IN2 27
#define IN3 12
#define IN4 13
#define ENA 32
#define ENB 33

// Sensors
#define TRIG_PIN 26
#define ECHO_PIN 25
#define FLAME_LEFT 34
#define FLAME_RIGHT 35
#define SERVO_PIN 18

// ============================================================
//                      OBJECTS & INSTANCES
// ============================================================

MotorController motor(IN1, IN2, IN3, IN4, ENA, ENB, 200);
UltrasonicSensor ultrasonic(TRIG_PIN, ECHO_PIN);
FlameSensor flameSensor(FLAME_LEFT, FLAME_RIGHT);

Servo scanServo;
BluetoothSerial SerialBT;
WiFiUDP udp;

// ============================================================
//                       NETWORK SETUP
// ============================================================

uint8_t local_IP[] = UNIT_CAR1_IP;
const char* centralIP = CENTRAL_IP;
const int localUdpPort = UNIT_CAR1_LOCAL_PORT;
const int remoteUdpPort = UNIT_CAR1_REMOTE_PORT;

char packetBuffer[255];

// ============================================================
//                     OPERATION MODES & STATE
// ============================================================

#define MODE_HOLD 0
#define MODE_AUTO 1
#define MODE_MANUAL 2

int currentMode = MODE_HOLD;
String lastCommand = "stop";
int servoPos = 90;
int servoDir = 1;
unsigned long lastServoMove = 0;

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
  SerialBT.begin("FireGuard_Car1");
  
  motor.init();
  
  scanServo.attach(SERVO_PIN);
  scanServo.write(90);
  
  configureWiFi(local_IP, localUdpPort);
  udp.begin(localUdpPort);
  
  Serial.println("\n[Car1] System initialized - MODE: HOLD");
  Serial.println("[Car1] Ready for network commands (WiFi + Bluetooth)");
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
  
  Serial.print("[Net] Command: ");
  Serial.println(cmd);
  
  // Mode switching
  if (cmd == "mode_auto") {
    currentMode = MODE_AUTO;
    lastCommand = "forward";
    Serial.println("[Car1] Mode: AUTO");
  }
  else if (cmd == "mode_manual") {
    currentMode = MODE_MANUAL;
    motor.stop();
    Serial.println("[Car1] Mode: MANUAL");
  }
  else if (cmd == "mode_hold") {
    currentMode = MODE_HOLD;
    motor.stop();
    Serial.println("[Car1] Mode: HOLD");
  }
  // Movement commands
  else if (currentMode != MODE_HOLD) {
    lastCommand = cmd;
  }
  
  // Scan request
  if (cmd == "scan") {
    startScan();
  }
}

// ============================================================
//                    SCAN OPERATIONS
// ============================================================

void startScan() {
  if (currentScanState == NOT_SCANNING) {
    currentScanState = SCAN_LEFT;
    scanResult = "";
    motor.stop();
    Serial.println("[Car1] Starting 3-point scan...");
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
        scanServo.write(150);
        delay(200);
        d = ultrasonic.getDistance();
        scanResult += "L:" + String(d) + ",";
        currentScanState = SCAN_CENTER;
        break;
        
      case SCAN_CENTER:
        scanServo.write(90);
        delay(200);
        d = ultrasonic.getDistance();
        scanResult += "C:" + String(d) + ",";
        currentScanState = SCAN_RIGHT;
        break;
        
      case SCAN_RIGHT:
        scanServo.write(30);
        delay(200);
        d = ultrasonic.getDistance();
        scanResult += "R:" + String(d);
        currentScanState = SCAN_COMPLETE;
        break;
        
      case SCAN_COMPLETE:
        scanServo.write(90);
        Serial.print("[Scan Result] ");
        Serial.println(scanResult);
        
        // Send result to central
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
  if (currentMode == MODE_HOLD || currentMode == MODE_MANUAL) {
    if (servoPos != 90) {
      servoPos = 90;
      scanServo.write(servoPos);
    }
    return;
  }
  
  // Auto sweep
  if (millis() - lastServoMove > 20) {
    servoPos += servoDir * 2;
    if (servoPos >= 150) servoDir = -1;
    if (servoPos <= 30) servoDir = 1;
    scanServo.write(servoPos);
    lastServoMove = millis();
  }
}

// ============================================================
//                      MAIN LOOP
// ============================================================

void loop() {
  handleServo();
  performScan();
  handleNetworkCommand();
  
  // FLAME DETECTION - HIGHEST PRIORITY
  if (flameSensor.isFlameDetected()) {
    motor.stop();
    Serial.println("[ALERT] FIRE DETECTED! Emergency stop activated.");
    
    // Send alert to central
    udp.beginPacket(centralIP, remoteUdpPort);
    udp.print("FIRE_ALERT_CAR1");
    udp.endPacket();
    
    delay(1000);
    return;
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
