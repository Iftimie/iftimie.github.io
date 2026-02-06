#pragma once
#include <stdint.h>
#include "config.h"

struct __attribute__((packed)) TouchSeg {
  uint8_t  state;         // 0/1
  uint16_t durationTicks; // units of TOUCH_TICK_MS
};

enum PacketType : uint8_t {
  PKT_PING      = 1,
  PKT_TOUCH     = 2,
  PKT_TOUCH_ACK = 3
};

struct __attribute__((packed)) PktHdr {
  uint8_t  type;  // PacketType
  uint8_t  src;
  uint8_t  dst;
  uint16_t seq;
};

struct __attribute__((packed)) TouchPacketFixed {
  PktHdr   h;
  uint8_t  segCount;
  TouchSeg segs[MAX_SEGMENTS];
};

float mapRSSI(float rssi);
float mapSNR(float snr);
float combineQuality(float rssiNorm, float snrNorm);
