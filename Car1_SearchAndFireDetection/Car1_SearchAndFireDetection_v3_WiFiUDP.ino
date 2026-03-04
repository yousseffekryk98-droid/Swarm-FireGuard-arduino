#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

// ================= NETWORK CONFIG =================
const char* ssid     = "SwarmRobotics_ECU";
const char* password = "ECU@2025";

IPAddress local_IP(192, 168, 1, 111);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const int localUdpPort = 5001;
const char* pcIp       = "192.168.1.100";
const int remoteUdpPort = 6001;

WiFiUDP udp;
char packetBuffer[255];

// ================= HARDWARE PINS =================
#define IN1 14
#define IN2 27
#define IN3 12
#define IN4 13
#define ENA 32   
#define ENB 33  

#define TRIG_PIN 26
#define ECHO_PIN 25
#define SERVO_PIN 18

// Modes
#define MODE_HOLD 0
#define MODE_AUTO 1
#define MODE_MANUAL 2

Servo scanServo;
int MOTOR_SPEED = 200;
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
    stopCar();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  
  scanServo.attach(SERVO_PIN);
  scanServo.write(90); // Look Forward by default

  if (!WiFi.config(local_IP, gateway, subnet)) Serial.println("Config Failed");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(100);
    Serial.print(".");
   }
  
  udp.begin(localUdpPort);
}

// ================= MOTOR FUNCTIONS =================
void forward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED); analogWrite(ENB, MOTOR_SPEED);
}
void backward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED); analogWrite(ENB, MOTOR_SPEED);
}
void left() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED); analogWrite(ENB, MOTOR_SPEED);
}
void right() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED); analogWrite(ENB, MOTOR_SPEED);
}
void stopCar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0); analogWrite(ENB, 0);
}

// ================= SENSOR LOGIC =================
float getDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long d = pulseIn(ECHO_PIN, HIGH, 30000);
  return (d == 0) ? 999.0 : d * 0.034 / 2;
}

// Perform a 3-point scan: Left(150), Center(90), Right(30)
// Returns string: "L:20,C:50,R:10"
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
      stopCar();
    }
    else if (cmd == "mode_hold") {
      currentMode = MODE_HOLD;
      lastCommand = "stop";
      stopCar();
    }

    if (currentMode != MODE_HOLD) {
        lastCommand = cmd;
    }
    
    // Always allow scan request? Or only in manual/hold?
    if (cmd == "scan") {
        startScan();
    }
  }

  if (currentMode != MODE_HOLD) {
    if (lastCommand == "forward") forward();
    else if (lastCommand == "backward") backward();
    else if (lastCommand == "left") left();
    else if (lastCommand == "right") right();
    else if (lastCommand == "stop") stopCar();
  }

  // Standard Telemetry (Forward View)
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 100) {
    // Only send if not scanning
    float d = getDistance();
    udp.beginPacket(pcIp, remoteUdpPort);
    udp.print(d);
    udp.endPacket();
    lastTime = millis();
  }
}
