#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===== Oscilloscope trace buffer =====
int16_t trace[SCREEN_WIDTH];

// “Audio-ish” oscillators
float ph1 = 0.0f, ph2 = 0.0f, ph3 = 0.0f;
float f1 = 0.55f, f2 = 0.13f, f3 = 0.03f;   // base phase steps (bigger = faster)

// Amplitude envelope (like loudness)
float env = 10.0f;
float envVel = 0.0f;

static inline int16_t clampi(int16_t v, int16_t lo, int16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void setup() {
  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  randomSeed(analogRead(A0) + analogRead(A1) * 31UL);

  for (int i = 0; i < SCREEN_WIDTH; i++) trace[i] = SCREEN_HEIGHT / 2;
}

void loop() {
  // Faster refresh (more “audio” feel). Lower = faster.
  delay(10); // ~100 FPS-ish on Uno; adjust 6..15

  // Random walk on “loudness” to mimic audio dynamics
  envVel += random(-8, 9) / 50.0f;   // push it around
  envVel *= 0.90f;                  // damping
  env += envVel;
  if (env < 6) env = 6;
  if (env > 26) env = 26;

  // Slight frequency wobble (like vibrato / messy audio)
  f1 += random(-3, 4) / 2000.0f;
  f2 += random(-2, 3) / 5000.0f;
  if (f1 < 0.25f) f1 = 0.25f;
  if (f1 > 1.40f) f1 = 1.40f; // faster cap
  if (f2 < 0.05f) f2 = 0.05f;
  if (f2 > 0.35f) f2 = 0.35f;

  // Create an “audio-ish” sample:
  // - main tone (ph1)
  // - second tone (ph2) for richness
  // - slow beat (ph3) to modulate amplitude
  float beat = 0.5f + 0.5f * sinf(ph3);        // 0..1
  float sample =
      sinf(ph1) +
      0.55f * sinf(ph2 + 1.1f) +
      0.15f * sinf(ph1 * 3.0f);                // a bit of harmonic

  // Update phases (bigger steps = faster waves)
  ph1 += f1;
  ph2 += f2 * 3.2f;
  ph3 += f3; // slow beat

  // Noise for “sound”
  int noise = random(-4, 5);

  int mid = SCREEN_HEIGHT / 2;
  int16_t yNew = (int16_t)(mid + sample * env * (0.55f + 0.65f * beat) + noise);
  yNew = clampi(yNew, 0, SCREEN_HEIGHT - 1);

  // Shift trace
  for (int i = 0; i < SCREEN_WIDTH - 1; i++) trace[i] = trace[i + 1];
  trace[SCREEN_WIDTH - 1] = yNew;

  // Render (NO GRID)
  display.clearDisplay();

  // Draw waveform (connected line)
  for (int x = 1; x < SCREEN_WIDTH; x++) {
    display.drawLine(x - 1, trace[x - 1], x, trace[x], WHITE);
  }

  // Bright “playhead” dot at the newest sample
  display.fillCircle(SCREEN_WIDTH - 1, trace[SCREEN_WIDTH - 1], 1, WHITE);

  display.display();
}
