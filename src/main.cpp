#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <math.h>
#include <HX711.h>
#include "loadcell.h"
#include "calibration_menu.h"
#include "mqtt_config.h"

// ------------------------------------------------
// HX711 wiring
// VCC  -> 3.3V
// GND  -> GND
// DT   -> D4  (GPIO 4)
// SCK  -> RX2 (GPIO 16)
// ------------------------------------------------
#define DOUT_PIN  4
#define SCK_PIN   16

HX711 scale;

// ------------------------------------------------
// WiFi credentials — set these before flashing
// ------------------------------------------------
#define WIFI_SSID     "POLITEKNIK-NEGERI-MADURA"
#define WIFI_PASSWORD ""

// ------------------------------------------------
// MQTT client (TLS on port 8883)
// ------------------------------------------------
WiFiClientSecure wifi_client;
PubSubClient mqtt_client(wifi_client);

// ------------------------------------------------
// 30-second data window buffer
// ------------------------------------------------
static float sample_buffer[SAMPLES_PER_INTERVAL];
static int sample_count = 0;
static unsigned long last_report_ms = 0;

// ------------------------------------------------
// MQTT callback
// ------------------------------------------------
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // Not used for now, but required by PubSubClient
}

// ------------------------------------------------
// Connect to WiFi
// ------------------------------------------------
void connect_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

// ------------------------------------------------
// Connect to MQTT broker
// ------------------------------------------------
void connect_mqtt() {
  // Disable certificate verification (for self-signed or test environments)
  // For production, use wifi_client.setCACert() with the proper CA certificate
  wifi_client.setInsecure();

  mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);

  Serial.print("Connecting to MQTT broker");
  while (!mqtt_client.connected()) {
    if (mqtt_client.connect(MQTT_DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println(" connected!");
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
}

// ------------------------------------------------
// Check if device is monitored via REST API
// Returns true if device is monitored, false otherwise
// ------------------------------------------------
bool check_device_monitored() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Status check: WiFi not connected");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, API_STATUS_URL);
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

  // Parse JSON response
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

// ------------------------------------------------
// Publish weight (grams) over MQTT
// ------------------------------------------------
void publish_weight(float grams) {
  char payload[32];
  snprintf(payload, sizeof(payload), "%.2f", grams);

  if (mqtt_client.connected()) {
    boolean published = mqtt_client.publish(MQTT_TOPIC, payload);
    if (published) {
      Serial.print("Published: ");
      Serial.print(MQTT_TOPIC);
      Serial.print(" -> ");
      Serial.println(payload);
    } else {
      Serial.println("MQTT publish FAILED");
    }
  } else {
    Serial.println("MQTT not connected, reconnecting...");
    connect_mqtt();
  }
}

// ------------------------------------------------
// Collect a sample into the 30-second window buffer
// ------------------------------------------------
void collect_sample() {
  if (scale.is_ready() && sample_count < SAMPLES_PER_INTERVAL) {
    sample_buffer[sample_count++] = (float)scale.read();
  }
}

// ------------------------------------------------
// Process and publish the 30-second window
// ------------------------------------------------
void process_and_publish_window() {
  if (sample_count == 0) {
    Serial.println("No samples collected in this window");
    return;
  }

  // Apply 2-means + RMS (same pipeline as loadcell_read)
  float rms_raw = loadcell_process_window(sample_buffer, sample_count);

  // Convert to grams using linear regression
  float grams = loadcell_to_grams(rms_raw);
  if (grams < 0) grams = 0;  // clamp negative values

  Serial.print("Window: ");
  Serial.print(sample_count);
  Serial.print(" samples, RMS raw = ");
  Serial.print(rms_raw, 2);
  Serial.print(", weight = ");
  Serial.print(grams, 2);
  Serial.println(" g");

  // Only send if weight has decreased (new weight < last sent weight)
  static float last_sent_weight = 1e10f;
  if (grams >= last_sent_weight) {
    Serial.print("Weight not decreased (");
    Serial.print(grams, 2);
    Serial.print(" >= ");
    Serial.print(last_sent_weight, 2);
    Serial.println("), skipping MQTT publish");
    sample_count = 0;
    return;
  }

  // Check if device is monitored before publishing
  if (check_device_monitored()) {
    publish_weight(grams);
    last_sent_weight = grams;
  } else {
    Serial.println("Device not monitored, skipping MQTT publish");
  }

  // Reset buffer
  sample_count = 0;
}

// ------------------------------------------------
// setup()
// ------------------------------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Set calibration values as specified by user
  loadcell_set_calibration(169656.02f, 409660.69f);

  // Initialize HX711
  scale.begin(DOUT_PIN, SCK_PIN);

  Serial.print("Waiting for HX711");
  while (!scale.is_ready()) {
    Serial.print(".");
    delay(200);
  }
  Serial.println(" READY!");
  Serial.println();

  if (is_calibration) {
    calibration_menu_print_help();
    return;  // Don't connect WiFi/MQTT in calibration mode
  }

  // Normal operation: connect WiFi and MQTT
  connect_wifi();
  connect_mqtt();

  last_report_ms = millis();
  Serial.println("Starting 30-second weight monitoring...");
  Serial.println();
}

// ------------------------------------------------
// loop()
// ------------------------------------------------
void loop() {
  if (is_calibration) {
    calibration_menu_loop(&scale);
    return;
  }

  unsigned long now = millis();

  // Collect raw samples continuously
  collect_sample();

  // Every 30 seconds: process and publish
  if (now - last_report_ms >= WEIGHT_REPORT_INTERVAL_MS) {
    process_and_publish_window();
    last_report_ms = now;
  }

  // Maintain MQTT connection
  if (WiFi.status() != WL_CONNECTED) {
    connect_wifi();
  }
  if (!mqtt_client.connected()) {
    connect_mqtt();
  }
  mqtt_client.loop();

  // Small delay to keep sampling rate reasonable
  delay(10);
}