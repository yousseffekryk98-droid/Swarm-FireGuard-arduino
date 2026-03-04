// ============================================================
// PUMP CONTROL LIBRARY
// FireGuard Swarm - Water/Extinguish System
// ============================================================
// Unified pump control for fire suppression units

#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#include <Arduino.h>

class PumpController {
  private:
    int pumpPin;
    bool isPumpOn;
    unsigned long pumpStartTime;
    unsigned long pumpDuration;
    
  public:
    PumpController(int _pumpPin) : pumpPin(_pumpPin), isPumpOn(false), pumpDuration(0) {
      pinMode(pumpPin, OUTPUT);
      digitalWrite(pumpPin, LOW);
    }
    
    void start() {
      if (!isPumpOn) {
        digitalWrite(pumpPin, HIGH);
        isPumpOn = true;
        pumpStartTime = millis();
        Serial.println("[Pump] ACTIVATED");
      }
    }
    
    void stop() {
      if (isPumpOn) {
        digitalWrite(pumpPin, LOW);
        isPumpOn = false;
        Serial.println("[Pump] DEACTIVATED");
      }
    }
    
    void pulse(unsigned long duration) {
      start();
      pumpDuration = duration;
      pumpStartTime = millis();
    }
    
    void update() {
      if (isPumpOn && pumpDuration > 0) {
        if (millis() - pumpStartTime >= pumpDuration) {
          stop();
          pumpDuration = 0;
        }
      }
    }
    
    bool isActive() {
      return isPumpOn;
    }
};

#endif
