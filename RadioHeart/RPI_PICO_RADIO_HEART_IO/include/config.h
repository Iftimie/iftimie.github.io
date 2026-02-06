#pragma once
#include <stdint.h>

// Device IDs via -D macros
#ifndef MY_ID
#define MY_ID 2
#endif
#ifndef PEER_ID
#define PEER_ID 1
#endif
#ifndef TOUCH_ACTIVE_HIGH
#define TOUCH_ACTIVE_HIGH 1
#endif

// Radio timing
constexpr uint32_t TX_INTERVAL_MS = 1500;  // was 500
constexpr uint32_t RX_WINDOW_MS   = 1200;

// Touch recording
constexpr uint32_t TOUCH_END_GAP_MS    = 2000;
constexpr uint32_t TOUCH_MAX_RECORD_MS = 12000;
constexpr uint16_t TOUCH_TICK_MS       = 10;
constexpr uint8_t  MAX_SEGMENTS        = 80;

// Touch send/ack
constexpr uint32_t ACK_WAIT_MS           = 1000;
constexpr uint32_t RETRY_GAP_MS          = 140;
constexpr uint32_t ACK_TOTAL_TIMEOUT_MS  = 15000;
constexpr uint8_t  TX_POWER_DBM          = 10;

// Signal LED fade
constexpr uint32_t FADE_TIME_MS = 1500;
constexpr uint8_t LED_MIN = 5;
constexpr uint8_t LED_MAX = 180;

// Built-in LED levels by peer
constexpr uint8_t BUILTIN_PEER1 = 180; // brighter
constexpr uint8_t BUILTIN_PEER2 = 35;  // dimmer
constexpr uint8_t BUILTIN_OTHER = 80;  // fallback

// Online LED
constexpr uint8_t ONLINE_PWM = 40;
constexpr uint32_t PLAYBACK_START_OFF_MS = 50;
