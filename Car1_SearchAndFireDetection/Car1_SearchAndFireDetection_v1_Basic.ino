/****************************************************
 *   SMART ESP32 RC CAR – NO LEDC VERSION
 *   - Ultrasonic sweep 1..180° (servo)
 *   - Obstacle avoidance (dynamic)
 *   - Flame sensors (left + right)
 *   - Bluetooth manual override
 *   - WiFi AccessPoint + Web dashboard
 *   - L298N motor control using ONLY digitalWrite (NO PWM)
 ****************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;
WebServer server(80);
Servo scanServo;

// ------------------- PINS -------------------
#define TRIG_PIN 26
#define ECHO_PIN 25
#define SERVO_PIN 18

// L298N MOTOR DRIVER
#define IN1 14
#define IN2 27
#define IN3 12
#define IN4 13
#define ENA_PIN 32   // now ON/OFF only
#define ENB_PIN 33   // now ON/OFF only

// Flame sensors
#define FLAME_LEFT 34
#define FLAME_RIGHT 35

// ------------------- BEHAVIOR -------------------
volatile bool autoMode = true;  // default automatic
const int SAFE_DIST_CM = 30;
const int SWEEP_DELAY_MS = 35;
const int SWEEP_STEP = 10;

// Telemetry
int lastFrontDistance = 999;
int lastMinLeft = 999;
int lastMinRight = 999;
bool flameLeftTriggered = false;
bool flameRightTriggered = false;

// Scan timing
unsigned long lastScanMillis = 0;
const unsigned long SCAN_INTERVAL = 150;

// =========================================================
//                    MOTOR CONTROL (NO LEDC)
// =========================================================
void motorEnable(bool enable) {
  digitalWrite(ENA_PIN, enable ? HIGH : LOW);
  digitalWrite(ENB_PIN, enable ? HIGH : LOW);
}

void forward() {
  motorEnable(true);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  motorEnable(true);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void leftTurn() {
  motorEnable(true);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void rightTurn() {
  motorEnable(true);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopCar() {
  motorEnable(false);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// =========================================================
//                    ULTRASONIC
// =========================================================
long measureDistanceOnce() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999;
  long dist = duration * 0.034 / 2;
  return (dist <= 0) ? 999 : dist;
}

// Sweep from 1 → 180
void performSweepAndDecide() {
  int minLeft = 999;
  int minRight = 999;
  int midDist = 999;

  for (int angle = 1; angle <= 180; angle += SWEEP_STEP) {
    scanServo.write(angle);
    delay(SWEEP_DELAY_MS);

    long d = measureDistanceOnce();

    if (angle > 110) minLeft = min(minLeft, (int)d);
    else if (angle < 70) minRight = min(minRight, (int)d);
    else midDist = min(midDist, (int)d);
  }

  lastMinLeft = minLeft;
  lastMinRight = minRight;
  lastFrontDistance = midDist;

  // Autonomous decision
  if (midDist > SAFE_DIST_CM) {
    forward();
  } else {
    stopCar();
    delay(120);

    if (minLeft <= SAFE_DIST_CM && minRight <= SAFE_DIST_CM) {
      backward();
      delay(350);
      stopCar();
      delay(120);

      if (minLeft > minRight) leftTurn();
      else rightTurn();

      delay(400);
      stopCar();
    } else {
      if (minLeft > minRight) {
        leftTurn();
      } else {
        rightTurn();
      }
      delay(400);
      stopCar();
    }
  }
}

// =========================================================
//                    WIFI DASHBOARD
// =========================================================
const char* AP_SSID = "ESP32-RC-CAR";
const char* AP_PASS = "12345678";

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head>
<meta charset='utf-8'><meta name='viewport' content='width=device-width'>
<title>ESP32 RC Car</title>
<style>
button{width:120px;height:50px;font-size:18px;margin:5px;}
</style>
</head><body>
<h2>ESP32 RC Car</h2>
<button onclick="cmd('F')">Forward</button><br>
<button onclick="cmd('L')">Left</button>
<button onclick="cmd('S')">Stop</button>
<button onclick="cmd('R')">Right</button><br>
<button onclick="cmd('B')">Backward</button><br><br>
<button onclick="setMode('auto')">Auto</button>
<button onclick="setMode('manual')">Manual</button>

<p id='info'></p>

<script>
function cmd(c){ fetch('/move?cmd='+c); }
function setMode(m){ fetch('/mode?m='+m); }
setInterval(async()=>{
 let r=await fetch('/status'); let j=await r.json();
 document.getElementById('info').innerHTML=
 'Mode: '+(j.autoMode?'AUTO':'MANUAL')+
 '<br>Front: '+j.front+' cm'+
 '<br>Left: '+j.left+' cm   Right: '+j.right+' cm'+
 '<br>Flame L: '+j.flLeft+'  Flame R: '+j.flRight;
},500);
</script>
</body></html>
)rawliteral";

void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void handleMove() {
  if (!server.hasArg("cmd")) return server.send(400, "text/plain", "Missing cmd");
  autoMode = false;
  char c = server.arg("cmd")[0];
  if (c=='F') forward();
  else if (c=='B') backward();
  else if (c=='L') leftTurn();
  else if (c=='R') rightTurn();
  else stopCar();
  server.send(200, "text/plain", "OK");
}

void handleMode() {
  if (!server.hasArg("m")) return server.send(400, "text/plain", "Missing m");
  String m = server.arg("m");
  autoMode = (m=="auto");
  if (autoMode) stopCar();
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String j = "{";
  j += "\"autoMode\":" + String(autoMode?"true":"false");
  j += ",\"front\":" + String(lastFrontDistance);
  j += ",\"left\":" + String(lastMinLeft);
  j += ",\"right\":" + String(lastMinRight);
  j += ",\"flLeft\":" + String(flameLeftTriggered?"true":"false");
  j += ",\"flRight\":" + String(flameRightTriggered?"true":"false");
  j += "}";
  server.send(200, "application/json", j);
}

// =========================================================
//                       SETUP
// =========================================================
void setup() {
  Serial.begin(115200);

  SerialBT.begin("ESP32_RC_CAR");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(FLAME_LEFT, INPUT);
  pinMode(FLAME_RIGHT, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);

  stopCar();

  scanServo.attach(SERVO_PIN);
  scanServo.write(90);

  WiFi.softAP(AP_SSID, AP_PASS);
  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.on("/mode", handleMode);
  server.on("/status", handleStatus);
  server.begin();
}

// =========================================================
//                         LOOP
// =========================================================
void loop() {
  server.handleClient();

  if (digitalRead(FLAME_LEFT) == LOW || digitalRead(FLAME_RIGHT) == LOW) {
    flameLeftTriggered = (digitalRead(FLAME_LEFT) == LOW);
    flameRightTriggered = (digitalRead(FLAME_RIGHT) == LOW);
    stopCar();
    return;
  }

  if (autoMode) {
    performSweepAndDecide();
  }

  delay(50);
}
