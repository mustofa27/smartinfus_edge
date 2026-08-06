#include "weight_monitor.h"
#include "loadcell.h"

WeightMonitor::WeightMonitor(HX711& scale, IPublisher& publisher, IDeviceStatus& status,
                             unsigned long reportIntervalMs)
    : scale_(scale),
      publisher_(publisher),
      status_(status),
      sampleCount_(0),
      lastReportMs_(0),
      reportIntervalMs_(reportIntervalMs),
      lastSentWeight_(1e10f) {}

void WeightMonitor::begin(unsigned long now_ms) {
  sampleCount_ = 0;
  lastReportMs_ = now_ms;
  lastSentWeight_ = 1e10f;
}

void WeightMonitor::collect() {
  if (scale_.is_ready() && sampleCount_ < config::SAMPLES_PER_INTERVAL) {
    buffer_[sampleCount_++] = (float)scale_.read();
  }
}

void WeightMonitor::update(unsigned long now_ms) {
  if (now_ms - lastReportMs_ >= reportIntervalMs_) {
    processWindow();
    lastReportMs_ = now_ms;
  }
}

void WeightMonitor::processWindow() {
  if (sampleCount_ == 0) {
    Serial.println("No samples collected in this window");
    return;
  }

  float rmsRaw = loadcell_process_window(buffer_, sampleCount_);
  float grams = loadcell_to_grams(rmsRaw);
  if (grams < 0) grams = 0;  // clamp negative values

  Serial.print("Window: ");
  Serial.print(sampleCount_);
  Serial.print(" samples, RMS raw = ");
  Serial.print(rmsRaw, 2);
  Serial.print(", weight = ");
  Serial.print(grams, 2);
  Serial.println(" g");

  // Only publish when the weight has decreased (new weight < last sent).
  if (grams >= lastSentWeight_) {
    Serial.print("Weight not decreased (");
    Serial.print(grams, 2);
    Serial.print(" >= ");
    Serial.print(lastSentWeight_, 2);
    Serial.println("), skipping MQTT publish");
    sampleCount_ = 0;
    return;
  }

  if (status_.isMonitored()) {
    publishWeight(grams);
    lastSentWeight_ = grams;
  } else {
    Serial.println("Device not monitored, skipping MQTT publish");
  }

  sampleCount_ = 0;
}

void WeightMonitor::publishWeight(float grams) {
  char payload[32];
  snprintf(payload, sizeof(payload), "%.2f", grams);

  if (!publisher_.isConnected()) {
    Serial.println("MQTT not connected, skipping publish");
    return;
  }

  if (publisher_.publish(config::MQTT_TOPIC, payload)) {
    Serial.print("Published: ");
    Serial.print(config::MQTT_TOPIC);
    Serial.print(" -> ");
    Serial.println(payload);
  } else {
    Serial.println("MQTT publish FAILED");
  }
}
