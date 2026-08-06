#include "led_controller.h"

LedController::LedController(uint8_t pin, LedModeProvider& modeProvider,
                             unsigned long blinkOffMs, unsigned long blinkGapMs,
                             unsigned long patternCycleMs)
    : pin_(pin),
      modeProvider_(modeProvider),
      currentMode_(LedMode::Standby),
      cycleStartMs_(0),
      blinkOffMs_(blinkOffMs),
      blinkGapMs_(blinkGapMs),
      patternCycleMs_(patternCycleMs) {}

void LedController::begin() {
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, HIGH);  // LED on by default when device is on
}

int LedController::blinkCount(LedMode mode) {
  switch (mode) {
    case LedMode::Startup:              return 7;
    case LedMode::Monitored:            return 2;
    case LedMode::Standby:              return 3;
    case LedMode::MqttDisconnected:     return 4;
    case LedMode::WifiDisconnected:     return 5;
    case LedMode::LoadCellDisconnected: return 6;
  }
  return 1;
}

void LedController::update(unsigned long now_ms) {
  LedMode mode = modeProvider_.resolveMode(now_ms);
  if (mode != currentMode_) {
    currentMode_ = mode;
    cycleStartMs_ = now_ms;
  }

  unsigned long phase_ms = (now_ms - cycleStartMs_) % patternCycleMs_;
  unsigned long pulseSpanMs = blinkOffMs_ + blinkGapMs_;
  int blinks = blinkCount(currentMode_);

  bool ledOn = true;  // ON by default; a blink turns it OFF briefly
  for (int i = 0; i < blinks; i++) {
    unsigned long pulseStartMs = i * pulseSpanMs;
    if (phase_ms >= pulseStartMs && phase_ms < pulseStartMs + blinkOffMs_) {
      ledOn = false;
      break;
    }
  }

  digitalWrite(pin_, ledOn ? HIGH : LOW);
}
