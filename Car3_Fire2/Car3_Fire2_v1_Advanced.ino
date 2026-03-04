#include <Arduino.h>
#include <ESP32Servo.h>

const int motorAIn1 = 18;
const int motorAIn2 = 5;
const int motorBIn3 = 17;
const int motorBIn4 = 16;

const int motorAEnable = 19;
const int motorBEnable = 4;

const int sensorServoPin = 33;
const int sprayServoPin = 32;

const int trigPin = 27;
const int echoPin = 26;

const int flameSensor1 = 13;
const int flameSensor2 = 15;

const int pumpPin = 25;

const int PWM_CHANNEL_A = 0;
const int PWM_CHANNEL_B = 1;
const int PWM_FREQ = 5000;
const int PWM_RES = 8;

Servo myServo;
Servo sprayServo;

const int SAFE_DISTANCE_CM = 10;
const int MOTOR_SPEED = 200;
const int TURBO_SPEED = 255;
const int MANUAL_TIMEOUT_MS = 5000;
const unsigned long AUTO_MOVE_DURATION_MS = 10000;

const int FIRE_SPRAY_ANGLE = 45;
const int IDLE_SPRAY_ANGLE = 90;
const int PULSE_DURATION_MS = 300;

enum RobotMode { AUTO_MODE, MANUAL_MODE };
RobotMode currentMode = AUTO_MODE;

bool fireWasDetected = false;
bool isPumpOn = false;
unsigned long lastManualCommandTime = 0;
unsigned long lastAutoCheckTime = 0;

int currentSpeed = MOTOR_SPEED;

void setMotorSpeed(int motorA, int motorB) {
  ledcWrite(PWM_CHANNEL_A, motorA);
  ledcWrite(PWM_CHANNEL_B, motorB);
}

void moveForward(int speed) {
  digitalWrite(motorAIn1, HIGH);
  digitalWrite(motorAIn2, LOW);
  digitalWrite(motorBIn3, HIGH);
  digitalWrite(motorBIn4, LOW);
  setMotorSpeed(speed, speed);
}

void moveBackward(int speed) {
  digitalWrite(motorAIn1, LOW);
  digitalWrite(motorAIn2, HIGH);
  digitalWrite(motorBIn3, LOW);
  digitalWrite(motorBIn4, HIGH);
  setMotorSpeed(speed, speed);
}

void turnRight(int speed) {
  digitalWrite(motorAIn1, HIGH);
  digitalWrite(motorAIn2, LOW);
  digitalWrite(motorBIn3, LOW);
  digitalWrite(motorBIn4, HIGH);
  setMotorSpeed(speed, speed);
}

void turnLeft(int speed) {
  digitalWrite(motorAIn1, LOW);
  digitalWrite(motorAIn2, HIGH);
  digitalWrite(motorBIn3, HIGH);
  digitalWrite(motorBIn4, LOW);
  setMotorSpeed(speed, speed);
}

void stopMotors() {
  setMotorSpeed(0, 0);
}

void softStop(int &currentSpeed) {
  while (currentSpeed > 0) {
    currentSpeed -= 20;
    if (currentSpeed < 0) currentSpeed = 0;
    setMotorSpeed(currentSpeed, currentSpeed);
    delay(50);
  }
}

long readDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration / 58;
}

long look(int angle) {
  myServo.write(angle);
  delay(200);
  long distance = readDistanceCM();
  myServo.write(90);
  delay(200);
  return distance;
}

void setSprayAngle(int angle) {
  sprayServo.write(angle);
  delay(100);
}

void startPump() {
  if (!isPumpOn) {
    digitalWrite(pumpPin, HIGH);
    isPumpOn = true;
  }
}

void stopPump() {
  if (isPumpOn) {
    digitalWrite(pumpPin, LOW);
    isPumpOn = false;
  }
}

void pulsePump() {
  startPump();
  delay(PULSE_DURATION_MS);
  stopPump();
}

int autoSpeedByDistance(long distance) {
  if (distance > SAFE_DISTANCE_CM + 10) return MOTOR_SPEED;
  if (distance <= SAFE_DISTANCE_CM) return 0;
  return map(distance, SAFE_DISTANCE_CM, SAFE_DISTANCE_CM + 10, 0, MOTOR_SPEED);
}

