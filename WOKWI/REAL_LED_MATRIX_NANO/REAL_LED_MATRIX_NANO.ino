#include <Arduino.h>
#include <SPI.h>

// ===================== PINS =====================
// Nano/Uno SPI pins fixed: MOSI=D11, SCK=D13
const int PIN_LATCH = 10;  // STCP / LE
const int PIN_OE_N  = 9;   // OE# (active-low)

// ===================== PANEL CONFIG =====================
static const uint8_t W = 16;
static const uint8_t H = 16;
static const uint8_t LAYERS = 5;

// Framebuffers: fb[layer][y] is a 16-bit row bitmask
volatile uint16_t fb[LAYERS][H];
static volatile uint8_t scanY = 0;

// ===================== TIMER SCANNER (ISR) =====================
// rowWord: bit y = 1
static inline uint16_t makeRowWord(uint8_t y) { return (uint16_t)1 << y; }

static inline void latchPulseFast() {
  digitalWrite(PIN_LATCH, HIGH);
  digitalWrite(PIN_LATCH, LOW);
}

ISR(TIMER1_COMPA_vect) {
  digitalWrite(PIN_OE_N, HIGH); // blank while shifting

  uint8_t y = scanY;
  uint16_t rowWord = makeRowWord(y);
  uint8_t sr0_byte = (uint8_t)(rowWord & 0xFF);
  uint8_t sr1_byte = (uint8_t)((rowWord >> 8) & 0xFF);

  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  // Daisy chain assumed:
  // MCU -> sr0 -> sr1 -> mbi0 -> mbi1 -> mbi2 -> mbi3 -> mbi4
  // Shift order must be: mbi4..mbi0, then sr1, sr0
  for (int l = (int)LAYERS - 1; l >= 0; --l) {
    uint16_t cols = fb[l][y];
    SPI.transfer((uint8_t)(cols >> 8));
    SPI.transfer((uint8_t)(cols & 0xFF));
  }

  SPI.transfer(sr1_byte);
  SPI.transfer(sr0_byte);

  SPI.endTransaction();

  latchPulseFast();
  digitalWrite(PIN_OE_N, LOW);  // enable LEDs

  scanY++;
  if (scanY >= H) scanY = 0;
}

static void startScanner(uint16_t rowRateHz) {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // prescaler 8 => 2MHz timer clock
  TCCR1B |= (1 << WGM12);  // CTC
  TCCR1B |= (1 << CS11);   // prescaler 8
  OCR1A = (uint16_t)((2000000UL / rowRateHz) - 1);

  TIMSK1 |= (1 << OCIE1A);
  sei();
}

// ===================== FB HELPERS =====================
static inline void clearAll() {
  cli();
  for (uint8_t l = 0; l < LAYERS; l++) {
    for (uint8_t y = 0; y < H; y++) fb[l][y] = 0;
  }
  sei();
}

static inline void commitVolume(uint16_t local[LAYERS][H]) {
  cli();
  for (uint8_t z = 0; z < LAYERS; z++) {
    for (uint8_t y = 0; y < H; y++) {
      fb[z][y] = local[z][y];
    }
  }
  sei();
}

static inline void setVoxelLocal(uint16_t local[LAYERS][H], int x, int y, int z) {
  if (x < 0 || x >= (int)W || y < 0 || y >= (int)H || z < 0 || z >= (int)LAYERS) return;
  local[z][y] |= (uint16_t)1 << x;
}

// ===================== TIMING =====================
static inline bool everyMs(uint32_t &last, uint32_t periodMs) {
  uint32_t now = millis();
  if (now - last >= periodMs) { last = now; return true; }
  return false;
}

// ===================== COMET (HEAD=1 PIXEL, TAIL=7) =====================

// Tail settings
static const uint8_t TAIL_LEN = 7;
static const uint8_t HISTORY  = TAIL_LEN + 1; // head + tail

struct Voxel { int8_t x, y, z; };
static Voxel trail[HISTORY];

// Direction: each component is -1, 0, or +1
static int8_t dx = 1, dy = 0, dz = 0;

// You can tweak these:
static const uint16_t STEP_MS   = 55;  // speed (lower = faster)
static const uint8_t  TURN_PROB = 30;  // % chance per step to pick a new direction (0..100)
static const uint8_t  Z_TURN_WEIGHT = 35; // % chance dz is non-zero when choosing new dir (0..100)

// Pick a new direction, but never (0,0,0)
static void chooseNewDirection() {
  while (true) {
    int8_t ndx = (int8_t)random(-1, 2); // -1,0,1
    int8_t ndy = (int8_t)random(-1, 2);

    int8_t ndz = 0;
    // make z changes less frequent if you want
    if ((uint8_t)random(100) < Z_TURN_WEIGHT) {
      ndz = (int8_t)random(-1, 2);
    }

    if (ndx == 0 && ndy == 0 && ndz == 0) continue;

    dx = ndx; dy = ndy; dz = ndz;
    return;
  }
}

// Advance head by (dx,dy,dz) with bounce on edges
static void stepComet() {
  // maybe change direction
  if ((uint8_t)random(100) < TURN_PROB) {
    chooseNewDirection();
  }

  int nx = trail[0].x + dx;
  int ny = trail[0].y + dy;
  int nz = trail[0].z + dz;

  // Bounce: if next step goes out, invert that axis direction and recompute
  if (nx < 0 || nx >= (int)W) { dx = -dx; nx = trail[0].x + dx; }
  if (ny < 0 || ny >= (int)H) { dy = -dy; ny = trail[0].y + dy; }
  if (nz < 0 || nz >= (int)LAYERS) { dz = -dz; nz = trail[0].z + dz; }

  // Still could be invalid if dx/dy/dz was 0 and position already at edge? (rare)
  if (nx < 0) nx = 0;
  if (nx >= (int)W) nx = W - 1;
  if (ny < 0) ny = 0;
  if (ny >= (int)H) ny = H - 1;
  if (nz < 0) nz = 0;
  if (nz >= (int)LAYERS) nz = LAYERS - 1;

  // shift trail
  for (int i = HISTORY - 1; i >= 1; --i) trail[i] = trail[i - 1];
  trail[0] = { (int8_t)nx, (int8_t)ny, (int8_t)nz };
}

// Render head + tail (all are single pixels)
static void renderComet() {
  uint16_t local[LAYERS][H];
  for (uint8_t z = 0; z < LAYERS; z++) for (uint8_t y = 0; y < H; y++) local[z][y] = 0;

  // head + tail
  for (uint8_t i = 0; i < HISTORY; i++) {
    setVoxelLocal(local, trail[i].x, trail[i].y, trail[i].z);
  }

  commitVolume(local);
}

static void animComet() {
  static uint32_t tStep = 0;
  if (!everyMs(tStep, STEP_MS)) return;

  stepComet();
  renderComet();
}

// ===================== ARDUINO =====================
void setup() {
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_OE_N, OUTPUT);
  digitalWrite(PIN_LATCH, LOW);
  digitalWrite(PIN_OE_N, HIGH);

  SPI.begin();
  clearAll();

  // 2000 rows/sec => 125 Hz refresh
  startScanner(2000);

  // seed RNG (good enough for motion)
  randomSeed(analogRead(A0));

  // init comet in center with a straight tail
  int cx = 7, cy = 7, cz = 2;
  for (uint8_t i = 0; i < HISTORY; i++) {
    trail[i] = { (int8_t)cx, (int8_t)cy, (int8_t)cz };
  }

  // initial direction
  chooseNewDirection();

  Serial.begin(115200);
}

void loop() {
  animComet();
}
