#include "protocol.h"

float mapRSSI(float rssi) {
  if (rssi <= -120) return 0.0f;
  if (rssi >= -60)  return 1.0f;
  return (rssi + 120.0f) / 60.0f;
}

float mapSNR(float snr) {
  if (snr <= -10) return 0.0f;
  if (snr >= 10)  return 1.0f;
  return (snr + 10.0f) / 20.0f;
}

float combineQuality(float rssiNorm, float snrNorm) {
  return 0.6f * rssiNorm + 0.4f * snrNorm;
}
