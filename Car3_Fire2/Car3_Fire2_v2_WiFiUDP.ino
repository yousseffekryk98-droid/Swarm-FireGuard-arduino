#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

// ================= NETWORK CONFIG =================
const char* ssid     = "SwarmRobotics_ECU";
const char* password = "ECU@2025";

IPAddress local_IP(192, 168, 1, 113);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const int localUdpPort = 5003; 
const char* pcIp       = "192.168.1.100"; 
const int remoteUdpPort = 6003;

WiFiUDP udp;
char packetBuffer[255];

// ================= HARDWARE PINS =================
const int motorAIn1 = 25;   
const int motorAIn2 = 26;   
const int motorBIn3 = 14;   
const int motorBIn4 = 13;   
const int motorAEnable = 27;  
const int motorBEnable = 16;  

const int pumpPin = 4;
const int flameSensor1 = 32;
const int flameSensor2 = 33;

#define TRIG_PIN 18
#define ECHO_PIN 19
#define SERVO_PIN 15

// Modes
#define MODE_HOLD 0
#define MODE_AUTO 1
#define MODE_MANUAL 2

const int SPEED = 150;
Servo scanServo;
int currentMode = MODE_HOLD;
String lastCommand = "stop";
int servoPos = 90;
int servoDir = 1;
unsigned long lastServoMove = 0;

// Variables for non-blocking scan
enum ScanState { NOT_SCANNING, SCAN_LEFT, SCAN_CENTER, SCAN_RIGHT, SCAN_COMPLETE };
ScanState currentScanState = NOT_SCANNING;
unsigned long lastScanTime = 0;
String scanResult = "";

void startScan() {
  if (currentScanState == NOT_SCANNING) {
    currentScanState = SCAN_LEFT;
    scanResult = "";
    stopMotors();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(motorAIn1, OUTPUT); pinMode(motorAIn2, OUTPUT);
  pinMode(motorBIn3, OUTPUT); pinMode(motorBIn4, OUTPUT);
  pinMode(motorAEnable, OUTPUT); pinMode(motorBEnable, OUTPUT);
  
  pinMode(pumpPin, OUTPUT); digitalWrite(pumpPin, LOW);
  pinMode(flameSensor1, INPUT_PULLUP);
  pinMode(flameSensor2, INPUT_PULLUP);
  
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  
  scanServo.attach(SERVO_PIN);
  scanServo.write(90);

  if (!WiFi.config(local_IP, gateway, subnet)) Serial.println("Config Failed");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(100);
    Serial.print(".");
   }
  
  udp.begin(localUdpPort);
}

void moveForward() {
  digitalWrite(motorAIn1, HIGH); digitalWrite(motorAIn2, LOW);
  digitalWrite(motorBIn3, HIGH); digitalWrite(motorBIn4, LOW);
  analogWrite(motorAEnable, SPEED); analogWrite(motorBEnable, SPEED);
}
void moveBackward() {
  digitalWrite(motorAIn1, LOW); digitalWrite(motorAIn2, HIGH);
  digitalWrite(motorBIn3, LOW); digitalWrite(motorBIn4, HIGH);
  analogWrite(motorAEnable, SPEED); analogWrite(motorBEnable, SPEED);
}
void turnLeft() {
  digitalWrite(motorAIn1, LOW); digitalWrite(motorAIn2, HIGH); 
  digitalWrite(motorBIn3, HIGH); digitalWrite(motorBIn4, LOW); 
  analogWrite(motorAEnable, SPEED); analogWrite(motorBEnable, SPEED);
}
void turnRight() {
  digitalWrite(motorAIn1, HIGH); digitalWrite(motorAIn2, LOW); 
  digitalWrite(motorBIn3, LOW); digitalWrite(motorBIn4, HIGH); 
  analogWrite(motorAEnable, SPEED); analogWrite(motorBEnable, SPEED);
}
void stopMotors() {
  analogWrite(motorAEnable, 0); analogWrite(motorBEnable, 0);
}
void startPump() { digitalWrite(pumpPin, HIGH); }
void stopPump() { digitalWrite(pumpPin, LOW); }

float getDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long d = pulseIn(ECHO_PIN, HIGH, 30000);
  return (d == 0) ? 999.0 : d * 0.034 / 2;
}

void performScan() {
  if (currentScanState == NOT_SCANNING) return;

  unsigned long currentTime = millis();
  if (currentTime - lastScanTime > 500) {
    lastScanTime = currentTime;
    float d;
    switch (currentScanState) {
      case SCAN_LEFT:
        scanServo.write(150);
        d = getDistance();
        scanResult += "L:" + String(d) + ",";
        currentScanState = SCAN_CENTER;
        break;
      case SCAN_CENTER:
        scanServo.write(90);
        d = getDistance();
        scanResult += "C:" + String(d) + ",";
        currentScanState = SCAN_RIGHT;
        break;
      case SCAN_RIGHT:
        scanServo.write(30);
        d = getDistance();
        scanResult += "R:" + String(d);
        currentScanState = SCAN_COMPLETE;
        break;
      case SCAN_COMPLETE:
        scanServo.write(90); // Reset to center
        udp.beginPacket(pcIp, remoteUdpPort);
        udp.print(scanResult); // Send detailed scan data
        udp.endPacket();
        currentScanState = NOT_SCANNING;
        break;
    }
  }
}

void handleServo() {
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
}

void loop() {
  handleServo();
  performScan();
  if (digitalRead(flameSensor1) == LOW || digitalRead(flameSensor2) == LOW) {
    stopMotors(); startPump(); delay(2000); stopPump();
  }

  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;
    String cmd = String(packetBuffer);
    cmd.trim();

    if (cmd == "mode_auto") {
      currentMode = MODE_AUTO;
      lastCommand = "forward"; // Default to moving forward in auto mode
    }
    else if (cmd == "mode_manual") {
      currentMode = MODE_MANUAL;
      lastCommand = "stop";
      stopMotors();
    }
    else if (cmd == "mode_hold") {
      currentMode = MODE_HOLD;
      lastCommand = "stop";
      stopMotors();
    }

    if (currentMode != MODE_HOLD) {
        lastCommand = cmd;
    }
    
    if (cmd == "scan") {
         startScan();
    }
  }

  if (currentMode != MODE_HOLD) {
    if (lastCommand == "forward") moveForward();
    else if (lastCommand == "backward") moveBackward();
    else if (lastCommand == "left") turnLeft();
    else if (lastCommand == "right") turnRight();
    else if (lastCommand == "stop") stopMotors();
    else if (lastCommand == "pump_on") startPump();
    else if (lastCommand == "pump_off") stopPump();
  }

  static unsigned long lastTime = 0;
  if (millis() - lastTime > 100) {
    udp.beginPacket(pcIp, remoteUdpPort);
    udp.print(getDistance());
    udp.endPacket();
    lastTime = millis();
  }
}
