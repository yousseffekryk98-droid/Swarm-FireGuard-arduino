#include <ESP32Servo.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// --- Pins Definition ---
#define IN1 12 
#define IN2 14
#define IN3 27
#define IN4 26
#define ENA 25
#define ENB 33
 
#define TRIG_PIN 32  
#define ECHO_PIN 4
#define SERVO_PIN 15

// --- Objects & Variables ---
SoftwareSerial mySoftwareSerial(16, 17); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
Servo scanServo;

int MOTOR_SPEED = 200; 
unsigned long lastPlayTime = 0; // Added: Needed for the 10-second timer

// --- Motor Functions ---
void robotForward() {
  digitalWrite(IN1, HIGH); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void robotBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void robotLeft() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void robotRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, MOTOR_SPEED);
  analogWrite(ENB, MOTOR_SPEED);
}

void robotStop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// --- Sensor Functions ---
int getUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); 
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

// Added: This function makes the servo look Left and Right
int scanBestDirection() {
  int leftDist, rightDist;
  
  scanServo.write(150); // Look Left
  delay(500);
  leftDist = getUltrasonicDistance();
  
  scanServo.write(30);  // Look Right
  delay(500);
  rightDist = getUltrasonicDistance();
  
  scanServo.write(90);  // Center
  
  if (leftDist > rightDist) return 150; // Return left angle
  else return 30; // Return right angle
}

// --- Setup ---
void setup() {
    Serial.begin(115200);
    
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    pinMode(ENA, OUTPUT); 
    pinMode(ENB, OUTPUT);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    scanServo.attach(SERVO_PIN);
    scanServo.write(90); // Start centered

    mySoftwareSerial.begin(9600);
    if (myDFPlayer.begin(mySoftwareSerial)) {
      myDFPlayer.volume(25);
    }

    robotStop();
}

// --- Main Loop ---
void loop() {
  int distance = getUltrasonicDistance();

  // 1. Sound Timer Logic (Plays every 10 seconds)
  unsigned long currentMillis = millis();
  if (currentMillis - lastPlayTime >= 10000) { 
    myDFPlayer.play(1); 
    lastPlayTime = currentMillis;
    Serial.println("Playing MP3...");
  }

  // 2. Movement & Obstacle Logic
  if (distance < 30 && distance > 0) {
    robotStop();
    delay(200);

    Serial.println("Obstacle detected, scanning...");
    int bestAngle = scanBestDirection(); 

    if (bestAngle > 90) { // If left was clearer
      Serial.println(">>> TURNING LEFT");
      robotLeft();
    } else {              // If right was clearer
      Serial.println(">>> TURNING RIGHT");
      robotRight();
    }
    delay(600);
    robotStop();
  } 
  else {
    robotForward();
  }

  delay(40); 
}
