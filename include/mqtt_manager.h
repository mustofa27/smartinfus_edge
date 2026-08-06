#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "publisher.h"

// Owns the MQTT connection and outbound messages (single responsibility:
// MQTT transport only).
class MqttManager : public IPublisher {
public:
  MqttManager(const char* host, uint16_t port, const char* clientId,
              const char* username, const char* password);

  void begin();        // configure TLS, broker and callback
  bool connectOnce();  // attempt a single connection

  bool isConnected() const override;
  bool publish(const char* topic, const char* payload) override;
  void loop();  // keep the client alive

private:
  static void onMessage(char* topic, byte* payload, unsigned int length);

  WiFiClientSecure wifiClient_;
  mutable PubSubClient client_;  // PubSubClient API is not const-correct
  const char* host_;
  uint16_t port_;
  const char* clientId_;
  const char* username_;
  const char* password_;
};

#endif  // MQTT_MANAGER_H
