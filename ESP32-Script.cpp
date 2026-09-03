#include <Wire.h>
#include "Adafruit_HTU21DF.h"

// Pines I2C estándar del ESP32
#define I2C_SDA 21
#define I2C_SCL 22

// Pin del sensor de sentina (debe ser RTC GPIO, ej. 33)
#define PIN_SENTINA GPIO_NUM_33

// Tiempo de sueño para la prueba: 30 segundos (en producción serán horas)
#define TIEMPO_SUENO_SEGUNDOS  30
#define FACTOR_CONVERSION_US   1000000ULL

Adafruit_HTU21DF htu = Adafruit_HTU21DF();

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("[WAKEUP] Despertado por temporizador programado.");
      break;
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("[ALERTA CRÍTICA] ¡Despertado por el flotador de sentina! Agua detectada.");
      break;
    default:
      Serial.printf("[WAKEUP] Inicio normal / Reset. Causa: %d\n", wakeup_reason);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Telemetría Velero Iniciando ---");

  // Mostrar la razón por la que se despertó
  print_wakeup_reason();

  // Configurar pin de sentina con resistencia interna pull-up
  pinMode(PIN_SENTINA, INPUT_PULLUP);

  // Inicializar bus I2C y sensor HTU21D
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!htu.begin()) {
    Serial.println("[ERROR] No se encontró el sensor HTU21D. Revisá conexiones SDA/SCL.");
  } else {
    float temp = htu.readTemperature();
    float hum = htu.readHumidity();
    Serial.printf("[SENSOR] Temperatura: %.2f °C | Humedad: %.2f %%\n", temp, hum);
  }

  // Leer estado de la sentina (LOW = flotador cerrado / agua alta)
  int estadoSentina = digitalRead(PIN_SENTINA);
  if (estadoSentina == LOW) {
    Serial.println("[SENTINA] ESTADO: ALERTA DE AGUA (Circuito Cerrado)");
  } else {
    Serial.println("[SENTINA] ESTADO: Seco (Normal)");
  }

  Serial.println("[SISTEMA] Tareas completadas. Preparando Deep Sleep...");

  // Configurar despertar por tiempo (ej. 30 segundos)
  esp_sleep_enable_timer_wakeup(TIEMPO_SUENO_SEGUNDOS * FACTOR_CONVERSION_US);

  // Configurar despertar por flotador (EXT0 despierta si el pin pasa a LOW/0V)
  esp_sleep_enable_ext0_wakeup(PIN_SENTINA, 0);

  Serial.println("[SISTEMA] Entrando en Deep Sleep ahora.");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {
  // En Deep Sleep nunca se llega al loop()
}