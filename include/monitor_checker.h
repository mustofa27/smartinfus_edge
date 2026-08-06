#ifndef MONITOR_CHECKER_H
#define MONITOR_CHECKER_H

#include <Arduino.h>

// Checks whether the device is being monitored via the REST API.
// The blocking HTTP call is made at most once per check interval; the
// result is cached in between (single responsibility: status lookups).
class MonitorChecker {
public:
  MonitorChecker(const char* statusUrl, unsigned long checkIntervalMs);

  void begin();  // perform the initial check and latch the timestamp
  bool isMonitored(bool wifiConnected);  // cached; refreshes on interval

private:
  bool checkNow();

  const char* url_;
  unsigned long intervalMs_;
  unsigned long lastCheckMs_;
  bool cached_;
};

#endif  // MONITOR_CHECKER_H
