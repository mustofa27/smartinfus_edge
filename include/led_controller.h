#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

// The status the LED is asked to display.
enum class LedMode {
  Startup,
  Monitored,
  Standby,
  WifiDisconnected,
  MqttDisconnected,
  LoadCellDisconnected,
};

// Supplies the current status for the LED.
// LedController depends on this abstraction (Dependency Inversion).
class LedModeProvider {
public:
  virtual ~LedModeProvider() = default;
  virtual LedMode resolveMode(unsigned long now_ms) = 0;
};

// Renders status patterns on the onboard LED (single responsibility:
// presentation only — it never decides *why* a state applies).
//
// The LED is ON by default; a "blink" briefly turns it OFF and back ON.
// A pattern is <blinks> short OFF blinks repeated every pattern cycle.
class LedController {
public:
  LedController(uint8_t pin, LedModeProvider& modeProvider,
                unsigned long blinkOffMs, unsigned long blinkGapMs,
                unsigned long patternCycleMs);

  void begin();                       // configure pin, LED ON by default
  void update(unsigned long now_ms);  // refresh the blink pattern

private:
  static int blinkCount(LedMode mode);

  uint8_t pin_;
  LedModeProvider& modeProvider_;
  LedMode currentMode_;
  unsigned long cycleStartMs_;
  unsigned long blinkOffMs_;
  unsigned long blinkGapMs_;
  unsigned long patternCycleMs_;
};

#endif  // LED_CONTROLLER_H
