#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// =====================================================
// DEVICE IDENTITY (EDIT THESE PER DEVICE)
// =====================================================
static const uint8_t MY_ID   = 1;
static const uint8_t PEER_ID = 2;

// =====================================================
// HARDWARE PINS (RP2040 / Pico)
// =====================================================
#define SIGNAL_LED_PIN    0   // GP0: packet signal LED (active HIGH)  [PWM]
#define BUILTIN_LED_PIN  25   // GP25: Pico built-in LED (active HIGH) [PWM]
#define ONLINE_LED_PIN   28   // GP28: "online" LED (active HIGH)      [PWM]
#define TOUCH_PIN        27   // GP27: touch sensor input

// Touch sensor polarity (common modules: HIGH when touched)
static const bool TOUCH_ACTIVE_HIGH = true;

// TOUCH payload: variable-length segments
// Each segment: state (0/1) + durationTicks (uint16) where 1 tick = TOUCH_TICK_MS
struct __attribute__((packed)) TouchSeg {
  uint8_t  state;         // 0/1
  uint16_t durationTicks; // duration / TOUCH_TICK_MS
};

// =====================================================
// LLCC68 wiring (SPI0) on Pico
// NSS=GP17, DIO1=GP20, RST=GP22, BUSY=GP21
// =====================================================
LLCC68 radio = new Module(
  17, // NSS
  20, // DIO1
  22, // RST
  21  // BUSY
);

// =====================================================
// RADIO TIMING (idle behavior)
// (reduced ping rate to reduce collisions + battery)
// =====================================================
static const uint32_t TX_INTERVAL_MS = 1500;  // was 500
static const uint32_t RX_WINDOW_MS   = 700;   // receive window while idle

// =====================================================
// TOUCH RECORDING
// =====================================================
static const uint32_t TOUCH_END_GAP_MS = 2000;  // stop recording after this long "not touched"
static const uint32_t TOUCH_MAX_RECORD_MS = 12000; // safety: never record longer than this
static const uint16_t TOUCH_TICK_MS    = 10;    // durations encoded in 10ms units
static const uint8_t  MAX_SEGMENTS     = 80;    // max on/off segments per message

// =====================================================
// TOUCH SEND/ACK
// =====================================================
static const uint32_t ACK_WAIT_MS      = 700;   // wait for ack after each transmit
static const uint32_t RETRY_GAP_MS     = 140;   // small gap between retries
static const uint32_t ACK_TOTAL_TIMEOUT_MS = 8000; // B) hard stop: don't get stuck forever
static const uint8_t  TX_POWER_DBM     = 10;    // conservative

// =====================================================
// SIGNAL LED (GP0) FADE ANIMATION
// =====================================================
static const uint32_t FADE_TIME_MS = 1500;
static const uint8_t LED_MIN = 5;
static const uint8_t LED_MAX = 180;

// =====================================================
// BUILT-IN LED (GP25) STATIC PWM PER PEER
// =====================================================
static const uint8_t BUILTIN_PEER1 = 180; // brighter
static const uint8_t BUILTIN_PEER2 = 35;  // dimmer
static const uint8_t BUILTIN_OTHER = 80;  // fallback

// =====================================================
// ONLINE LED (GP28) STATIC PWM
// =====================================================
static const uint8_t ONLINE_PWM = 40;     // always on when idle
static const uint32_t PLAYBACK_START_OFF_MS = 50; // turn off briefly before playback

// =====================================================
// PACKET TYPES
// =====================================================
enum PacketType : uint8_t {
  PKT_PING      = 1,
  PKT_TOUCH     = 2,
  PKT_TOUCH_ACK = 3
};

// =====================================================
// PACKET FORMATS
// =====================================================
// Common header
struct __attribute__((packed)) PktHdr {
  uint8_t  type;  // PacketType
  uint8_t  src;
  uint8_t  dst;
  uint16_t seq;
};

