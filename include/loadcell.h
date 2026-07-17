#ifndef LOADCELL_H
#define LOADCELL_H

#include <Arduino.h>
#include <HX711.h>

// Number of raw samples to pool per reading
#define POOL_SIZE  30

// Calibration reference weight in grams
#define CALIB_WEIGHT_GRAMS  536.0

// Calibration data
struct Calibration {
  float raw_at_0g;      // RMS reading at 0g (offset)
  float raw_at_calib;   // RMS reading at CALIB_WEIGHT_GRAMS
  bool calibrated;      // true once calibrated
};

// Initialize load cell with pin assignments
void loadcell_init(int dout_pin, int sck_pin);

// Get a single filtered reading using 2-means + RMS
float loadcell_read(HX711* scale);

// Calibrate: set the offset (call with no load)
void loadcell_calibrate_offset(HX711* scale);

// Calibrate: set the reference weight reading (call with known weight applied)
void loadcell_calibrate_weight(HX711* scale);

// Convert a raw filtered reading to grams using linear regression
float loadcell_to_grams(float raw_reading);

// Get current calibration info
Calibration loadcell_get_calibration();

// Process raw samples using 2-means + RMS (same pipeline as loadcell_read)
// Useful for processing a collected window of samples
float loadcell_process_window(const float* samples, int count);

// Set calibration manually (e.g., from saved values)
void loadcell_set_calibration(float raw_0g, float raw_calib);

#endif
