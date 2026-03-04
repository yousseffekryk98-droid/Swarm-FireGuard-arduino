// ============================================================
// SHARED MOTOR CONTROL LIBRARY
// FireGuard Swarm - Common Utilities
// ============================================================
// This header provides standardized motor control functions
// for all robot units in the fleet.

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

// Standardized motor control interface
class MotorController {
  private:
    int in1, in2, in3, in4, ena, enb;
    int speed;
    
  public:
    MotorController(int _in1, int _in2, int _in3, int _in4, int _ena, int _enb, int _speed = 200) {
      in1 = _in1; in2 = _in2; in3 = _in3; in4 = _in4;
      ena = _ena; enb = _enb;
      speed = _speed;
      init();
    }
    
    void init() {
      pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
      pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);
      pinMode(ena, OUTPUT); pinMode(enb, OUTPUT);
      stop();
    }
    
    void forward(int customSpeed = -1) {
      int s = (customSpeed >= 0) ? customSpeed : speed;
      digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
      digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
      analogWrite(ena, s); analogWrite(enb, s);
    }
    
    void backward(int customSpeed = -1) {
      int s = (customSpeed >= 0) ? customSpeed : speed;
      digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
      digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
      analogWrite(ena, s); analogWrite(enb, s);
    }
    
    void left(int customSpeed = -1) {
      int s = (customSpeed >= 0) ? customSpeed : speed;
      digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
      digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
      analogWrite(ena, s); analogWrite(enb, s);
    }
    
    void right(int customSpeed = -1) {
      int s = (customSpeed >= 0) ? customSpeed : speed;
      digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
      digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
      analogWrite(ena, s); analogWrite(enb, s);
    }
    
    void stop() {
      digitalWrite(in1, LOW); digitalWrite(in2, LOW);
      digitalWrite(in3, LOW); digitalWrite(in4, LOW);
      analogWrite(ena, 0); analogWrite(enb, 0);
    }
    
    void setSpeed(int newSpeed) {
      speed = constrain(newSpeed, 0, 255);
    }
};

#endif
