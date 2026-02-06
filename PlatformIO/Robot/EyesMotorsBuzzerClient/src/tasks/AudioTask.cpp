#include "TaskCommon.h"
#include "AudioTask.h"

// Buzzer (GPIO4) - NEW LEDC API (pin-based)
#define BUZZER_PIN 4
static const int BUZZER_RES  = 10;   // 10-bit (0..1023)
static const int BUZZER_DUTY = 512;  // ~50%

static void toneOn(int freqHz) {
  if (freqHz <= 0) return;
  ledcWriteTone(BUZZER_PIN, freqHz);
  ledcWrite(BUZZER_PIN, BUZZER_DUTY);
}
static void toneOff() {
  ledcWrite(BUZZER_PIN, 0);
  ledcWriteTone(BUZZER_PIN, 0);
}

struct NoteMap { const char* name; int freq; };

static const NoteMap NOTE_TABLE[] = {
  {"C3", 131}, {"CS3", 139}, {"D3", 147}, {"DS3", 156}, {"E3", 165},
  {"F3", 175}, {"FS3", 185}, {"G3", 196}, {"GS3", 208}, {"A3", 220}, {"AS3", 233}, {"B3", 247},
  {"C4", 262}, {"CS4", 277}, {"D4", 294}, {"DS4", 311}, {"E4", 330},
  {"F4", 349}, {"FS4", 370}, {"G4", 392}, {"GS4", 415}, {"A4", 440}, {"AS4", 466}, {"B4", 494},
  {"C5", 523}, {"CS5", 554}, {"D5", 587}, {"DS5", 622}, {"E5", 659},
  {"F5", 698}, {"FS5", 740}, {"G5", 784}, {"GS5", 831}, {"A5", 880}, {"AS5", 932}, {"B5", 988},
  {"REST", 0},
};

static int noteToFreq(const char* note) {
  if (!note) return 0;
  char up[8];
  size_t n = strlen(note);
  if (n >= sizeof(up)) n = sizeof(up) - 1;
  for (size_t i = 0; i < n; i++) up[i] = toupper((unsigned char)note[i]);
  up[n] = '\0';

  for (const auto& e : NOTE_TABLE) {
    if (strcmp(up, e.name) == 0) return e.freq;
  }
  return atoi(note);
}

static volatile bool g_audio_stop = false;

static bool parsePair(const char* tok, int& freq, int& ms) {
  const char* comma = strchr(tok, ',');
  if (!comma) return false;

  char note[8] = {0};
  size_t nlen = (size_t)(comma - tok);
  if (nlen >= sizeof(note)) nlen = sizeof(note) - 1;
  memcpy(note, tok, nlen);
  note[nlen] = '\0';

  freq = noteToFreq(skipSpaces(note));
  ms = atoi(skipSpaces(comma + 1));
  if (ms < 0) ms = 0;
  return true;
}

void taskAudio(void* /*arg*/) {
  ledcAttach(BUZZER_PIN, 2000, BUZZER_RES);
  toneOff();

  AudioCmd cmd{};
  uint32_t current_audio_id = 0;

  for (;;) {
    if (xQueueReceive(g_audio_q, &cmd, portMAX_DELAY) != pdTRUE) continue;

    const char* s = skipSpaces(cmd.text);
    Serial.printf("[AUDIO] rx: %s\n", s);

    if (strcasecmp(s, "audio:stop") == 0) {
      g_audio_stop = true;
      toneOff();
      // signal done for current audio if any
      if (current_audio_id != 0 && g_done_q) {
        DoneEvent de{current_audio_id};
        xQueueSend(g_done_q, &de, 0);
        current_audio_id = 0;
      }
      continue;
    }

    if (!startsWithNoCase(s, "audio:")) continue;

    g_audio_stop = true;
    vTaskDelay(pdMS_TO_TICKS(5));
    g_audio_stop = false;

    const char* payload = skipSpaces(afterPrefix(s, "audio:"));

    char buf[CMD_MAX_LEN];
    strncpy(buf, payload, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    // record id for this playback
    current_audio_id = cmd.id;

    char* saveptr = nullptr;
    char* tok = strtok_r(buf, ";", &saveptr);

    while (tok) {
      if (g_audio_stop) break;

      tok = (char*)skipSpaces(tok);
      if (*tok == '\0') { tok = strtok_r(nullptr, ";", &saveptr); continue; }

      int freq = 0, ms = 0;
      if (!parsePair(tok, freq, ms)) { tok = strtok_r(nullptr, ";", &saveptr); continue; }

      if (freq > 0) toneOn(freq);
      else toneOff();

      int remaining = ms;
      while (remaining > 0 && !g_audio_stop) {
        int chunk = (remaining > 20) ? 20 : remaining;
        vTaskDelay(pdMS_TO_TICKS(chunk));
        remaining -= chunk;
      }

      toneOff();
      vTaskDelay(pdMS_TO_TICKS(5));
      tok = strtok_r(nullptr, ";", &saveptr);
    }

    toneOff();
    // playback finished -> signal done if requested
    if (current_audio_id != 0 && g_done_q) {
      DoneEvent de{current_audio_id};
      xQueueSend(g_done_q, &de, 0);
      current_audio_id = 0;
    }
  }
}
