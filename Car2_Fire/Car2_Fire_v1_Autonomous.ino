#include <Arduino.h>         
#include <ESP32Servo.h>      

// --- PIN DEFINITIONS ---
const int motorAIn1 = 25;   
const int motorAIn2 = 26;   
const int motorBIn3 = 14;   
const int motorBIn4 = 13;   
const int motorAEnable = 27;  
const int motorBEnable = 16;  

const int servoPin = 15;      
const int trigPin = 18;       
const int echoPin = 19;       
const int flameSensor1 = 32;  
const int flameSensor2 = 33;  
const int pumpPin = 4;        

// --- INSTANCES & CONSTANTS ---
Servo myServo;                  
const int DEFAULT_MOVE_SPEED = 120; 
const int REVERSE_MOVE_SPEED = 100; 

// --- FUNCTION PROTOTYPES ---
void moveForward();
void moveBackward();
void turnRight();
void turnLeft();
void stopMotors();
long readDistanceCM();
long look(int angle); 
void startPump();
void stopPump();

// Function: Setup and Initialization
void setup() {
  Serial.begin(115200);

  pinMode(motorAIn1, OUTPUT); pinMode(motorAIn2, OUTPUT);
  pinMode(motorBIn3, OUTPUT); pinMode(motorBIn4, OUTPUT);
  pinMode(motorAEnable, OUTPUT); pinMode(motorBEnable, OUTPUT); 

  pinMode(flameSensor1, INPUT_PULLUP);
  pinMode(flameSensor2, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW); 
  
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin);
  
  stopMotors();
  myServo.write(90);      
  
  Serial.println("System Ready - Autonomous Firefighting Mode");
}

// --- CORE MOVEMENT FUNCTIONS ---

void moveForward() {
  digitalWrite(motorAIn1, HIGH); 
  digitalWrite(motorAIn2, LOW);
  digitalWrite(motorBIn3, HIGH); 
  digitalWrite(motorBIn4, LOW);
  analogWrite(motorAEnable, DEFAULT_MOVE_SPEED); 
  analogWrite(motorBEnable, DEFAULT_MOVE_SPEED);
}

void moveBackward() {
  digitalWrite(motorAIn1, LOW); 
  digitalWrite(motorAIn2, HIGH);
  digitalWrite(motorBIn3, LOW); 
  digitalWrite(motorBIn4, HIGH);
  analogWrite(motorAEnable, REVERSE_MOVE_SPEED); 
  analogWrite(motorBEnable, REVERSE_MOVE_SPEED);
}

void turnRight() {
  digitalWrite(motorAIn1, HIGH); 
  digitalWrite(motorAIn2, LOW); 
  digitalWrite(motorBIn3, LOW); 
  digitalWrite(motorBIn4, HIGH); 
  analogWrite(motorAEnable, DEFAULT_MOVE_SPEED); 
  analogWrite(motorBEnable, DEFAULT_MOVE_SPEED);
}

void turnLeft() {
  digitalWrite(motorAIn1, LOW); 
  digitalWrite(motorAIn2, HIGH); 
  digitalWrite(motorBIn3, HIGH); 
  digitalWrite(motorBIn4, LOW); 
  analogWrite(motorAEnable, DEFAULT_MOVE_SPEED); 
  analogWrite(motorBEnable, DEFAULT_MOVE_SPEED);
}

void stopMotors() {
  analogWrite(motorAEnable, 0);
  analogWrite(motorBEnable, 0);
}

// --- UTILITY FUNCTIONS ---

void startPump() {
    digitalWrite(pumpPin, HIGH);  
    Serial.println("!!! WATER PUMP ACTIVATED !!!");
}

void stopPump() {
    digitalWrite(pumpPin, LOW);  
    Serial.println("!!! WATER PUMP DEACTIVATED !!!");
}

long readDistanceCM() {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); 
  return duration / 58;
}

long look(int angle) {
  myServo.write(angle);
  delay(300); 
  long distance = readDistanceCM();
  myServo.write(90); 
  delay(300);
  return distance;
}

// Function: Main Execution Loop
void loop() {
  
  int flame1Status = digitalRead(flameSensor1);  
  int flame2Status = digitalRead(flameSensor2);  

  if (flame1Status == LOW || flame2Status == LOW) { 
    stopMotors(); 
    Serial.println("!!! FIRE DETECTED !!!");
    
    startPump(); 
    delay(5000); 
    stopPump(); 
    
    moveBackward(); delay(1000);
    turnLeft(); delay(1000);
    stopMotors(); 
    
  } else {
    long distance = readDistanceCM(); 
    
    if (distance > 0 && distance <= 15) { 
      stopMotors(); 
      long rightDistance = look(30); 
      long leftDistance = look(150); 
      
      if (rightDistance > leftDistance) { 
        turnRight(); delay(1000); 
      } 
      else { 
        turnLeft(); delay(1000); 
      }
      stopMotors(); 
    } 
    else {
      moveForward();
    }
  }
  
  delay(50); 
}
