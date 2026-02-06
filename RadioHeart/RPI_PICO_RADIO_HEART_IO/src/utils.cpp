#include "utils.h"
#include <stdarg.h>
#include <stdio.h>

bool everyMs(uint32_t &last, uint32_t period) {
  uint32_t now = millis();
  if (now - last >= period) { last = now; return true; }
  return false;
}

void logf(const char *fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  Serial.print('[');
  Serial.print(millis());
  Serial.print("] ");
  Serial.println(buf);
}
