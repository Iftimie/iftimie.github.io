#include <Arduino.h>
#include "pins.h"
#include "config.h"
#include "touch_io.h"
#include "utils.h"

bool touchRead() {
  int v = digitalRead(TOUCH_PIN);
  return TOUCH_ACTIVE_HIGH ? (v == HIGH) : (v == LOW);
}

uint8_t recordTouchPattern(TouchSeg *outSegs, uint8_t maxSegs) {
  uint8_t segCount = 0;

  bool cur = touchRead();
  uint32_t segStart = millis();
  uint32_t lastTouchedMs = cur ? millis() : 0;
  uint32_t recordStartMs = millis();

  logf("touch: record start, initial=%s", cur ? "HIGH" : "LOW");

  auto pushSeg = [&](bool state, uint32_t durMs) {
    if (segCount >= maxSegs) return;
    uint32_t ticks = (durMs + (TOUCH_TICK_MS / 2)) / TOUCH_TICK_MS;
    if (ticks == 0) ticks = 1;
    if (ticks > 65535) ticks = 65535;
    outSegs[segCount].state = state ? 1 : 0;
    outSegs[segCount].durationTicks = (uint16_t)ticks;
    segCount++;
    logf("touch: push seg #%u state=%u durMs=%lu ticks=%u", (unsigned)segCount, (unsigned)outSegs[segCount-1].state, (unsigned long)durMs, (unsigned)outSegs[segCount-1].durationTicks);
  };

  while (true) {
    bool now = touchRead();
    uint32_t ms = millis();

    if (ms - recordStartMs >= TOUCH_MAX_RECORD_MS) {
      uint32_t dur = ms - segStart;
      pushSeg(cur, dur);
      logf("touch: stop reason=max_record (%u ms)", (unsigned)(ms - recordStartMs));
      break;
    }

    if (now) lastTouchedMs = ms;

    if (now != cur) {
      uint32_t dur = ms - segStart;
      pushSeg(cur, dur);
      cur = now;
      segStart = ms;
      logf("touch: transition to %s", cur ? "HIGH" : "LOW");
    }

    if (!now && lastTouchedMs != 0 && (ms - lastTouchedMs >= TOUCH_END_GAP_MS)) {
      uint32_t dur = ms - segStart;
      pushSeg(cur, dur);
      logf("touch: stop reason=end_gap after last touch (%u ms)", (unsigned)(ms - lastTouchedMs));
      break;
    }

    if (!now && lastTouchedMs == 0 && (ms - segStart >= TOUCH_END_GAP_MS)) {
      segCount = 0;
      logf("touch: stop reason=initial_idle (no touch detected)");
      break;
    }

    delay(1);

    if (segCount >= maxSegs) {
      uint32_t dur = ms - segStart;
      pushSeg(cur, dur);
      logf("touch: stop reason=max_segments (%u)", (unsigned)maxSegs);
      break;
    }
  }

  if (segCount > 0 && outSegs[0].state == 0) {
    for (uint8_t i = 1; i < segCount; i++) outSegs[i - 1] = outSegs[i];
    segCount--;
    logf("touch: dropped leading idle segment, new count=%u", (unsigned)segCount);
  }

  logf("touch: record done, segCount=%u", (unsigned)segCount);
  return segCount;
}
