// ============================================================
// SHARED NETWORK CONFIGURATION
// FireGuard Swarm - WiFi & UDP Setup
// ============================================================
// Centralized network config for all fleet units

#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

// WiFi Credentials
#define WIFI_SSID "SwarmRobotics_ECU"
#define WIFI_PASS "ECU@2025"

// Central PC (Control Station)
#define CENTRAL_IP "192.168.1.100"

// Robot IP Addresses & Ports
typedef struct {
  const char* ssid;
  const char* password;
  uint8_t ip[4];      // e.g., {192, 168, 1, 111}
  uint16_t localPort;
  uint16_t remotePort;
} NetworkConfig;

// Unit-specific configurations
#define UNIT_CAR1_IP {192, 168, 1, 111}
#define UNIT_CAR1_LOCAL_PORT 5001
#define UNIT_CAR1_REMOTE_PORT 6001

#define UNIT_CAR2_IP {192, 168, 1, 112}
#define UNIT_CAR2_LOCAL_PORT 5002
#define UNIT_CAR2_REMOTE_PORT 6002

#define UNIT_CAR3_IP {192, 168, 1, 113}
#define UNIT_CAR3_LOCAL_PORT 5003
#define UNIT_CAR3_REMOTE_PORT 6003

#define UNIT_CAR4_IP {192, 168, 1, 114}
#define UNIT_CAR4_LOCAL_PORT 5004
#define UNIT_CAR4_REMOTE_PORT 6004

// Standard WiFi connection function
#include <WiFi.h>

void configureWiFi(uint8_t ip[4], uint16_t localPort) {
  IPAddress local_IP(ip[0], ip[1], ip[2], ip[3]);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("[WiFi] Config failed");
  }
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(100);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n[WiFi] Connected: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] Connection failed!");
  }
}

#endif
