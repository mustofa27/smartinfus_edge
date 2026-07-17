#include "loadcell.h"
#include <math.h>

// Global calibration state
static Calibration cal = {0, {{0}}, 1.0f, 0.0f, 0.0f, false};

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

// =====================================================
// Linear Regression Calibration Implementation
// =====================================================

// Performs a filtered read and stores a calibration point at the given weight.
// This is the primary way to add a calibration point during live calibration.
void loadcell_add_calibration_point(HX711* scale, float weight_g) {
  if (cal.num_points >= MAX_CALIB_POINTS) {
    Serial.println("ERROR: Max calibration points reached.");
    return;
  }

  float raw = loadcell_read(scale);
  cal.points[cal.num_points].raw_value = raw;
  cal.points[cal.num_points].weight_g = weight_g;
  cal.num_points++;
  cal.calibrated = false;  // regression needs to be recomputed

  Serial.print("Added calibration point: raw=");
  Serial.print(raw, 2);
  Serial.print(", weight=");
  Serial.print(weight_g, 2);
  Serial.println(" g");
}

void loadcell_add_manual_point(float raw_value, float weight_g) {
  if (cal.num_points >= MAX_CALIB_POINTS) {
    Serial.println("ERROR: Max calibration points reached.");
    return;
  }
  cal.points[cal.num_points].raw_value = raw_value;
  cal.points[cal.num_points].weight_g  = weight_g;
  cal.num_points++;
  cal.calibrated = false;
  Serial.print("Added manual point: raw=");
  Serial.print(raw_value, 2);
  Serial.print(", weight=");
  Serial.print(weight_g, 2);
  Serial.println(" g");
}

void loadcell_clear_calibration_points() {
  cal.num_points = 0;
  cal.calibrated = false;
  cal.slope = 1.0f;
  cal.intercept = 0.0f;
  cal.r_squared = 0.0f;
  Serial.println("Cleared all calibration points.");
}

bool loadcell_compute_regression() {
  if (cal.num_points < 2) {
    Serial.print("ERROR: Need at least 2 calibration points, have ");
    Serial.println(cal.num_points);
    return false;
  }

  // Least-squares linear regression: weight = slope * raw + intercept
  // Uses the normal equations:
  //   slope = (n*sum(xy) - sum(x)*sum(y)) / (n*sum(x^2) - sum(x)^2)
  //   intercept = (sum(y) - slope * sum(x)) / n
  //
  // where x = raw_value, y = weight_g

  int n = cal.num_points;
  float sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;

  for (int i = 0; i < n; i++) {
    float x = cal.points[i].raw_value;
    float y = cal.points[i].weight_g;
    sum_x  += x;
    sum_y  += y;
    sum_xy += x * y;
    sum_xx += x * x;
  }

  float denominator = n * sum_xx - sum_x * sum_x;

  if (fabs(denominator) < 1e-12f) {
    Serial.println("ERROR: Calibration points are collinear (all raw values identical).");
    return false;
  }

  cal.slope = (n * sum_xy - sum_x * sum_y) / denominator;
  cal.intercept = (sum_y - cal.slope * sum_x) / n;

  // Compute R-squared (coefficient of determination)
  float sum_y_mean = sum_y / n;
  float ss_res = 0, ss_tot = 0;
  for (int i = 0; i < n; i++) {
    float y_pred = cal.slope * cal.points[i].raw_value + cal.intercept;
    float residual = cal.points[i].weight_g - y_pred;
    ss_res += residual * residual;
    float diff_mean = cal.points[i].weight_g - sum_y_mean;
    ss_tot += diff_mean * diff_mean;
  }

  if (ss_tot > 0) {
    cal.r_squared = 1.0f - (ss_res / ss_tot);
  } else {
    cal.r_squared = 1.0f;  // all y values are identical (only possible with 1 point, but just in case)
  }

  cal.calibrated = true;

  Serial.println("=== Linear Regression Results ===");
  Serial.print("Slope (grams/raw):     ");
  Serial.println(cal.slope, 6);
  Serial.print("Intercept (grams):      ");
  Serial.println(cal.intercept, 4);
  Serial.print("R-squared:              ");
  Serial.println(cal.r_squared, 4);
  Serial.print("Number of points:       ");
  Serial.println(cal.num_points);
  Serial.println("================================");

  return true;
}

float loadcell_to_grams(float raw_reading) {
  if (!cal.calibrated) {
    return 0.0f;
  }
  // grams = slope * raw + intercept
  float grams = cal.slope * raw_reading + cal.intercept;
  return grams;
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

// Legacy: set calibration using two raw values (raw_0g, raw_calib).
// Adds both as calibration points and computes regression.
void loadcell_set_calibration(float raw_0g, float raw_calib) {
  loadcell_clear_calibration_points();

  // Add 0g point
  cal.points[cal.num_points].raw_value = raw_0g;
  cal.points[cal.num_points].weight_g = 0.0f;
  cal.num_points++;

  // Add known weight point
  cal.points[cal.num_points].raw_value = raw_calib;
  cal.points[cal.num_points].weight_g = CALIB_WEIGHT_GRAMS;
  cal.num_points++;

  // Compute regression
  loadcell_compute_regression();
}