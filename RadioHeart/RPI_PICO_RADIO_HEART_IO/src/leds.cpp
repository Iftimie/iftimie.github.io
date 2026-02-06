#include <Arduino.h>
#include "pins.h"
#include "config.h"
#include "leds.h"

static uint32_t fadeStartMs = 0;
static uint8_t  fadePeak   = 0;
static bool     fading     = false;

static inline uint8_t computeFade() {
  uint32_t elapsed = millis() - fadeStartMs;
  if (elapsed >= FADE_TIME_MS) {
    fading = false;
    return 0;
  }
  float t = 1.0f - (float)elapsed / (float)FADE_TIME_MS; // 1 -> 0
  float gamma = t * t; // exponential-ish
  return (uint8_t)(fadePeak * gamma);
}

void triggerSignalLed(float quality) {
  if (fading) return;
  uint8_t peak = LED_MIN + (uint8_t)(quality * (LED_MAX - LED_MIN));
  fadePeak    = peak;
  fadeStartMs = millis();
  fading      = true;
}

void updateSignalLed() {
  if (!fading) {
    analogWrite(SIGNAL_LED_PIN, 0);
    return;
  }
  analogWrite(SIGNAL_LED_PIN, computeFade());
}

uint8_t chooseBuiltinLevel(uint8_t peer) {
  if (peer == 1) return BUILTIN_PEER1;
  if (peer == 2) return BUILTIN_PEER2;
  return BUILTIN_OTHER;
}

void setOnlineIdle() {
  analogWrite(ONLINE_LED_PIN, ONLINE_PWM);
}

void playbackOnOnlineLed(const TouchSeg *segs, uint8_t segCount) {
  analogWrite(ONLINE_LED_PIN, 0);
  delay(PLAYBACK_START_OFF_MS);

  for (uint8_t i = 0; i < segCount; i++) {
    uint8_t state = segs[i].state ? 1 : 0;
    uint16_t ticks = segs[i].durationTicks;
    uint32_t durMs = (uint32_t)ticks * (uint32_t)TOUCH_TICK_MS;

    analogWrite(ONLINE_LED_PIN, state ? ONLINE_PWM : 0);

    uint32_t t0 = millis();
    while (millis() - t0 < durMs) {
      delay(1);
    }
  }

  setOnlineIdle();
}
