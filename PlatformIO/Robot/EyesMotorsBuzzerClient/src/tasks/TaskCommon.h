#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Command sizes and queue lengths (shared)
static constexpr size_t CMD_MAX_LEN   = 160;
static constexpr size_t CMD_QUEUE_LEN = 10;
static constexpr size_t AUDIO_QUEUE_LEN = 6;
static constexpr size_t MOTOR_QUEUE_LEN = 6;

// Command structs include an optional `id` (0 == no ack requested).
struct EyesCmd  { char text[CMD_MAX_LEN]; uint32_t id; };
struct AudioCmd { char text[CMD_MAX_LEN]; uint32_t id; };
struct MotorCmd { char text[CMD_MAX_LEN]; uint32_t id; };

struct DoneEvent { uint32_t id; };

extern QueueHandle_t g_cmd_q;
extern QueueHandle_t g_audio_q;
extern QueueHandle_t g_motor_q;
extern QueueHandle_t g_done_q;

// Lightweight helpers (inline to allow usage across TUs)
static inline const char* skipSpaces(const char* s) {
  while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
  return s;
}

static inline bool startsWithNoCase(const char* s, const char* prefix) {
  while (*prefix && *s) {
    if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return false;
    s++; prefix++;
  }
  return *prefix == '\0';
}

static inline const char* afterPrefix(const char* s, const char* prefix) {
  return s + strlen(prefix);
}
