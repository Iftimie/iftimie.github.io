#pragma once
#include <stdint.h>
#include <stddef.h>

bool radioInit();
void radioTransmit(const uint8_t *buf, size_t len);
int16_t radioReceive(uint8_t *buf, size_t len, uint32_t timeoutMs);
float radioGetRSSI();
float radioGetSNR();
