#include "mqtt_manager.h"

MqttManager::MqttManager(const char* host, uint16_t port, const char* clientId,
                         const char* username, const char* password)
    : wifiClient_(),
      client_(wifiClient_),
      host_(host),
      port_(port),
      clientId_(clientId),
      username_(username),
      password_(password) {}

void MqttManager::begin() {
  // No CA certificate for now (self-signed / test broker).
  wifiClient_.setInsecure();
  client_.setServer(host_, port_);
  client_.setCallback(&MqttManager::onMessage);
}

bool MqttManager::connectOnce() {
  return client_.connect(clientId_, username_, password_);
}

bool MqttManager::isConnected() const {
  return client_.connected();
}

bool MqttManager::publish(const char* topic, const char* payload) {
  if (!client_.connected()) return false;
  return client_.publish(topic, payload);
}

void MqttManager::loop() {
  client_.loop();
}

void MqttManager::onMessage(char* topic, byte*, unsigned int) {
  // Reserved for future commands (e.g. remote control via an MQTT/Blynk bridge).
  Serial.print("MQTT message on topic: ");
  Serial.println(topic);
}
