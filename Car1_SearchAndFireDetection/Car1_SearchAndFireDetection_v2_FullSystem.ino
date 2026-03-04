/****************************************************
 *  SMART ESP32 RC CAR – FULL COMBINED SYSTEM
 *  Ultrasonic + Servo + Flame + Bluetooth + WiFi
 *  Merged without ledc, using analogWrite for PWM
 *  Car moves based on ultrasonic signals in auto mode
 *  Servo sweeps from 1 to 180 degrees in auto mode
 *  Flame sensors have priority emergency stop
 *  Optimized for ESP32 Dev Module
 ****************************************************/

#include <ESP32Servo.h>
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <WebServer.h>

// ------------------- BLUETOOTH -------------------
BluetoothSerial SerialBT;

// ------------------- WIFI ACCESS POINT -------------------
const char* WIFI_SSID = "ESP32_RC_SMART";
const char* WIFI_PASS = "12345678";
WebServer server(80);

// ------------------- SERVO + ULTRASONIC -------------------
Servo scanServo;
#define TRIG_PIN 26
#define ECHO_PIN 25
#define SERVO_PIN 18

// ------------------- MOTOR DRIVER (L298N) -------------------
#define IN1 14
#define IN2 27
#define IN3 12
#define IN4 13
#define ENA 32   
#define ENB 33  

int MOTOR_SPEED = 200; // 0–255

// ------------------- FLAME SENSORS -------------------
#define FLAME_LEFT 34
#define FLAME_RIGHT 35

// ------------------- MODES -------------------
bool autoMode = true;  // Default = automatic mode

long duration;
int distance;

// ------------------- WIFI WEBPAGE -------------------
String webpage = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 RC SMART CAR</title>
<style>
button {
  padding: 20px;
  font-size: 20px;
  margin: 10px;
  width: 120px;
}
</style>
</head>
<body>
<h1>ESP32 RC SMART CAR CONTROL</h1>

<button onclick="send('F')">Forward</button><br>
<button onclick="send('L')">Left</button>
<button onclick="send('S')">Stop</button>
<button onclick="send('R')">Right</button><br>
<button onclick="send('B')">Backward</button><br><br>

<button onclick="send('A')">Auto Mode</button>
<button onclick="send('M')">Manual Mode</button><br><br>

<button onclick="send('T')">Read Sensors</button>

<script>
function send(cmd) {
  fetch('/cmd?c=' + cmd);
}
</script>

</body>
</html>
)====";

// =========================================================
//                   MOTOR CONTROL
// =========================================================
void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void leftTurn() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void rightTurn() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// =========================================================
//                   ULTRASONIC FUNCTION
// =========================================================
int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return 999;

  return duration * 0.034 / 2;
}

// =========================================================
//                   WIFI COMMAND HANDLER
// =========================================================
void handleCommand() {
  String cmd = server.arg("c");

  if (cmd == "A") {
    autoMode = true;
    Serial.println("Auto Mode Activated via Web");
    server.send(200, "text/plain", "Auto Mode ON");
    return;
  }
  if (cmd == "M") {
    autoMode = false;
    stopCar();
    Serial.println("Manual Mode Activated via Web");
    server.send(200, "text/plain", "Manual Mode ON");
    return;
  }

  if (!autoMode) {
    if (cmd == "F") forward();
    else if (cmd == "B") backward();
    else if (cmd == "L") leftTurn();
    else if (cmd == "R") rightTurn();
    else if (cmd == "S") stopCar();
    server.send(200, "text/plain", "OK");
    return;
  }

  if (cmd == "T") {
    long d = getDistance();
    String msg = "Dist: " + String(d) + " cm";
    server.send(200, "text/plain", msg);
    return;
  }

  server.send(200, "text/plain", "OK");
}

// =========================================================
//                    SETUP
// =========================================================
void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_RC_SMART");

  // WiFi AP
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  WiFi.mode(WIFI_AP);  // Ensure AP mode
  Serial.print("WiFi AP Started → ");
  Serial.println(WiFi.softAPIP());

  // Web server
  server.on("/", []() { server.send(200, "text/html", webpage); });
  server.on("/cmd", handleCommand);
  server.begin();

  scanServo.attach(SERVO_PIN);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(FLAME_LEFT, INPUT);
  pinMode(FLAME_RIGHT, INPUT);

  Serial.println("System Ready – Auto Mode ON");
}

// =========================================================
//                    MAIN LOOP
// =========================================================
void loop() {
  // Handle WiFi server
  server.handleClient();

  // =====================================================
  //        FLAME SENSOR SAFETY CHECK (ALWAYS PRIORITY)
  // =====================================================
  if (digitalRead(FLAME_LEFT) == LOW || digitalRead(FLAME_RIGHT) == LOW) {
    Serial.println("🔥 FLAME DETECTED — EMERGENCY BACKUP");
    stopCar();
    backward();
    delay(600);
    stopCar();
    delay(400);
    return; // skip everything else
  }

  // =====================================================
  //        MANUAL CONTROL VIA BLUETOOTH OR WIFI
  // =====================================================
  if (SerialBT.available()) {
    char cmd = SerialBT.read();

    if (cmd == 'A') {
      autoMode = true;
      Serial.println("Auto Mode Activated via BT");
      return;
    }
    if (cmd == 'M') {
      autoMode = false;
      stopCar();
      Serial.println("Manual Mode Activated via BT");
      return;
    }

    if (!autoMode) {
      switch (cmd) {
        case 'F': forward(); break;
        case 'B': backward(); break;
        case 'L': leftTurn(); break;
        case 'R': rightTurn(); break;
        case 'S': stopCar(); break;
      }
      return;
    }
  }

  // =====================================================
  //        AUTOMATIC MODE – ULTRASONIC SCANNING
  // =====================================================
  if (autoMode) {
    // Servo sweeps from 1 to 180 degrees, taking readings at intervals
    int distances[181]; // Array to store distances from 1 to 180
    for (int angle = 1; angle <= 180; angle += 5) { // Sweep in 5-degree steps for speed
      scanServo.write(angle);
      delay(100); // Increased delay for smoother servo movement
      distances[angle] = getDistance();
      Serial.print("Angle: "); Serial.print(angle); Serial.print(" Dist: "); Serial.println(distances[angle]);
    }

    // Find the safest direction (max distance)
    int maxDist = 0;
    int bestAngle = 90; // Default center
    for (int angle = 1; angle <= 180; angle += 5) {
      if (distances[angle] > maxDist && distances[angle] < 999) { // Ignore invalid readings
        maxDist = distances[angle];
        bestAngle = angle;
      }
    }

    Serial.print("Best Angle: "); Serial.print(bestAngle); Serial.print(" Max Dist: "); Serial.println(maxDist);

    // Decision Logic
    if (maxDist > 30) {
      // If front (around 90) is clear, go forward
      if (distances[90] > 30) {
        forward();
      } else {
        // Turn towards best angle
        if (bestAngle < 90) {
          Serial.println("Turning LEFT (Safer Side)");
          leftTurn();
          delay(400);
        } else if (bestAngle > 90) {
          Serial.println("Turning RIGHT (Safer Side)");
          rightTurn();
          delay(400);
        } else {
          // If center is best but blocked, stop or backup
          stopCar();
          delay(200);
        }
      }
    } else {
      // All directions blocked, stop
      stopCar();
      Serial.println("All directions blocked, stopping");
    }
  }

  delay(20);
}
