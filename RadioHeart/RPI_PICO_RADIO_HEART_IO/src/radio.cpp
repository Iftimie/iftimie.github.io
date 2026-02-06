#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "pins.h"
#include "config.h"
#include "radio.h"

static LLCC68 radio = LLCC68(new Module(
  LLCC68_NSS, // NSS
  LLCC68_DIO1,// DIO1
  LLCC68_RST, // RST
  LLCC68_BUSY // BUSY
));

bool radioInit() {
  SPI.begin();

  radio.setDio2AsRfSwitch(true);

  if (radio.begin(433.0) != RADIOLIB_ERR_NONE) {
    return false;
  }

  radio.setOutputPower(TX_POWER_DBM);
  radio.setBandwidth(250.0);
  radio.setSpreadingFactor(7);
  radio.setCodingRate(5);
  radio.setPreambleLength(8);
  radio.setCRC(true);
  radio.setSyncWord(0x12);

  return true;
}

void radioTransmit(const uint8_t *buf, size_t len) {
  radio.transmit((uint8_t*)buf, len);
}

int16_t radioReceive(uint8_t *buf, size_t len, uint32_t timeoutMs) {
  return radio.receive(buf, len, timeoutMs);
}

float radioGetRSSI() {
  return radio.getRSSI();
}

float radioGetSNR() {
  return radio.getSNR();
}
