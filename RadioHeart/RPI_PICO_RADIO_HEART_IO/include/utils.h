#pragma once
#include <Arduino.h>

bool everyMs(uint32_t &last, uint32_t period);

// Lightweight printf-style logger with millis prefix.
void logf(const char *fmt, ...);
