#include "led_mode_resolver.h"

LedModeResolver::LedModeResolver(IDeviceStatus& status, unsigned long startupDurationMs)
    : status_(status), startupDurationMs_(startupDurationMs), startMs_(0) {}

void LedModeResolver::begin(unsigned long now_ms) {
  startMs_ = now_ms;
}

LedMode LedModeResolver::resolveMode(unsigned long now_ms) {
  if (startMs_ != 0 && now_ms - startMs_ < startupDurationMs_) {
    return LedMode::Startup;
  }
  if (!status_.isScaleReady())    return LedMode::LoadCellDisconnected;
  if (!status_.isWifiConnected()) return LedMode::WifiDisconnected;
  if (!status_.isMqttConnected()) return LedMode::MqttDisconnected;
  return status_.isMonitored() ? LedMode::Monitored : LedMode::Standby;
}
