#ifndef LED_MODE_RESOLVER_H
#define LED_MODE_RESOLVER_H

#include "led_controller.h"
#include "device_status.h"

// Decides which status the LED should show, derived from the device state.
// Implements LedModeProvider so LedController can consume it.
class LedModeResolver : public LedModeProvider {
public:
  LedModeResolver(IDeviceStatus& status, unsigned long startupDurationMs);

  void begin(unsigned long now_ms);  // latch the startup timestamp
  LedMode resolveMode(unsigned long now_ms) override;

private:
  IDeviceStatus& status_;
  unsigned long startupDurationMs_;
  unsigned long startMs_;
};

#endif  // LED_MODE_RESOLVER_H
