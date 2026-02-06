#include <Arduino.h>

const int LED_PIN = 2; // change to your board's LED pin (ESP32-CAM often uses 4 for flash)

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}