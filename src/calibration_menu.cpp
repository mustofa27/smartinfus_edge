#include "calibration_menu.h"
#include "loadcell.h"
#include <stdio.h>

// Global flag definition
bool is_calibration = false;   // default to calibration mode

// Commands are line-based: they only execute after the Enter key is pressed.
#define CMD_LINE_MAX  64
static char cmd_line[CMD_LINE_MAX];
static int  cmd_len = 0;

// Block until the user presses Enter (consumes everything up to the newline).
static void waitForEnter() {
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') return;
    }
    delay(50);
  }
}

void calibration_menu_print_help() {
  Serial.println();
  Serial.println("====================================");
  Serial.println("   Calibration Menu (Linear Regression)");
  Serial.println("====================================");
  Serial.println("Type a command, then press Enter:");
  Serial.println("  o              -> add point at 0g (remove all weight)");
  Serial.println("  w <weight>     -> add point at the given weight (e.g., w 536)");
  Serial.println("  c <raw> <wgt>  -> add point manually (raw value + weight)");
  Serial.println("  s <slope> <int>  -> set slope & intercept directly (grams = slope*raw + intercept)");
  Serial.println("  l              -> list all calibration points");
  Serial.println("  x              -> clear all calibration points");
  Serial.println("  f              -> fit linear regression from stored points");
  Serial.println("  p              -> print calibration results (regression)");
  Serial.println("  r              -> show one raw filtered reading");
  Serial.println("  g              -> show weight in grams");
  Serial.println("  h              -> print this help");
  Serial.println();
}

// Execute one complete command line (already terminated by Enter).
static void process_command(HX711* scale, const char* line) {
  // Skip any leading whitespace before the command letter
  while (*line == ' ' || *line == '\t') line++;

  char cmd = line[0];

  switch (cmd) {

    case 'o': {
      // Add calibration point at 0g (no load)
      Serial.println("Place NO weight on the scale, then press Enter...");
      waitForEnter();

      loadcell_add_calibration_point(scale, 0.0f);
      break;
    }

    case 'w': {
      // Add calibration point at a user-specified weight
      float weight = 0;
      if (sscanf(line, "w %f", &weight) != 1 || weight <= 0) {
        Serial.println("Usage: w <weight_in_grams>");
        Serial.println("Place the known weight on the scale, then type w 536 (for example).");
        break;
      }
      Serial.print("Place ");
      Serial.print(weight, 1);
      Serial.print("g on the scale, then press Enter...");
      waitForEnter();

      loadcell_add_calibration_point(scale, weight);
      break;
    }

    case 'c': {
      // Add calibration point manually (raw value and weight)
      float raw_val = 0, weight = 0;
      if (sscanf(line, "c %f %f", &raw_val, &weight) != 2) {
        Serial.println("Usage: c <raw> <weight>");
        break;
      }

      loadcell_add_manual_point(raw_val, weight);
      break;
    }

    case 's': {
      // Set slope and intercept directly (from a known previous calibration)
      float slope = 0, intercept = 0;
      if (sscanf(line, "s %f %f", &slope, &intercept) != 2) {
        Serial.println("Usage: s <slope> <intercept>");
        break;
      }
      loadcell_set_slope_intercept(slope, intercept);
      break;
    }

    case 'l': {
      // List all calibration points
      Calibration c = loadcell_get_calibration();
      Serial.print("Calibration points (");
      Serial.print(c.num_points);
      Serial.println("):");
      if (c.num_points == 0) {
        Serial.println("  (none)");
      } else {
        for (int i = 0; i < c.num_points; i++) {
          Serial.print("  ");
          Serial.print(i + 1);
          Serial.print(": raw=");
          Serial.print(c.points[i].raw_value, 2);
          Serial.print(", weight=");
          Serial.print(c.points[i].weight_g, 2);
          Serial.println(" g");
        }
      }
      break;
    }

    case 'x': {
      // Clear all calibration points
      loadcell_clear_calibration_points();
      break;
    }

    case 'f': {
      // Fit linear regression
      loadcell_compute_regression();
      break;
    }

    case 'p': {
      // Print calibration results
      Calibration c = loadcell_get_calibration();
      Serial.println("--- Calibration ---");
      if (!c.calibrated) {
        Serial.println("  Not calibrated. Add points with 'o', 'w', or 'c', then use 'f' to fit.");
        Serial.print("  Current points: ");
        Serial.println(c.num_points);
        break;
      }
      Serial.print("Slope:       ");
      Serial.println(c.slope, 6);
      Serial.print("Intercept:   ");
      Serial.println(c.intercept, 4);
      Serial.print("R-squared:   ");
      Serial.println(c.r_squared, 4);
      Serial.print("Calibrated:  ");
      Serial.println(c.calibrated ? "yes" : "no");
      Serial.print("Num points:  ");
      Serial.println(c.num_points);
      for (int i = 0; i < c.num_points; i++) {
        float predicted = c.slope * c.points[i].raw_value + c.intercept;
        float error = predicted - c.points[i].weight_g;
        Serial.print("  Point ");
        Serial.print(i + 1);
        Serial.print(": raw=");
        Serial.print(c.points[i].raw_value, 2);
        Serial.print(", actual=");
        Serial.print(c.points[i].weight_g, 2);
        Serial.print(" g, pred=");
        Serial.print(predicted, 2);
        Serial.print(" g, err=");
        Serial.print(error, 2);
        Serial.println(" g");
      }

      // Verification using a new reading
      Serial.println();
      Serial.println("Verification (current reading):");
      float raw = loadcell_read(scale);
      float grams = loadcell_to_grams(raw);
      Serial.print("  Raw: ");
      Serial.print(raw, 2);
      Serial.print(", Weight: ");
      Serial.print(grams, 2);
      Serial.println(" g");
      break;
    }

    case 'r': {
      float raw = loadcell_read(scale);
      Serial.print("Raw filtered: ");
      Serial.println(raw, 2);
      break;
    }

    case 'g': {
      Calibration c = loadcell_get_calibration();
      if (!c.calibrated) {
        Serial.println("Scale not calibrated. Use 'f' to fit regression first.");
        break;
      }
      float raw = loadcell_read(scale);
      float grams = loadcell_to_grams(raw);
      Serial.print("Weight: ");
      Serial.print(grams, 2);
      Serial.println(" g");
      break;
    }

    case 'h': {
      calibration_menu_print_help();
      break;
    }

    default:
      break;
  }
  Serial.println();
}

void calibration_menu_loop(HX711* scale) {
  // Accumulate characters until Enter is pressed, then process the full line.
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (cmd_len > 0) {
        cmd_line[cmd_len] = '\0';
        cmd_len = 0;
        process_command(scale, cmd_line);
      }
      // empty lines (e.g. the second byte of CRLF) are ignored
    } else if (cmd_len < CMD_LINE_MAX - 1) {
      cmd_line[cmd_len++] = c;
    }
  }
}