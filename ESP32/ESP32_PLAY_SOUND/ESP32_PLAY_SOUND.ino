#include <Arduino.h>

#define BUZZER_PIN 14   // Safe pin if SD card not used

static const int BUZZER_RES  = 10;   // PWM resolution (bits)
static const int BUZZER_DUTY = 512;  // ~50% duty (0–1023)

void toneOn(int freqHz) {
  ledcWriteTone(BUZZER_PIN, freqHz);
  ledcWrite(BUZZER_PIN, BUZZER_DUTY);
}

void toneOff() {
  ledcWrite(BUZZER_PIN, 0);     // silence
  ledcWriteTone(BUZZER_PIN, 0);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // NEW API: attach PWM directly to pin
  ledcAttach(BUZZER_PIN, 2000, BUZZER_RES);

  Serial.println("ESP32-CAM buzzer test (LEDC v3 API)");
}

void loop() {
  // Beep beep
  toneOn(2000); delay(150);
  toneOff();    delay(100);
  toneOn(2600); delay(150);
  toneOff();    delay(600);

  // Frequency sweep (confirms passive buzzer)
  for (int f = 800; f <= 3000; f += 150) {
    toneOn(f);
    delay(40);
  }

  toneOff();
  delay(1200);
}
