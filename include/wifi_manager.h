#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

// Owns the WiFi connection lifecycle (single responsibility: connectivity).
class WifiManager {
public:
  WifiManager(const char* ssid, const char* password);

  void begin();  // start connecting to the network
  bool isConnected() const;
  void printConnectionStatus() const;

private:
  const char* ssid_;
  const char* password_;
};

#endif  // WIFI_MANAGER_H
