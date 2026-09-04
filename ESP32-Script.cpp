#include <Wire.h>
#include "Adafruit_HTU21DF.h"

// Standard ESP32 I2C pins
#define I2C_SDA 21
#define I2C_SCL 22

// Bilge sensor pin (must be an RTC GPIO, e.g., GPIO 33)
#define BILGE_PIN GPIO_NUM_33

// Sleep time for testing: 30 seconds (hours in production)
#define SLEEP_TIME_SECONDS    30
#define US_CONVERSION_FACTOR  1000000ULL

Adafruit_HTU21DF htu = Adafruit_HTU21DF();

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("[WAKEUP] Woken up by scheduled timer.");
      break;
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("[CRITICAL ALERT] Woken up by bilge float switch! Water detected.");
      break;
    default:
      Serial.printf("[WAKEUP] Normal boot / Reset. Cause: %d\n", wakeup_reason);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Sailboat Telemetry Starting ---");

  // Display the wake-up reason
  print_wakeup_reason();

  // Configure bilge pin with internal pull-up resistor
  pinMode(BILGE_PIN, INPUT_PULLUP);

  // Initialize I2C bus and HTU21D sensor
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!htu.begin()) {
    Serial.println("[ERROR] HTU21D sensor not found. Check SDA/SCL connections.");
  } else {
    float temp = htu.readTemperature();
    float hum = htu.readHumidity();
    Serial.printf("[SENSOR] Temperature: %.2f °C | Humidity: %.2f %%\n", temp, hum);
  }

  // Read bilge status (LOW = switch closed / water level high)
  int bilgeState = digitalRead(BILGE_PIN);
  if (bilgeState == LOW) {
    Serial.println("[BILGE] STATUS: WATER ALERT (Closed Circuit)");
  } else {
    Serial.println("[BILGE] STATUS: Dry (Normal)");
  }

  Serial.println("[SYSTEM] Tasks completed. Preparing Deep Sleep...");

  // Configure timer wake-up (e.g., 30 seconds)
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_SECONDS * US_CONVERSION_FACTOR);

  // Configure float switch wake-up (EXT0 wakes up when the pin goes LOW/0V)
  esp_sleep_enable_ext0_wakeup(BILGE_PIN, 0);

  Serial.println("[SYSTEM] Entering Deep Sleep now.");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {
  // In Deep Sleep, loop() is never reached
}