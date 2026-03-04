// ============================================================
// CAR 4: SEARCH & RESCUE (SAR)
// FireGuard Swarm - Victim Location & Audio Beacon Unit
// ============================================================
// Mission: Locate survivors & broadcast audio distress beacons
// Features: DFPlayer Mini audio system + obstacle avoidance

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#include "DFRobotDFPlayerMini.h"

// Include common libraries
#include "../common_ai/network_config.h"
#include "../common_ai/motor_control.h"
#include "../common_ai/sensor_control.h"

// ============================================================
//                    HARDWARE CONFIGURATION
// ============================================================

// Motor pins
#define IN1 12
#define IN2 14
#define IN3 27
#define IN4 26
#define ENA 25
#define ENB 33

// Sensors
#define TRIG_PIN 32
#define ECHO_PIN 4
#define SERVO_PIN 15

// Audio (DFPlayer)
HardwareSerial dfSerial(2);  // UART 2 (GPIO16/17)

// ============================================================
//                      OBJECTS & INSTANCES
// ============================================================

MotorController motor(IN1, IN2, IN3, IN4, ENA, ENB, 200);
UltrasonicSensor ultrasonic(TRIG_PIN, ECHO_PIN);

Servo scanServo;
DFRobotDFPlayerMini dfPlayer;
WiFiUDP udp;

// ============================================================
//                       NETWORK SETUP
// ============================================================

uint8_t local_IP[] = UNIT_CAR4_IP;
const char* centralIP = CENTRAL_IP;
const int localUdpPort = UNIT_CAR4_LOCAL_PORT;
const int remoteUdpPort = UNIT_CAR4_REMOTE_PORT;

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
unsigned long lastAudioTime = 0;

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
  
  scanServo.attach(SERVO_PIN);
  scanServo.write(90);
  
  // Initialize DFPlayer
  dfSerial.begin(9600, SERIAL_8N1, 16, 17);
  delay(100);
  
  if (dfPlayer.begin(dfSerial)) {
    dfPlayer.setTimeOut(500);
    dfPlayer.volume(30);
    Serial.println("[DFPlayer] Initialized successfully");
  } else {
    Serial.println("[DFPlayer] Initialization failed!");
  }
  
  configureWiFi(local_IP, localUdpPort);
  udp.begin(localUdpPort);
  
  Serial.println("\n[Car4-SAR] System Ready - MODE: HOLD");
  Serial.println("[Car4-SAR] Audio beacon ready (plays every 10s in AUTO)");
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
    Serial.println("[Car4] Mode: AUTO");
  }
  else if (cmd == "mode_manual") {
    currentMode = MODE_MANUAL;
    motor.stop();
    Serial.println("[Car4] Mode: MANUAL");
  }
  else if (cmd == "mode_hold") {
    currentMode = MODE_HOLD;
    motor.stop();
    Serial.println("[Car4] Mode: HOLD");
  }
  // Movement commands
  else if (currentMode != MODE_HOLD) {
    lastCommand = cmd;
  }
  
  // Special commands
  if (cmd == "scan") {
    startScan();
  }
  else if (cmd == "audio_on") {
    playRescueAudio();
  }
  else if (cmd == "audio_off") {
    dfPlayer.stop();
  }
}

// ============================================================
//                    AUDIO FUNCTIONS
// ============================================================

void playRescueAudio() {
  dfPlayer.play(1);  // Play audio file 0001.mp3
  Serial.println("[Audio] Playing rescue beacon");
  lastAudioTime = millis();
}

void handleAudioBeacon() {
  // In AUTO mode, play beacon every 10 seconds
  if (currentMode == MODE_AUTO) {
    if (millis() - lastAudioTime >= 10000) {
      playRescueAudio();
    }
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
    Serial.println("[Car4] Starting 3-point scan...");
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
        Serial.print("[Scan] ");
        Serial.println(scanResult);
        
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
  handleAudioBeacon();
  
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
