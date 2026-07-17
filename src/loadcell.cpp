#include "loadcell.h"
#include <math.h>

// Global calibration state
static Calibration cal = {0, 0, false};

// Linear regression parameters: grams = slope * (raw - offset)
static float slope = 1.0;       // grams per raw unit
static float offset = 0.0;      // raw value at 0g

void loadcell_init(int dout_pin, int sck_pin) {
  // HX711 is initialized externally in main.cpp via scale.begin()
  // This function is reserved for future init needs
}

// -- 2-means clustering helper --
// Partitions 'data' (len = n) into 2 clusters.
// Returns the index (0 or 1) of the cluster with the most members.
// The centroid of the winning cluster is stored in *output.
static void cluster_rms(const float* data, int n, float* output) {
  // --- Step 1: Initialize two centroids (min and max) ---
  float min_val = data[0];
  float max_val = data[0];
  for (int i = 1; i < n; i++) {
    if (data[i] < min_val) min_val = data[i];
    if (data[i] > max_val) max_val = data[i];
  }

  // If all values are identical, just return RMS of all
  if (fabs(max_val - min_val) < 1e-6f) {
    float sum_sq = 0;
    for (int i = 0; i < n; i++) sum_sq += data[i] * data[i];
    *output = sqrtf(sum_sq / n);
    return;
  }

  float c1 = min_val;
  float c2 = max_val;
  int labels[n];   // VLA — GCC supports this on ESP32

  // --- Step 2: Iterate k-means (k=2) ---
  for (int iter = 0; iter < 20; iter++) {
    // Assign each point to nearest centroid
    int count1 = 0, count2 = 0;
    float sum1 = 0, sum2 = 0;

    for (int i = 0; i < n; i++) {
      float d1 = fabs(data[i] - c1);
      float d2 = fabs(data[i] - c2);
      if (d1 <= d2) {
        labels[i] = 0;
        count1++;
        sum1 += data[i];
      } else {
        labels[i] = 1;
        count2++;
        sum2 += data[i];
      }
    }

    // Recompute centroids
    if (count1 > 0) c1 = sum1 / count1;
    if (count2 > 0) c2 = sum2 / count2;

    // Handle empty clusters
    if (count1 == 0) c1 = min_val;
    if (count2 == 0) c2 = max_val;
  }

  // --- Step 3: Select cluster with most members ---
  int count1 = 0, count2 = 0;
  for (int i = 0; i < n; i++) {
    if (labels[i] == 0) count1++;
    else count2++;
  }

  int winner_idx;          // which cluster won (0 or 1)
  int winner_count;        // number of members in winning cluster
  if (count1 >= count2) {
    winner_idx = 0;
    winner_count = count1;
  } else {
    winner_idx = 1;
    winner_count = count2;
  }

  // --- Step 4: Compute RMS of the winning cluster ---
  float sum_sq = 0;
  for (int i = 0; i < n; i++) {
    if (labels[i] == winner_idx) {
      sum_sq += data[i] * data[i];
    }
  }
  *output = sqrtf(sum_sq / winner_count);
}

// Read one filtered value: pool 30, 2-means, pick biggest cluster, RMS
float loadcell_read(HX711* scale) {
  float pool[POOL_SIZE];
  int samples_read = 0;

  // Collect POOL_SIZE raw readings
  for (int i = 0; i < POOL_SIZE; i++) {
    if (scale->is_ready()) {
      pool[samples_read++] = (float)scale->read();
    }
    delay(10);
  }

  // If we got no readings, return 0
  if (samples_read == 0) return 0;

  // Run 2-means + RMS on the collected samples
  float result;
  cluster_rms(pool, samples_read, &result);
  return result;
}

void loadcell_calibrate_offset(HX711* scale) {
  cal.raw_at_0g = loadcell_read(scale);
  offset = cal.raw_at_0g;

  // (Re)calculate slope if both points are known
  if (cal.calibrated) {
    float raw_diff = cal.raw_at_calib - cal.raw_at_0g;
    if (fabs(raw_diff) > 1e-6f) {
      slope = CALIB_WEIGHT_GRAMS / raw_diff;
    }
  }
}

void loadcell_calibrate_weight(HX711* scale) {
  cal.raw_at_calib = loadcell_read(scale);
  cal.calibrated = true;

  float raw_diff = cal.raw_at_calib - cal.raw_at_0g;
  if (fabs(raw_diff) > 1e-6f) {
    slope = CALIB_WEIGHT_GRAMS / raw_diff;
  }
}

float loadcell_to_grams(float raw_reading) {
  return slope * (raw_reading - offset);
}

Calibration loadcell_get_calibration() {
  return cal;
}

// Process any number of raw samples using the same 2-means + RMS pipeline
// Used by both loadcell_read (single shot) and the 30-second window in main.cpp
float loadcell_process_window(const float* samples, int count) {
  if (count == 0) return 0;
  float result;
  cluster_rms(samples, count, &result);
  return result;
}

void loadcell_set_calibration(float raw_0g, float raw_calib) {
  cal.raw_at_0g = raw_0g;
  cal.raw_at_calib = raw_calib;
  offset = raw_0g;

  float raw_diff = cal.raw_at_calib - cal.raw_at_0g;
  if (fabs(raw_diff) > 1e-6f) {
    slope = CALIB_WEIGHT_GRAMS / raw_diff;
  }
  cal.calibrated = true;
}