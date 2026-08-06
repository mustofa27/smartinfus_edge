#include <Arduino.h>
#include "infusion_app.h"

InfusionApp app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}
