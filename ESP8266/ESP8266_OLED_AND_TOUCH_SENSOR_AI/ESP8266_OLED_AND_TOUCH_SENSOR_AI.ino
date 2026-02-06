#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===== TOUCH + LED =====
#define TOUCH_PIN D5   // GPIO14
#define LED_PIN   D6   // GPIO12
int lastTouchState = LOW;

// Touch feel tuning
unsigned long nextAllowedTouchMs = 0;
const unsigned long TOUCH_COOLDOWN_MS = 200;

// ===== AI FACTS =====
const char* facts[] = {
  "Honey never spoils.\nJars found in\nEgypt are edible.",
  "Octopuses have\nthree hearts\nand blue blood.",
  "Bananas are\nradioactive.\nPotassium-40!",
  "Your brain uses\n~20% of your\nbody's energy.",
  "Plants can\ncommunicate\nusing chemicals.",
  "There are more\npossible games of\nchess than atoms.",
  "Wombat poop\nis cube-shaped.\nYes. Really.",
  "AI models don't\n\"think\".\nThey predict."
};
const int FACT_COUNT = sizeof(facts) / sizeof(facts[0]);
int currentFactIndex = 0;

// ===== SCENES =====
enum Scene : uint8_t {
  SCENE_LOADING = 0,
  SCENE_EYES    = 1,
  SCENE_SCOPE   = 2,
  SCENE_FACT    = 3,
};
Scene currentScene = SCENE_LOADING;

// ===== TIMING =====
unsigned long lastFrameMs = 0;
const uint16_t FRAME_MS = 33; // ~30 FPS

// Boot sequence control
bool bootSequenceDone = false;
unsigned long sceneStartMs = 0;
const unsigned long EYES_DURATION_MS  = 2500;
const unsigned long SCOPE_DURATION_MS = 2500;

// ===== SCENE STATE =====
uint8_t loadProgress = 0;

int eyeLookX = 0;          // -1,0,1
int eyeLookY = 0;          // -1,0,1
uint8_t blinkPhase = 0;    // 0=open, 1=closing, 2=closed, 3=opening
unsigned long blinkNextMs = 0;
unsigned long lookNextMs = 0;

uint16_t scopePhase = 0;
uint8_t scanline = 0;

// ===== UTILS =====
static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void drawAIScreen(int factIndex) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);

  display.println("DeskMate AI");
  display.println("------------");
  display.println();
  display.println(facts[factIndex]);

  display.setCursor(0, 56);
  display.print("Touch: next (");
  display.print(factIndex + 1);
  display.print("/");
  display.print(FACT_COUNT);
  display.print(")");

  display.display();
}

// ===== SCENE: LOADING =====
void resetLoading() { loadProgress = 0; }

void tickLoading() {
  if (loadProgress < 100) loadProgress++;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("BOOT: DESKMATE.AI");
  display.println("----------------");

  display.setCursor(0, 22);
  display.print("LINK ");
  display.print((loadProgress / 7) % 10);
  display.print(".");
  display.print((loadProgress / 3) % 10);
  display.print(".");
  display.println(loadProgress % 10);

  display.setCursor(0, 32);
  display.print("SYNC ");
  display.print((loadProgress * 13) % 97);
  display.println("%");

  int barX = 6, barY = 50, barW = 116, barH = 10;
  display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
  int fillW = (barW - 2) * loadProgress / 100;
  display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SSD1306_WHITE);

  for (int i = 0; i < 14; i++) {
    int x = (loadProgress * 7 + i * 13) % SCREEN_WIDTH;
    int y = (loadProgress * 3 + i * 11) % SCREEN_HEIGHT;
    display.drawPixel(x, y, SSD1306_WHITE);
  }

  display.display();
}

// ===== SCENE: EYES =====
void resetEyes() {
  blinkPhase = 0;
  blinkNextMs = millis() + 900;
  lookNextMs  = millis() + 400;
  eyeLookX = 0;
  eyeLookY = 0;
}

void drawEye(int cx, int cy, int w, int h, int lookX, int lookY, uint8_t blink) {
  display.drawRoundRect(cx - w/2, cy - h/2, w, h, 6, SSD1306_WHITE);

  int openH = h - 6;
  if (blink == 1) openH = openH * 2 / 3;
  if (blink == 2) openH = 2;
  if (blink == 3) openH = openH * 2 / 3;

  if (blink == 2) {
    display.drawLine(cx - w/2 + 3, cy, cx + w/2 - 3, cy, SSD1306_WHITE);
    return;
  }

  int px = cx + lookX * 4;
  int py = cy + lookY * 3;

  px = clampi(px, cx - w/2 + 8, cx + w/2 - 8);
  py = clampi(py, cy - openH/2 + 4, cy + openH/2 - 4);

  display.fillCircle(px, py, 3, SSD1306_WHITE);
  display.drawPixel(px - 1, py - 1, SSD1306_BLACK);
}

