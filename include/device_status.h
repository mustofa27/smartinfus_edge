#ifndef DEVICE_STATUS_H
#define DEVICE_STATUS_H

// Abstraction of the device's runtime state.
// High-level logic (LED mode decisions, weight reporting) depends on this
// interface instead of concrete hardware/network classes (Dependency Inversion).
class IDeviceStatus {
public:
  virtual ~IDeviceStatus() = default;

  // Note: not const because the underlying hardware/network libraries
  // (HX711, PubSubClient) and cached lookups are not const-correct.
  virtual bool isScaleReady() = 0;
  virtual bool isWifiConnected() = 0;
  virtual bool isMqttConnected() = 0;
  virtual bool isMonitored() = 0;
};

#endif  // DEVICE_STATUS_H
