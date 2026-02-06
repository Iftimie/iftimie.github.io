#include <Arduino.h>
#include <RadioLib.h>
#include "pins.h"
#include "config.h"
#include "protocol.h"
#include "utils.h"
#include "leds.h"
#include "touch_io.h"
#include "radio.h"

// https://amoghdesai.com/uncategorized/cheapest-lora-transceiver-with-the-llcc68-and-esp32%E2%80%91c3-supermini-plus/
// https://fcc.report/FCC-ID/2ATPO-RA-01SC/5342082.pdf
// https://www.mouser.com/pdfDocs/DS_LLCC68_V10-2.pdf?srsltid=AfmBOopJTulMF7V0ETsfU0KjyOFxj9cEhIapcI3SRSC6vgiyycHJfP2A

static uint32_t lastTxMs = 0;
static uint32_t lastRxWindowMs = 0;

static uint16_t txSeqPing  = 0;
static uint16_t txSeqTouch = 0;

static int32_t lastRxPingSeq  = -1;
static int32_t lastRxTouchSeq = -1;

static void sendPing() {
  PktHdr h;
  h.type = PKT_PING;
  h.src  = MY_ID;
  h.dst  = PEER_ID;
  h.seq  = txSeqPing++;
  logf("tx: send PING seq=%u to=%u", (unsigned)h.seq, (unsigned)h.dst);
  radioTransmit((uint8_t*)&h, sizeof(h));
}

static void sendTouchUntilAck(const TouchSeg *segs, uint8_t segCount) {
  if (segCount == 0) return;

  TouchPacketFixed pkt;
  memset(&pkt, 0, sizeof(pkt));

  pkt.h.type = PKT_TOUCH;
  pkt.h.src  = MY_ID;
  pkt.h.dst  = PEER_ID;
  pkt.h.seq  = txSeqTouch++;
  pkt.segCount = segCount;

  memcpy(pkt.segs, segs, segCount * sizeof(TouchSeg));

  uint32_t overallStart = 0;
  logf("tx: send TOUCH seq=%u segs=%u to=%u", (unsigned)pkt.h.seq, (unsigned)segCount, (unsigned)pkt.h.dst);
  for (uint8_t i = 0; i < segCount; i++) {
    logf("tx: seg[%u] state=%u ticks=%u", (unsigned)i, (unsigned)pkt.segs[i].state, (unsigned)pkt.segs[i].durationTicks);
  }

  while (true) {
    logf("tx: transmit TOUCH seq=%u", (unsigned)pkt.h.seq);
    size_t pktLen = sizeof(PktHdr) + 1 + (size_t)segCount * sizeof(TouchSeg);
    radioTransmit((uint8_t*)&pkt, pktLen);
    if (overallStart == 0) overallStart = millis();

    uint32_t t0 = millis();
    logf("tx: wait ACK for seq=%u (window=%u ms)", (unsigned)pkt.h.seq, (unsigned)ACK_WAIT_MS);
    while (millis() - t0 < ACK_WAIT_MS) {
      PktHdr rh;
      int16_t st = radioReceive((uint8_t*)&rh, sizeof(rh), 140);
      if (st != RADIOLIB_ERR_NONE) continue;

      if (rh.type != PKT_TOUCH_ACK) continue;
      if (rh.dst  != MY_ID)        continue;
      if (rh.src  != PEER_ID)      continue;
      if (rh.seq  != pkt.h.seq)    continue;
      logf("rx: ACK received seq=%u from=%u rssi=%.1f snr=%.1f", (unsigned)rh.seq, (unsigned)rh.src, radioGetRSSI(), radioGetSNR());
      return; // ACK received
    }

    if (overallStart != 0 && (millis() - overallStart >= ACK_TOTAL_TIMEOUT_MS)) {
      logf("tx: ACK total timeout for seq=%u (elapsed=%u ms)", (unsigned)pkt.h.seq, (unsigned)(millis() - overallStart));
      return;
    }
    logf("tx: ACK wait timed out, retry in %u ms", (unsigned)RETRY_GAP_MS);
    delay(RETRY_GAP_MS);
  }
}

