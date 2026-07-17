#ifndef CALIBRATION_MENU_H
#define CALIBRATION_MENU_H

#include <Arduino.h>
#include <HX711.h>

// Global flag: true = run calibration menu, false = run normal operation
extern bool is_calibration;

// Print the calibration menu help text
void calibration_menu_print_help();

// Run one iteration of the calibration menu (call in loop() when is_calibration == true)
void calibration_menu_loop(HX711* scale);

#endif