// C) Fixed-length TOUCH packet (deterministic RX)
struct __attribute__((packed)) TouchPacketFixed {
  PktHdr   h;
  uint8_t  segCount;               // how many are valid in segs[]
  TouchSeg segs[MAX_SEGMENTS];     // always transmitted
};

// =====================================================
// TUNING TABLES (RSSI/SNR -> brightness)
// =====================================================
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

// =====================================================
// STATE
// =====================================================
static uint32_t lastTxMs = 0;
static uint32_t lastRxWindowMs = 0;

static uint32_t fadeStartMs = 0;
static uint8_t  fadePeak   = 0;
static bool     fading     = false;

static uint16_t txSeqPing  = 0;
static uint16_t txSeqTouch = 0;

static int32_t lastRxPingSeq  = -1;
static int32_t lastRxTouchSeq = -1;

// =====================================================
// HELPERS
// =====================================================
static inline bool everyMs(uint32_t &last, uint32_t period) {
  uint32_t now = millis();
  if (now - last >= period) { last = now; return true; }
  return false;
}

static inline uint8_t chooseBuiltinLevel(uint8_t peer) {
  if (peer == 1) return BUILTIN_PEER1;
  if (peer == 2) return BUILTIN_PEER2;
  return BUILTIN_OTHER;
}

static inline bool touchRead() {
  int v = digitalRead(TOUCH_PIN);
  return TOUCH_ACTIVE_HIGH ? (v == HIGH) : (v == LOW);
}

// =====================================================
// SIGNAL LED CONTROL (GP0) - smooth fade
// =====================================================
static inline uint8_t computeFade() {
  uint32_t elapsed = millis() - fadeStartMs;
  if (elapsed >= FADE_TIME_MS) {
    fading = false;
    return 0;
  }
  float t = 1.0f - (float)elapsed / (float)FADE_TIME_MS; // 1 -> 0
  float gamma = t * t; // exponential-ish
  return (uint8_t)(fadePeak * gamma);
}

static void triggerSignalLed(float quality) {
  if (fading) return; // keep your behavior: ignore retriggers during fade
  uint8_t peak = LED_MIN + (uint8_t)(quality * (LED_MAX - LED_MIN));
  fadePeak    = peak;
  fadeStartMs = millis();
  fading      = true;
}

static void updateSignalLed() {
  if (!fading) {
    analogWrite(SIGNAL_LED_PIN, 0);
    return;
  }
  analogWrite(SIGNAL_LED_PIN, computeFade());
}

// =====================================================
// RADIO SEND PRIMITIVE
// =====================================================
static void txBytes(const uint8_t *buf, size_t len) {
  radio.transmit((uint8_t*)buf, len);
}

// =====================================================
// ONLINE LED playback (GP28)
// =====================================================
static void setOnlineIdle() {
  analogWrite(ONLINE_LED_PIN, ONLINE_PWM);
}

// Play pattern on ONLINE LED, keeping timing.
// ONLINE LED is normally on; playback turns it off briefly, then replays.
static void playbackOnOnlineLed(const TouchSeg *segs, uint8_t segCount) {
  // start: force off for a short "click"
  analogWrite(ONLINE_LED_PIN, 0);
  delay(PLAYBACK_START_OFF_MS);

  for (uint8_t i = 0; i < segCount; i++) {
    uint8_t state = segs[i].state ? 1 : 0;
    uint16_t ticks = segs[i].durationTicks;
    uint32_t durMs = (uint32_t)ticks * (uint32_t)TOUCH_TICK_MS;

    analogWrite(ONLINE_LED_PIN, state ? ONLINE_PWM : 0);

    uint32_t t0 = millis();
    while (millis() - t0 < durMs) {
      delay(1); // blocking by request
    }
  }

  // Return to steady on
  setOnlineIdle();
}

