#include <Arduino.h>

// ===================== PINS (match diagram generator) =====================
const int PIN_DATA  = 8;   // SR_DATA  (UNO -> sr0 DS)
const int PIN_CLK   = 9;   // SR_CLK   (SHCP / CLK)
const int PIN_LATCH = 10;  // SR_LATCH (STCP / LE)
const int PIN_OE_N  = 11;  // MBI OE# (active-low)

// ===================== PANEL CONFIG =====================
static const uint8_t W = 16;
static const uint8_t H = 16;

// Two layers: fb0 = layer0 (mbi0), fb1 = layer1 (mbi1)
// Each row is 16 bits: bit0=x0 ... bit15=x15
volatile uint16_t fb0[H];
volatile uint16_t fb1[H];

// Scan timing
static const uint16_t ROW_ON_US = 800;

// ===================== LOW LEVEL IO =====================
static inline void pulseClock() {
  digitalWrite(PIN_CLK, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_CLK, LOW);
  delayMicroseconds(1);
}

static inline void pulseLatch() {
  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_LATCH, LOW);
  delayMicroseconds(1);
}

// Shift N bits, MSB-first
static inline void shiftBitsMSB(uint32_t value, uint8_t bits) {
  for (int i = bits - 1; i >= 0; --i) {
    digitalWrite(PIN_DATA, (value >> i) & 1);
    pulseClock();
  }
}

// ===================== GRAPHICS API =====================
static inline void clearLayer(volatile uint16_t *fb) {
  for (uint8_t y = 0; y < H; y++) fb[y] = 0;
}

static inline void setPixel(volatile uint16_t *fb, uint8_t x, uint8_t y, bool on) {
  if (x >= W || y >= H) return;
  uint16_t mask = (uint16_t)1 << x;
  if (on) fb[y] |= mask;
  else    fb[y] &= ~mask;
}

static inline void drawRect(volatile uint16_t *fb,
                            uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                            bool on) {
  if (x1 < x0) { uint8_t t = x0; x0 = x1; x1 = t; }
  if (y1 < y0) { uint8_t t = y0; y0 = y1; y1 = t; }
  if (x0 >= W || y0 >= H) return;
  if (x1 >= W) x1 = W - 1;
  if (y1 >= H) y1 = H - 1;

  for (uint8_t x = x0; x <= x1; x++) {
    setPixel(fb, x, y0, on);
    setPixel(fb, x, y1, on);
  }
  for (uint8_t y = y0; y <= y1; y++) {
    setPixel(fb, x0, y, on);
    setPixel(fb, x1, y, on);
  }
}

// Build a centered square frame based on "step" (0..7)
static void radiatingSquare(volatile uint16_t *fb, uint8_t step) {
  const int cx0 = 7, cx1 = 8;
  const int cy0 = 7, cy1 = 8;

  int x0 = cx0 - (int)step;
  int y0 = cy0 - (int)step;
  int x1 = cx1 + (int)step;
  int y1 = cy1 + (int)step;

  clearLayer(fb);

  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > 15) x1 = 15;
  if (y1 > 15) y1 = 15;

  drawRect(fb, (uint8_t)x0, (uint8_t)y0, (uint8_t)x1, (uint8_t)y1, true);
}

// ===================== SCAN / OUTPUT =====================
// Row select word for 2x 74HC595:
// - sr0 drives ROWEN_0..7 (Q0..Q7)
// - sr1 drives ROWEN_8..15 (Q0..Q7)
static inline uint16_t makeRowWord(uint8_t y) {
  return (uint16_t)1 << y;
}

// Shift order with chain: UNO -> sr0 -> sr1 -> mbi0 -> mbi1
// The FIRST shifted bits travel farthest, so they land in the far end (mbi1).
//
// So we must shift in this order:
//   1) mbi1 16 bits (layer1 columns)
//   2) mbi0 16 bits (layer0 columns)
//   3) sr1  8 bits
//   4) sr0  8 bits
static void showRow(uint8_t y) {
  digitalWrite(PIN_OE_N, HIGH); // blank while updating

  uint16_t cols0 = fb0[y];
  uint16_t cols1 = fb1[y];
  uint16_t rowWord = makeRowWord(y);

  uint8_t sr0_byte = (uint8_t)(rowWord & 0xFF);
  uint8_t sr1_byte = (uint8_t)((rowWord >> 8) & 0xFF);

  // 1) Layer1 (mbi1)
  shiftBitsMSB(cols1, 16);

  // 2) Layer0 (mbi0)
  shiftBitsMSB(cols0, 16);

  // 3) sr1
  shiftBitsMSB(sr1_byte, 8);

  // 4) sr0
  shiftBitsMSB(sr0_byte, 8);

  pulseLatch();

  digitalWrite(PIN_OE_N, LOW);  // enable
  delayMicroseconds(ROW_ON_US);
}

static void scanOnce() {
  for (uint8_t y = 0; y < H; y++) showRow(y);
}

// ===================== ANIMATION =====================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_OE_N, OUTPUT);

  digitalWrite(PIN_DATA, LOW);
  digitalWrite(PIN_CLK, LOW);
  digitalWrite(PIN_LATCH, LOW);

  digitalWrite(PIN_OE_N, HIGH); // start blanked

  clearLayer(fb0);
  clearLayer(fb1);
}

void loop() {
  // Keep scanning always
  scanOnce();

  // Animation update
  static uint32_t lastAnim = 0;
  static uint8_t step = 0;

  uint32_t now = millis();
  if (now - lastAnim > 120) {
    lastAnim = now;

    // Layer0 expands
    radiatingSquare(fb0, step);

    // Layer1 contracts (reverse step)
    uint8_t inv = 7 - step;
    radiatingSquare(fb1, inv);

    step++;
    if (step > 7) step = 0;
  }
}