void handleManualControl(char command, int &currentSpeed) {
  int targetSpeed = MOTOR_SPEED;
  if (command == 'W' || command == 'S' || command == 'A' || command == 'D')
    targetSpeed = TURBO_SPEED;

  if (currentSpeed > targetSpeed)
    softStop(currentSpeed);

  currentSpeed = targetSpeed;

  char nextCommand = Serial.peek();

  if ((command == 'w' || command == 'W') && (nextCommand == 'd' || nextCommand == 'D')) {
    moveForward(currentSpeed);
    setMotorSpeed(currentSpeed, currentSpeed / 2);
    Serial.read();
    return;
  }

  if ((command == 'w' || command == 'W') && (nextCommand == 'a' || nextCommand == 'A')) {
    moveForward(currentSpeed);
    setMotorSpeed(currentSpeed / 2, currentSpeed);
    Serial.read();
    return;
  }

  switch (command) {
    case 'w':
    case 'W': moveForward(currentSpeed); break;
    case 's':
    case 'S': moveBackward(currentSpeed); break;
    case 'a':
    case 'A': turnLeft(currentSpeed); break;
    case 'd':
    case 'D': turnRight(currentSpeed); break;
    case 'x':
    case 'X':
      softStop(currentSpeed);
      stopMotors();
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(motorAIn1, OUTPUT);
  pinMode(motorAIn2, OUTPUT);
  pinMode(motorBIn3, OUTPUT);
  pinMode(motorBIn4, OUTPUT);

  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RES);
  ledcAttachPin(motorAEnable, PWM_CHANNEL_A);

  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(motorBEnable, PWM_CHANNEL_B);

  pinMode(flameSensor1, INPUT_PULLUP);
  pinMode(flameSensor2, INPUT_PULLUP);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);

  myServo.setPeriodHertz(50);
  myServo.attach(sensorServoPin);

  sprayServo.setPeriodHertz(50);
  sprayServo.attach(sprayServoPin);

  stopMotors();

  myServo.write(90);
  sprayServo.write(IDLE_SPRAY_ANGLE);

  lastAutoCheckTime = millis();

  Serial.println("System Ready. Mode: AUTO");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();

    if (strchr("wWsSaAdDxX", command)) {
      if (currentMode == AUTO_MODE) {
        softStop(currentSpeed);
        currentMode = MANUAL_MODE;
      }

      handleManualControl(command, currentSpeed);
      lastManualCommandTime = millis();
      return;
    }
  }

  if (currentMode == MANUAL_MODE && millis() - lastManualCommandTime > MANUAL_TIMEOUT_MS) {
    softStop(currentSpeed);
    currentMode = AUTO_MODE;
    Serial.println("Manual timeout → AUTO");
    lastAutoCheckTime = millis();
  }

  if (currentMode == AUTO_MODE) {
    int flame1 = digitalRead(flameSensor1);
    int flame2 = digitalRead(flameSensor2);

    if (flame1 == LOW || flame2 == LOW) {
      softStop(currentSpeed);
      setSprayAngle(FIRE_SPRAY_ANGLE);
      pulsePump();
      fireWasDetected = true;
      delay(500);
      return;
    }

    else if (fireWasDetected) {
      stopPump();
      setSprayAngle(IDLE_SPRAY_ANGLE);
      fireWasDetected = false;

      moveBackward(MOTOR_SPEED);
      delay(1000);

      turnLeft(MOTOR_SPEED);
      delay(1000);

      stopMotors();
      lastAutoCheckTime = millis();
    }

    if (!fireWasDetected) {
      long distance = readDistanceCM();
      int speed = autoSpeedByDistance(distance);

      moveForward(speed);

      if (distance <= SAFE_DISTANCE_CM) {
        stopMotors();

        long right1 = look(45);
        long right2 = look(30);

        long left1 = look(135);
        long left2 = look(150);

        long right = max(right1, right2);
        long left = max(left1, left2);

        if (right > left) {
          turnRight(MOTOR_SPEED);
          delay(1000);
        } else if (left > right) {
          turnLeft(MOTOR_SPEED);
          delay(1000);
        } else {
          moveBackward(MOTOR_SPEED);
          delay(500);
          turnRight(MOTOR_SPEED);
          delay(1000);
        }

        stopMotors();
        lastAutoCheckTime = millis();
      }

      else if (millis() - lastAutoCheckTime >= AUTO_MOVE_DURATION_MS) {
        stopMotors();
        delay(300);

        if (random(2) == 0)
          turnRight(MOTOR_SPEED);
        else
          turnLeft(MOTOR_SPEED);

        delay(random(500, 1500));
        stopMotors();
        lastAutoCheckTime = millis();
      }
    }
  }

  delay(50);
}
