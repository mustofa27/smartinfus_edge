#include "calibration_menu.h"
#include "loadcell.h"

// Global flag definition
bool is_calibration = false;   // default to calibration mode

void calibration_menu_print_help() {
  Serial.println();
  Serial.println("====================================");
  Serial.println("   Calibration Menu");
  Serial.println("====================================");
  Serial.println("Commands:");
  Serial.println("  o   -> calibrate offset (remove all weight first)");
  Serial.println("  w   -> calibrate weight (place known weight)");
  Serial.println("  p   -> print calibration values");
  Serial.println("  r   -> show one raw filtered reading");
  Serial.println("  g   -> show weight in grams");
  Serial.println("  c <offset_raw> <weight_raw> -> set calibration manually");
  Serial.println();
}

void calibration_menu_loop(HX711* scale) {
  // Only process if a command is available
  if (!Serial.available()) return;

  char cmd = Serial.read();

  switch (cmd) {

    case 'o': {
      Serial.print("Calibrating offset (0g)... ");
      loadcell_calibrate_offset(scale);
      Calibration c = loadcell_get_calibration();
      Serial.print("done. Raw at 0g = ");
      Serial.println(c.raw_at_0g, 2);
      break;
    }

    case 'w': {
      Serial.print("Calibrating weight (");
      Serial.print(CALIB_WEIGHT_GRAMS);
      Serial.print("g)... ");
      loadcell_calibrate_weight(scale);
      Calibration c = loadcell_get_calibration();
      Serial.print("done. Raw at ");
      Serial.print(CALIB_WEIGHT_GRAMS);
      Serial.print("g = ");
      Serial.println(c.raw_at_calib, 2);
      break;
    }

    case 'p': {
      Calibration c = loadcell_get_calibration();
      Serial.println("--- Calibration ---");
      Serial.print("Raw @ 0g:   "); Serial.println(c.raw_at_0g, 2);
      Serial.print("Raw @ ");
      Serial.print(CALIB_WEIGHT_GRAMS);
      Serial.print("g: ");
      Serial.println(c.raw_at_calib, 2);
      Serial.print("Calibrated: ");
      Serial.println(c.calibrated ? "yes" : "no");
      if (c.calibrated) {
        float grams_test = loadcell_to_grams(c.raw_at_calib);
        Serial.print("Verification: ");
        Serial.print(CALIB_WEIGHT_GRAMS);
        Serial.print("g -> ");
        Serial.print(grams_test, 2);
        Serial.println("g");
      }
      break;
    }

    case 'r': {
      float raw = loadcell_read(scale);
      Serial.print("Raw filtered: ");
      Serial.println(raw, 2);
      break;
    }

    case 'g': {
      float raw = loadcell_read(scale);
      float grams = loadcell_to_grams(raw);
      Serial.print("Weight: ");
      Serial.print(grams, 2);
      Serial.println(" g");
      break;
    }

    case 'c': {
      float raw_0g = Serial.parseFloat();
      float raw_wt = Serial.parseFloat();
      loadcell_set_calibration(raw_0g, raw_wt);
      Serial.println("Calibration set manually.");
      Calibration c = loadcell_get_calibration();
      Serial.print("Raw @ 0g: "); Serial.println(c.raw_at_0g, 2);
      Serial.print("Raw @ ");
      Serial.print(CALIB_WEIGHT_GRAMS);
      Serial.print("g: ");
      Serial.println(c.raw_at_calib, 2);
      break;
    }

    default:
      break;
  }
  Serial.println();
}