void tickEyes() {
  unsigned long now = millis();

  if (now >= lookNextMs) {
    eyeLookX = (int)((now / 233) % 3) - 1;
    eyeLookY = (int)((now / 317) % 3) - 1;
    lookNextMs = now + 250 + (now % 400);
  }

  if (now >= blinkNextMs) {
    if (blinkPhase == 0) { blinkPhase = 1; blinkNextMs = now + 70; }
    else if (blinkPhase == 1) { blinkPhase = 2; blinkNextMs = now + 80; }
    else if (blinkPhase == 2) { blinkPhase = 3; blinkNextMs = now + 70; }
    else { blinkPhase = 0; blinkNextMs = now + 900 + (now % 1200); }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("VISUAL CORE ONLINE");

  drawEye(44, 36, 42, 26, eyeLookX, eyeLookY, blinkPhase);
  drawEye(84, 36, 42, 26, eyeLookX, eyeLookY, blinkPhase);

  display.drawLine(30, 58, 98, 58, SSD1306_WHITE);
  for (int i = 0; i < 6; i++) {
    int x = 30 + (i * 12) + ((now / 120) % 6);
    display.drawPixel(x, 57, SSD1306_WHITE);
  }

  display.display();
}

// ===== SCENE: OSCILLOSCOPE =====
void resetScope() {
  scopePhase = 0;
  scanline = 0;
}

void tickScope() {
  scopePhase += 3;
  scanline = (scanline + 1) % 8;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("SIGNAL TRACE");

  for (int x = 0; x < SCREEN_WIDTH; x += 16) display.drawFastVLine(x, 12, 52, SSD1306_WHITE);
  for (int y = 12; y < SCREEN_HEIGHT; y += 13) display.drawFastHLine(0, y, SCREEN_WIDTH, SSD1306_WHITE);

  int midY = 38;
  int prevX = 0;
  int prevY = midY;

  for (int x = 0; x < SCREEN_WIDTH; x++) {
    int a = (int)((x * 5 + scopePhase) % 64) - 32;
    int b = (int)((x * 9 + scopePhase * 2) % 48) - 24;
    int y = midY + (a / 8) + (b / 12);
    y = clampi(y, 14, 62);

    if (x > 0) display.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
    prevX = x;
    prevY = y;
  }

  int yScan = 12 + scanline;
  display.drawFastHLine(0, yScan, SCREEN_WIDTH, SSD1306_WHITE);

  display.setCursor(0, 54);
  display.print("AMP:");
  display.print((scopePhase * 7) % 100);
  display.print("  F:");
  display.print((scopePhase * 3) % 99);

  display.display();
}

// ===== SCENE CONTROL =====
void setScene(Scene s) {
  currentScene = s;
  sceneStartMs = millis();

  Serial.print("Scene -> ");
  Serial.println((int)currentScene);

  if (s == SCENE_LOADING) resetLoading();
  if (s == SCENE_EYES)    resetEyes();
  if (s == SCENE_SCOPE)   resetScope();
  if (s == SCENE_FACT)    drawAIScreen(currentFactIndex);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("=== DeskMate AI (boot then facts) ===");

  Wire.begin(D2, D1); // SDA, SCL

  pinMode(TOUCH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED not found.");
    while (true) delay(100);
  }

  setScene(SCENE_LOADING);
  Serial.println("Boot sequence running. Then facts forever.");
}

void loop() {
  unsigned long now = millis();

  // --- Touch read (works always) ---
  int touched = digitalRead(TOUCH_PIN);
  digitalWrite(LED_PIN, touched ? HIGH : LOW);

  // After boot sequence: ONLY facts + touch cycles facts
  if (bootSequenceDone) {
    if (touched == HIGH && lastTouchState == LOW && now >= nextAllowedTouchMs) {
      currentFactIndex = (currentFactIndex + 1) % FACT_COUNT;
      Serial.print("Next fact -> ");
      Serial.println(currentFactIndex);
      drawAIScreen(currentFactIndex);
      nextAllowedTouchMs = now + TOUCH_COOLDOWN_MS;
    }
    lastTouchState = touched;
    delay(5);
    return;
  }

  // --- Boot sequence animation (plays once) ---
  if (now - lastFrameMs >= FRAME_MS) {
    lastFrameMs = now;

    if (currentScene == SCENE_LOADING) {
      tickLoading();
      if (loadProgress >= 100) setScene(SCENE_EYES);
    }
    else if (currentScene == SCENE_EYES) {
      tickEyes();
      if (now - sceneStartMs >= EYES_DURATION_MS) setScene(SCENE_SCOPE);
    }
    else if (currentScene == SCENE_SCOPE) {
      tickScope();
      if (now - sceneStartMs >= SCOPE_DURATION_MS) {
        bootSequenceDone = true;
        setScene(SCENE_FACT);
      }
    }
  }

  lastTouchState = touched;
  delay(5);
}