static void handleIncomingIdle(uint32_t timeoutMs) {
  TouchPacketFixed rxp;
  int16_t st = radioReceive((uint8_t*)&rxp, sizeof(rxp), timeoutMs);
  if (st != RADIOLIB_ERR_NONE) return;

  PktHdr &h = rxp.h;
  logf("rx: packet type=%u seq=%u from=%u to=%u", (unsigned)h.type, (unsigned)h.seq, (unsigned)h.src, (unsigned)h.dst);
  if (h.dst != MY_ID || h.src != PEER_ID) return;

  if (h.type == PKT_PING) {
    if ((int32_t)h.seq == lastRxPingSeq) return;
    lastRxPingSeq = h.seq;

    float rssi = radioGetRSSI();
    float snr  = radioGetSNR();
    float q = combineQuality(mapRSSI(rssi), mapSNR(snr));
    logf("rx: PING seq=%u from=%u rssi=%.1f snr=%.1f q=%.2f", (unsigned)h.seq, (unsigned)h.src, rssi, snr, q);
    triggerSignalLed(q);
    return;
  }

  if (h.type == PKT_TOUCH) {
    logf("rx: TOUCH seq=%u from=%u segs=%u", (unsigned)h.seq, (unsigned)h.src, (unsigned)rxp.segCount);
    PktHdr ack;
    ack.type = PKT_TOUCH_ACK;
    ack.src  = MY_ID;
    ack.dst  = PEER_ID;
    ack.seq  = h.seq;
    logf("tx: send ACK seq=%u to=%u", (unsigned)ack.seq, (unsigned)ack.dst);
    radioTransmit((uint8_t*)&ack, sizeof(ack));

    if ((int32_t)h.seq == lastRxTouchSeq) return;
    lastRxTouchSeq = h.seq;

    uint8_t segCount = rxp.segCount;
    if (segCount > MAX_SEGMENTS) return;

    for (uint8_t i = 0; i < segCount; i++) {
      logf("rx: seg[%u] state=%u ticks=%u", (unsigned)i, (unsigned)rxp.segs[i].state, (unsigned)rxp.segs[i].durationTicks);
    }
    playbackOnOnlineLed(rxp.segs, segCount);
    return;
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t __sw = millis();
  while (!Serial && (millis() - __sw) < 3000) { delay(10); }
  logf("boot: usb-serial %s", Serial ? "connected" : "not connected");
  logf("boot: build %s %s", __DATE__, __TIME__);

  pinMode(SIGNAL_LED_PIN, OUTPUT);
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  pinMode(ONLINE_LED_PIN, OUTPUT);

  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  analogWrite(BUILTIN_LED_PIN, chooseBuiltinLevel(PEER_ID));
  setOnlineIdle();
  analogWrite(SIGNAL_LED_PIN, 0);

  if (!radioInit()) {
    logf("radio: init failed");
    while (true) { delay(1000); }
  }
  logf("radio: init OK, my_id=%u peer_id=%u", (unsigned)MY_ID, (unsigned)PEER_ID);
}

void loop() {
  if (touchRead()) {
    logf("touch: start detected");
    TouchSeg segs[MAX_SEGMENTS];
    uint8_t segCount = recordTouchPattern(segs, MAX_SEGMENTS);
    logf("touch: pattern recorded segCount=%u", (unsigned)segCount);
    sendTouchUntilAck(segs, segCount);

    setOnlineIdle();
    delay(30);
    return;
  }

  if (everyMs(lastTxMs, TX_INTERVAL_MS)) {
    sendPing();
  }

  if (everyMs(lastRxWindowMs, RX_WINDOW_MS + 80)) {
    logf("rx: open idle window=%u ms", (unsigned)RX_WINDOW_MS);
    handleIncomingIdle(RX_WINDOW_MS);
  }

  updateSignalLed();
}