// =====================================================
// PING (idle background)
// =====================================================
static void sendPing() {
  PktHdr h;
  h.type = PKT_PING;
  h.src  = MY_ID;
  h.dst  = PEER_ID;
  h.seq  = txSeqPing++;
  txBytes((uint8_t*)&h, sizeof(h));
}

// =====================================================
// TOUCH RECORDING (blocking)
// =====================================================
static uint8_t recordTouchPattern(TouchSeg *outSegs, uint8_t maxSegs) {
  uint8_t segCount = 0;

  bool cur = touchRead();
  uint32_t segStart = millis();
  uint32_t lastTouchedMs = cur ? millis() : 0;
  uint32_t recordStartMs = millis();

  auto pushSeg = [&](bool state, uint32_t durMs) {
    if (segCount >= maxSegs) return;
    uint32_t ticks = (durMs + (TOUCH_TICK_MS / 2)) / TOUCH_TICK_MS;
    if (ticks == 0) ticks = 1;
    if (ticks > 65535) ticks = 65535;
    outSegs[segCount].state = state ? 1 : 0;
    outSegs[segCount].durationTicks = (uint16_t)ticks;
    segCount++;
  };

  while (true) {
    bool now = touchRead();
    uint32_t ms = millis();

    // Safety: never record forever
    if (ms - recordStartMs >= TOUCH_MAX_RECORD_MS) {
      uint32_t dur = ms - segStart;
      pushSeg(cur, dur);
      break;
    }

    if (now) lastTouchedMs = ms;

    if (now != cur) {
      uint32_t dur = ms - segStart;
      pushSeg(cur, dur);
      cur = now;
      segStart = ms;
    }

    if (!now && lastTouchedMs != 0 && (ms - lastTouchedMs >= TOUCH_END_GAP_MS)) {
      uint32_t dur = ms - segStart;
      pushSeg(cur, dur);
      break;
    }

    if (!now && lastTouchedMs == 0 && (ms - segStart >= TOUCH_END_GAP_MS)) {
      segCount = 0;
      break;
    }

    delay(1);

    if (segCount >= maxSegs) {
      uint32_t dur = ms - segStart;
      pushSeg(cur, dur);
      break;
    }
  }

  // drop leading OFF segment (keeps playback cleaner)
  if (segCount > 0 && outSegs[0].state == 0) {
    for (uint8_t i = 1; i < segCount; i++) outSegs[i - 1] = outSegs[i];
    segCount--;
  }

  return segCount;
}

// =====================================================
// TOUCH TX UNTIL ACK (B + C)
// - Fixed-size TOUCH packet
// - Hard total timeout so it never bricks forever
// =====================================================
static void sendTouchUntilAck(const TouchSeg *segs, uint8_t segCount) {
  if (segCount == 0) return;

  TouchPacketFixed pkt;
  memset(&pkt, 0, sizeof(pkt));

  pkt.h.type = PKT_TOUCH;
  pkt.h.src  = MY_ID;
  pkt.h.dst  = PEER_ID;
  pkt.h.seq  = txSeqTouch++;
  pkt.segCount = segCount;

  // copy only segCount, rest stays zero
  memcpy(pkt.segs, segs, segCount * sizeof(TouchSeg));

  uint32_t overallStart = millis();

  while (true) {
    txBytes((uint8_t*)&pkt, sizeof(pkt));

    // wait for ACK
    uint32_t t0 = millis();
    while (millis() - t0 < ACK_WAIT_MS) {
      PktHdr rh;
      int16_t st = radio.receive((uint8_t*)&rh, sizeof(rh), 140);
      if (st != RADIOLIB_ERR_NONE) continue;

      if (rh.type != PKT_TOUCH_ACK) continue;
      if (rh.dst  != MY_ID)        continue;
      if (rh.src  != PEER_ID)      continue;
      if (rh.seq  != pkt.h.seq)    continue;

      // ACK received ✅
      return;
    }

    // B) total timeout: give up and return to idle instead of bricking
    if (millis() - overallStart >= ACK_TOTAL_TIMEOUT_MS) {
      return;
    }

    delay(RETRY_GAP_MS);
  }
}

