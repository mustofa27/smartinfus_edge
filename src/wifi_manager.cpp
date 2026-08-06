#include "wifi_manager.h"
#include <WiFi.h>

WifiManager::WifiManager(const char* ssid, const char* password)
    : ssid_(ssid), password_(password) {}

void WifiManager::begin() {
  WiFi.begin(ssid_, password_);
}

bool WifiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

void WifiManager::printConnectionStatus() const {
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}
