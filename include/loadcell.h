#ifndef LOADCELL_H
#define LOADCELL_H

#include <Arduino.h>
#include <HX711.h>

// Number of raw samples to pool per reading
#define POOL_SIZE  30

// Calibration reference weight in grams (used by default 'w' command)
#define CALIB_WEIGHT_GRAMS  536.0

// Maximum number of calibration points for linear regression
#define MAX_CALIB_POINTS  10

// A single calibration point: known weight and its corresponding raw reading
struct CalibrationPoint {
  float raw_value;   // RMS filtered raw reading
  float weight_g;    // known weight in grams
};

// Calibration data using linear regression
struct Calibration {
  int num_points;
  CalibrationPoint points[MAX_CALIB_POINTS];
  float slope;        // regression slope (grams per raw unit)
  float intercept;    // regression intercept (grams)
  float r_squared;    // goodness of fit (0..1)
  bool calibrated;    // true once regression has been computed
};

// Initialize load cell with pin assignments
void loadcell_init(int dout_pin, int sck_pin);

// Get a single filtered reading using 2-means + RMS
float loadcell_read(HX711* scale);

// === Linear Regression Calibration API ===

// Add a calibration point (reads a live filtered raw value for the given known weight)
void loadcell_add_calibration_point(HX711* scale, float weight_g);

// Add a calibration point manually (known raw value and weight, without reading the sensor)
void loadcell_add_manual_point(float raw_value, float weight_g);

// Remove all calibration points
void loadcell_clear_calibration_points();

// Compute linear regression from stored calibration points
// Returns true on success (need at least 2 points), false otherwise
bool loadcell_compute_regression();

// Convert a raw filtered reading to grams using the linear regression model
float loadcell_to_grams(float raw_reading);

// Get current calibration info
Calibration loadcell_get_calibration();

// Process raw samples using 2-means + RMS (same pipeline as loadcell_read)
// Useful for processing a collected window of samples
float loadcell_process_window(const float* samples, int count);

// Legacy: set calibration with two points (0g and calib weight raw values).
// Adds two calibration points and computes regression.
void loadcell_set_calibration(float raw_0g, float raw_calib);

#endif