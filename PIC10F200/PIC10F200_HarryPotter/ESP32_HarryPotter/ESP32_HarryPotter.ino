#include <Arduino.h>

#define BUZZER_PIN 14

static const int BUZZER_RES  = 10;
static const int BUZZER_DUTY = 512;
struct Step {
  uint16_t hz;     // 0 = rest
  uint16_t ms;
};

// Optional tiny gap between notes for articulation
static const uint16_t GAP_MS = 12;
// ------------ timing ------------
static const uint16_t UNIT_MS = 180;          // 1 duration "unit" in ms (tweak tempo)
static const uint8_t  QUARTER = 3;            // matches your ASM quarter_note
static const uint8_t  HALF    = 6;
static const uint8_t  DOTTED_HALF = 9;

// Convert duration-units -> milliseconds
static inline uint16_t durMs(uint8_t units) {
  // units * UNIT_MS fits in uint16_t for these values
  return (uint16_t)units * UNIT_MS;
}

// ------------ notes (Hz) ------------
// These are standard equal-temperament frequencies.
// I mapped your PIC note names to reasonable octaves for a buzzer melody.
// If anything sounds too high/low, we can shift an octave by *2 or /2.

static const uint16_t E1_HZ  = 330;  // E4
static const uint16_t A1_HZ  = 440;  // A4
static const uint16_t C1_HZ  = 523;  // C5
static const uint16_t H1_HZ  = 494;  // B4  (H = B)
static const uint16_t D2_HZ  = 587;  // D5
static const uint16_t G1_HZ  = 392;  // G4
static const uint16_t AS1_HZ = 466;  // A#4 / Bb4
static const uint16_t E2_HZ  = 659;  // E5
static const uint16_t G2_HZ  = 784;  // G5
static const uint16_t FS2_HZ = 740;  // F#5
static const uint16_t CS1_HZ = 554;  // C#5
static const uint16_t F2_HZ  = 698;  // F5
static const uint16_t EB2_HZ = 622;  // Eb5 / D#5
static const uint16_t EB1_HZ = 311;  // Eb4 / D#4 (lower Eb)

// ------------ buzzer helpers ------------
void toneOn(int freqHz) {
  if (freqHz <= 0) return;
  ledcWriteTone(BUZZER_PIN, freqHz);
  ledcWrite(BUZZER_PIN, BUZZER_DUTY);
}

void toneOff() {
  ledcWrite(BUZZER_PIN, 0);
  ledcWriteTone(BUZZER_PIN, 0);
}



static void playStep(const Step& s) {
  if (s.hz == 0) {
    toneOff();
    delay(s.ms);
    return;
  }
  toneOn(s.hz);
  if (s.ms > GAP_MS) delay(s.ms - GAP_MS);
  toneOff();
  delay(GAP_MS);
}

// ------------ the song (converted from your ASM calls) ------------
const Step SONG[] = {
  // MAIN_LOOP
  { E1_HZ,  durMs(QUARTER) },
  { A1_HZ,  durMs(QUARTER) },
  { C1_HZ,  durMs(QUARTER) },
  { H1_HZ,  durMs(QUARTER) },

  { A1_HZ,  durMs(HALF)    },
  { E2_HZ,  durMs(QUARTER) },

  { D2_HZ,  durMs(DOTTED_HALF) },
  { H1_HZ,  durMs(DOTTED_HALF) },

  // second line of Hedwig’s Theme only
  { A1_HZ,  durMs(QUARTER) },
  { C1_HZ,  durMs(QUARTER) },
  { H1_HZ,  durMs(QUARTER) },
  { G1_HZ,  durMs(HALF)    },
  { AS1_HZ, durMs(QUARTER) },

  { E1_HZ,  durMs(DOTTED_HALF) },
  { E1_HZ,  durMs(QUARTER)     },
  { E1_HZ,  durMs(QUARTER)     },

  // third line
  { A1_HZ,  durMs(QUARTER) },
  { C1_HZ,  durMs(QUARTER) },
  { H1_HZ,  durMs(QUARTER) },
  { A1_HZ,  durMs(HALF)    },

  { E2_HZ,  durMs(QUARTER) },
  { G2_HZ,  durMs(HALF)    },

  { FS2_HZ, durMs(QUARTER) },
  { FS2_HZ, durMs(HALF)    },

  { CS1_HZ, durMs(QUARTER) },

  // forth line
  { F2_HZ,  durMs(QUARTER) },
  { E2_HZ,  durMs(QUARTER) },
  { EB2_HZ, durMs(QUARTER) },
  { EB1_HZ, durMs(HALF)    },

  { C1_HZ,  durMs(QUARTER) },
  { A1_HZ,  durMs(DOTTED_HALF) },
  { A1_HZ,  durMs(QUARTER) },
  { C1_HZ,  durMs(QUARTER) },

  // fifth line
  { E2_HZ,  durMs(HALF)    },
  { C1_HZ,  durMs(QUARTER) },
  { E2_HZ,  durMs(HALF)    },
  { C1_HZ,  durMs(QUARTER) },

  { F2_HZ,  durMs(HALF)    },
  { E2_HZ,  durMs(QUARTER) },
  { EB2_HZ, durMs(HALF)    },
  { H1_HZ,  durMs(QUARTER) },

  { C1_HZ,  durMs(QUARTER) },
  { E2_HZ,  durMs(QUARTER) },
  { EB2_HZ, durMs(QUARTER) },
  { EB1_HZ, durMs(HALF)    },

  { E1_HZ,  durMs(QUARTER) },
  { E2_HZ,  durMs(DOTTED_HALF) },
  { E2_HZ,  durMs(HALF)    },
  { C1_HZ,  durMs(QUARTER) },

  // next line
  { E2_HZ,  durMs(HALF)    },
  { C1_HZ,  durMs(QUARTER) },
  { E2_HZ,  durMs(HALF)    },
  { C1_HZ,  durMs(QUARTER) },
};

const size_t SONG_LEN = sizeof(SONG) / sizeof(SONG[0]);

void setup() {
  Serial.begin(115200);
  delay(200);

  ledcAttach(BUZZER_PIN, 2000, BUZZER_RES);
  Serial.println("ESP32 Hedwig buzzer player");
}

void loop() {
  for (size_t i = 0; i < SONG_LEN; i++) {
    playStep(SONG[i]);
  }
  delay(1200);
}
