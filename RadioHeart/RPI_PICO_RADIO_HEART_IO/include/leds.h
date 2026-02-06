#pragma once
#include <stdint.h>
#include "protocol.h"

void triggerSignalLed(float quality);
void updateSignalLed();
uint8_t chooseBuiltinLevel(uint8_t peer);
void setOnlineIdle();
void playbackOnOnlineLed(const TouchSeg *segs, uint8_t segCount);
