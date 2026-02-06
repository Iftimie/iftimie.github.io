#pragma once
#include <stdint.h>
#include "protocol.h"

bool touchRead();
uint8_t recordTouchPattern(TouchSeg *outSegs, uint8_t maxSegs);