// =====================================================
// RX HANDLER (idle mode only)
// - Handles:
//   - PING: triggers SIGNAL LED fade
//   - TOUCH: ACK + playback on ONLINE LED (blocking playback)
// =====================================================
static void handleIncomingIdle(uint32_t timeoutMs) {
  // receive fixed-size buffer for TOUCH packets (largest case)
  TouchPacketFixed rxp;
  int16_t st = radio.receive((uint8_t*)&rxp, sizeof(rxp), timeoutMs);
  if (st != RADIOLIB_ERR_NONE) return;

  // minimum sanity: header pair
  PktHdr &h = rxp.h;

  if (h.dst != MY_ID || h.src != PEER_ID) return;

  if (h.type == PKT_PING) {
    if ((int32_t)h.seq == lastRxPingSeq) return;
    lastRxPingSeq = h.seq;

    float rssi = radio.getRSSI();
    float snr  = radio.getSNR();
    float q = combineQuality(mapRSSI(rssi), mapSNR(snr));
    triggerSignalLed(q);
    return;
  }

  if (h.type == PKT_TOUCH) {
    // ACK immediately (even for duplicates)
    PktHdr ack;
    ack.type = PKT_TOUCH_ACK;
    ack.src  = MY_ID;
    ack.dst  = PEER_ID;
    ack.seq  = h.seq;
    txBytes((uint8_t*)&ack, sizeof(ack));

    // duplicates: ACK but don't replay
    if ((int32_t)h.seq == lastRxTouchSeq) return;
    lastRxTouchSeq = h.seq;

    uint8_t segCount = rxp.segCount;
    if (segCount > MAX_SEGMENTS) return;

    playbackOnOnlineLed(rxp.segs, segCount);
    return;
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);

  pinMode(SIGNAL_LED_PIN, OUTPUT);
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  pinMode(ONLINE_LED_PIN, OUTPUT);

  // A) make touch input deterministic (prevents floating/false touch)
  // Most TTP223 modules are active-high; pulldown makes "not touched" stable LOW.
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  // Static indicators
  analogWrite(BUILTIN_LED_PIN, chooseBuiltinLevel(PEER_ID));
  setOnlineIdle();
  analogWrite(SIGNAL_LED_PIN, 0);

  // SPI pins for Pico (SPI0)
  SPI.setRX(16);
  SPI.setTX(19);
  SPI.setSCK(18);
  SPI.setCS(17);
  SPI.begin();

  radio.setDio2AsRfSwitch(true);

  if (radio.begin(433.0) != RADIOLIB_ERR_NONE) {
    while (true) { delay(1000); }
  }

  // Radio params (match both sides)
  radio.setOutputPower(TX_POWER_DBM);
  radio.setBandwidth(125.0);
  radio.setSpreadingFactor(12);
  radio.setCodingRate(8);
  radio.setPreambleLength(12);
  radio.setCRC(true);
  radio.setSyncWord(0x12);
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  // 1) TOUCH RECORDING TRIGGER (blocking record + blocking send until ack)
  if (touchRead()) {
    TouchSeg segs[MAX_SEGMENTS];
    uint8_t segCount = recordTouchPattern(segs, MAX_SEGMENTS);

    sendTouchUntilAck(segs, segCount);

    // Restore steady online LED
    setOnlineIdle();

    // small settle
    delay(30);
    return;
  }

  // 2) IDLE: periodic ping + periodic RX + signal fade
  if (everyMs(lastTxMs, TX_INTERVAL_MS)) {
    sendPing();
  }

  if (everyMs(lastRxWindowMs, RX_WINDOW_MS + 80)) {
    handleIncomingIdle(RX_WINDOW_MS);
  }

  updateSignalLed();
}
