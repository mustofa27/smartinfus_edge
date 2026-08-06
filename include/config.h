#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Centralised compile-time configuration.
// Every tunable value the application needs at build time lives here.
namespace config {

// ---- Pins ---------------------------------------------------------------
static constexpr uint8_t PIN_DOUT = 4;   // HX711 data pin
static constexpr uint8_t PIN_SCK  = 16;  // HX711 clock pin
static constexpr uint8_t PIN_LED  = 2;   // onboard status LED

// ---- WiFi ---------------------------------------------------------------
static constexpr const char* WIFI_SSID     = "POLITEKNIK-NEGERI-MADURA";
static constexpr const char* WIFI_PASSWORD = "";

// ---- MQTT (TLS on port 8883) --------------------------------------------
static constexpr const char* MQTT_HOST      = "mqtt.icminovasi.my.id";
static constexpr uint16_t    MQTT_PORT      = 8883;
static constexpr const char* MQTT_USERNAME  = "smart-infusion";
static constexpr const char* MQTT_PASSWORD  = "SmartInfusion@2026";
static constexpr const char* MQTT_DEVICE_ID = "device_2-c51c";
static constexpr const char* MQTT_TOPIC     = "smart-infusion/device_2-c51c/weight";

// ---- Device status API --------------------------------------------------
static constexpr const char* API_STATUS_URL =
    "https://smartinfusion.icminovasi.my.id/api/v1/device-status/monitored?device_code=device_2-c51c";

// ---- Weight monitoring ---------------------------------------------------
static constexpr unsigned long WEIGHT_REPORT_INTERVAL_MS = 5000;   // window length (5 s)
static constexpr int           SAMPLES_PER_INTERVAL      = 150;    // max samples buffered per window

// Calibration reads pool raw samples for this long before applying the
// 2-means + RMS filter (same pipeline as the monitoring window).
static constexpr unsigned long CALIBRATION_SAMPLE_MS = 5000;

// ---- LED status indicator ------------------------------------------------
static constexpr unsigned long LED_BLINK_OFF_MS        = 120;   // LED OFF during one blink
static constexpr unsigned long LED_BLINK_GAP_MS        = 140;   // LED stays ON between blinks
static constexpr unsigned long LED_PATTERN_CYCLE_MS    = 10000; // pattern repeat period
static constexpr unsigned long LED_STARTUP_DURATION_MS = 10000;
static constexpr unsigned long MONITOR_CHECK_INTERVAL_MS = 60000;

// ---- Load cell calibration (linear regression, set before flashing) ------
static constexpr float CALIBRATION_RAW_0G    = 169656.02f;
static constexpr float CALIBRATION_RAW_CALIB = 409660.69f;

// Direct linear model coefficients (grams = slope * raw + intercept).
// When CALIBRATION_SLOPE is non-zero the infusion setup applies these directly;
// otherwise the two-point raw calibration above is used.
static constexpr float CALIBRATION_SLOPE     = 0.002181f;  // 0 = disabled (use raw points above)
static constexpr float CALIBRATION_INTERCEPT = -193.8264f;

}  // namespace config

#endif  // CONFIG_H
