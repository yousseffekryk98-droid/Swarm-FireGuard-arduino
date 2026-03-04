// ============================================================
// SHARED SENSOR CONTROL LIBRARY
// FireGuard Swarm - Common Ultrasonic & Flame Detection
// ============================================================
// Standardized sensor interface for all units

#ifndef SENSOR_CONTROL_H
#define SENSOR_CONTROL_H

#include <Arduino.h>

// Ultrasonic sensor interface
class UltrasonicSensor {
  private:
    int trigPin, echoPin;
    
  public:
    UltrasonicSensor(int _trig, int _echo) : trigPin(_trig), echoPin(_echo) {
      pinMode(trigPin, OUTPUT);
      pinMode(echoPin, INPUT);
    }
    
    long getDistance() {
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
      
      long duration = pulseIn(echoPin, HIGH, 30000);
      if (duration == 0) return 999;
      return duration * 0.034 / 2;  // Convert to cm
    }
};

// Flame sensor interface
class FlameSensor {
  private:
    int pin1, pin2;
    
  public:
    FlameSensor(int _pin1, int _pin2) : pin1(_pin1), pin2(_pin2) {
      pinMode(pin1, INPUT_PULLUP);
      pinMode(pin2, INPUT_PULLUP);
    }
    
    bool isFlameDetected() {
      // Active LOW logic - flame detected when pin is LOW
      return (digitalRead(pin1) == LOW || digitalRead(pin2) == LOW);
    }
    
    bool isLeftFlame() {
      return digitalRead(pin1) == LOW;
    }
    
    bool isRightFlame() {
      return digitalRead(pin2) == LOW;
    }
};

#endif
