#include "infusion_app.h"
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "loadcell.h"
#include "calibration_menu.h"

InfusionApp::InfusionApp()
    : wifi_(config::WIFI_SSID, config::WIFI_PASSWORD),
      mqtt_(config::MQTT_HOST, config::MQTT_PORT, config::MQTT_DEVICE_ID,
            config::MQTT_USERNAME, config::MQTT_PASSWORD),
      monitor_(config::API_STATUS_URL, config::MONITOR_CHECK_INTERVAL_MS),
      ledResolver_(*this, config::LED_STARTUP_DURATION_MS),
      led_(config::PIN_LED, ledResolver_, config::LED_BLINK_OFF_MS,
           config::LED_BLINK_GAP_MS, config::LED_PATTERN_CYCLE_MS),
      weightMonitor_(scale_, mqtt_, *this) {}

void InfusionApp::setup() {
  Serial.begin(9600);
  while (!Serial);

  ledResolver_.begin(millis());
  led_.begin();

  // Calibration values specified by the user
  if (config::CALIBRATION_SLOPE != 0.0f) {
    loadcell_set_slope_intercept(config::CALIBRATION_SLOPE, config::CALIBRATION_INTERCEPT);
  } else {
    loadcell_set_calibration(config::CALIBRATION_RAW_0G, config::CALIBRATION_RAW_CALIB);
  }
  scale_.begin(config::PIN_DOUT, config::PIN_SCK);

  waitForScale();

  if (is_calibration) {
    calibration_menu_print_help();
    return;  // stay offline in calibration mode
  }

  connectNetwork();
  monitor_.begin();
  weightMonitor_.begin(millis());

  Serial.println("Starting 5-second weight monitoring...");
}

void InfusionApp::loop() {
  if (is_calibration) {
    calibration_menu_loop(&scale_);
    return;
  }

  unsigned long now = millis();

  led_.update(now);
  weightMonitor_.collect();
  weightMonitor_.update(now);

  maintainConnections();
  mqtt_.loop();

  delay(10);  // keep the sampling rate reasonable
}

// ---- IDeviceStatus --------------------------------------------------------

bool InfusionApp::isScaleReady() { return scale_.is_ready(); }
bool InfusionApp::isWifiConnected() { return wifi_.isConnected(); }
bool InfusionApp::isMqttConnected() { return mqtt_.isConnected(); }
bool InfusionApp::isMonitored() { return monitor_.isMonitored(wifi_.isConnected()); }

// ---- private helpers ------------------------------------------------------

void InfusionApp::waitForScale() {
  Serial.print("Waiting for HX711");
  while (!scale_.is_ready()) {
    Serial.print(".");
    led_.update(millis());
    delay(200);
  }
  Serial.println(" READY!");
}

void InfusionApp::connectNetwork() {
  reconnectWifi();
  reconnectMqtt();
}

void InfusionApp::reconnectWifi() {
  wifi_.begin();
  Serial.print("Connecting to WiFi");
  while (!wifi_.isConnected()) {
    Serial.print(".");
    led_.update(millis());
    delay(500);
  }
  Serial.println();
  wifi_.printConnectionStatus();
}

void InfusionApp::reconnectMqtt() {
  mqtt_.begin();
  Serial.print("Connecting to MQTT broker");
  while (!mqtt_.connectOnce()) {
    Serial.print(".");
    led_.update(millis());
    delay(1000);
  }
  Serial.println(" connected!");
}

void InfusionApp::maintainConnections() {
  if (!wifi_.isConnected()) reconnectWifi();
  if (!mqtt_.isConnected()) reconnectMqtt();
}
