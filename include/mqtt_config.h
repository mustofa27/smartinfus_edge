#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

#include <Arduino.h>

// MQTT Broker
#define MQTT_HOST     "mqtt.icminovasi.my.id"
#define MQTT_PORT     8883
#define MQTT_USERNAME "smart-infusion"
#define MQTT_PASSWORD "SmartInfusion@2026"

// MQTT Topic: smart-infusion/{device_id}/weight
#define MQTT_TOPIC_PREFIX  "smart-infusion"
#define MQTT_TOPIC_SUFFIX  "weight"

// Select device ID: change to "device_1-c51c" or "device_2-c51c"
#define MQTT_DEVICE_ID     "device_1-c51c"

// Full topic: "smart-infusion/device_1-c51c/weight"
#define MQTT_TOPIC  MQTT_TOPIC_PREFIX "/" MQTT_DEVICE_ID "/" MQTT_TOPIC_SUFFIX

// Device code for status checking
#define DEVICE_CODE      "device_1-c51c"

// API base URL for device status check
#define API_BASE_URL     "https://smartinfusion.icminovasi.my.id"
#define API_STATUS_PATH  "/api/v1/device-status/monitored"
#define API_STATUS_URL   API_BASE_URL API_STATUS_PATH "?device_code=" DEVICE_CODE

// Weight reporting interval (milliseconds)
#define WEIGHT_REPORT_INTERVAL_MS  30000  // 30 seconds

// Number of raw samples to collect per interval
#define SAMPLES_PER_INTERVAL  150  // ~5 samples/second × 30 seconds (at 10ms each)

#endif