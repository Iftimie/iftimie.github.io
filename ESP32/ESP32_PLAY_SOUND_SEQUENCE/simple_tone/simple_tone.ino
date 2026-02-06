#include <Arduino.h>

// ESP32-CAM: GPIO14 is usually safe if you're not using SD card
#define BUZZER_PIN 14

static const int BUZZER_RES  = 10;   // 10-bit (0..1023)
static const int BUZZER_DUTY = 512;  // ~50%

// ----- Notes (Hz) -----
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494

#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_G5  784

// ----- Tone helpers -----
void toneOn(int freqHz) {
  if (freqHz <= 0) return;
  ledcWriteTone(BUZZER_PIN, freqHz);
  ledcWrite(BUZZER_PIN, BUZZER_DUTY);
}

void toneOff() {
  ledcWrite(BUZZER_PIN, 0);
  ledcWriteTone(BUZZER_PIN, 0);
}

// ----- Sequence format -----
struct Step {
  int freq;       // Hz; 0 = rest
  uint16_t ms;    // duration in milliseconds
};

// Example sequence (replace with yours)
const Step SEQ[] = {
  { NOTE_E4, 200 }, { 0,  80 },
  { NOTE_E4, 200 }, { 0,  80 },
  { NOTE_E4, 200 }, { 0, 250 },

  { NOTE_C4, 200 },
  { NOTE_E4, 200 },
  { NOTE_G4, 400 },
  { 0, 300 },

  // "Chord" approximation for a buzzer: play notes quickly one after another
  { NOTE_C4, 120 }, { NOTE_E4, 120 }, { NOTE_G4, 110 },
  { 0, 150 },
  { NOTE_D4, 120 }, { NOTE_F4, 120 }, { NOTE_A4, 110 },
};

const size_t SEQ_LEN = sizeof(SEQ) / sizeof(SEQ[0]);

void playSequence(const Step* seq, size_t len, int gapMs = 10) {
  for (size_t i = 0; i < len; i++) {
    const int f = seq[i].freq;
    const int d = seq[i].ms;

    if (f > 0) {
      toneOn(f);
      delay(max(0, d - gapMs));
      toneOff();
      if (gapMs > 0) delay(gapMs);
    } else {
      toneOff();
      delay(d);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // LEDC v3: attach channel-less PWM to pin
  ledcAttach(BUZZER_PIN, 2000, BUZZER_RES);

  Serial.println("Buzzer sequence player ready.");
}

void loop() {
  playSequence(SEQ, SEQ_LEN, 8);
  delay(800);
}
