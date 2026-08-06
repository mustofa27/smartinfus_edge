#ifndef INFUSION_APP_H
#define INFUSION_APP_H

#include <HX711.h>
#include "device_status.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "monitor_checker.h"
#include "led_controller.h"
#include "led_mode_resolver.h"
#include "weight_monitor.h"

// Composition root: wires the concrete components together and exposes
// the device state to the higher-level logic. This is the only place that
// knows about every component.
class InfusionApp : public IDeviceStatus {
public:
  InfusionApp();

  void setup();
  void loop();

  // IDeviceStatus
  bool isScaleReady() override;
  bool isWifiConnected() override;
  bool isMqttConnected() override;
  bool isMonitored() override;

private:
  void waitForScale();
  void connectNetwork();
  void maintainConnections();
  void reconnectWifi();
  void reconnectMqtt();

  HX711 scale_;
  WifiManager wifi_;
  MqttManager mqtt_;
  MonitorChecker monitor_;
  LedModeResolver ledResolver_;
  LedController led_;
  WeightMonitor weightMonitor_;
};

#endif  // INFUSION_APP_H
