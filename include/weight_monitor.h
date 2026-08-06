#ifndef WEIGHT_MONITOR_H
#define WEIGHT_MONITOR_H

#include <Arduino.h>
#include <HX711.h>
#include "config.h"
#include "device_status.h"
#include "publisher.h"

// Samples the load cell over a fixed window, then reports the weight.
// Owns the reporting business rules:
//   - only publish when the weight has decreased, and
//   - only publish while the device is being monitored.
class WeightMonitor {
public:
  WeightMonitor(HX711& scale, IPublisher& publisher, IDeviceStatus& status,
                unsigned long reportIntervalMs = config::WEIGHT_REPORT_INTERVAL_MS);

  void begin(unsigned long now_ms);  // reset the current window
  void collect();                    // add one raw sample (if space remains)
  void update(unsigned long now_ms); // process + publish when the window completes

private:
  void processWindow();
  void publishWeight(float grams);

  HX711& scale_;
  IPublisher& publisher_;
  IDeviceStatus& status_;

  float buffer_[config::SAMPLES_PER_INTERVAL];
  int sampleCount_;
  unsigned long lastReportMs_;
  unsigned long reportIntervalMs_;
  float lastSentWeight_;
};

#endif  // WEIGHT_MONITOR_H
