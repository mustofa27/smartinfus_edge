#include "monitor_checker.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

MonitorChecker::MonitorChecker(const char* statusUrl, unsigned long checkIntervalMs)
    : url_(statusUrl), intervalMs_(checkIntervalMs), lastCheckMs_(0), cached_(false) {}

void MonitorChecker::begin() {
  cached_ = checkNow();
  lastCheckMs_ = millis();
}

bool MonitorChecker::isMonitored(bool wifiConnected) {
  if (!wifiConnected) {
    cached_ = false;
    return false;
  }

  unsigned long now = millis();
  if (lastCheckMs_ == 0 || now - lastCheckMs_ >= intervalMs_) {
    cached_ = checkNow();
    lastCheckMs_ = now;
  }
  return cached_;
}

bool MonitorChecker::checkNow() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, url_);
  http.setTimeout(5000);

  int httpCode = http.GET();

  if (httpCode <= 0) {
    Serial.print("Status check: HTTP request failed, error: ");
    Serial.println(http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }

  if (httpCode != HTTP_CODE_OK) {
    Serial.print("Status check: HTTP response code: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  String response = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("Status check: JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  bool monitored = doc["monitored"] | false;
  Serial.print("Status check: monitored = ");
  Serial.println(monitored ? "true" : "false");
  return monitored;
}
