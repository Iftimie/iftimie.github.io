#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Comet parameters
int x = 10;
int y = 10;
int vx = 6;   // horizontal speed
int vy = 3;   // vertical speed
const int TAIL_LEN = 18;

void drawComet(int hx, int hy, int vx, int vy) {
  // Tail goes opposite movement direction
  int tx = -vx;
  int ty = -vy;

  // Draw tail segments
  for (int i = 1; i <= TAIL_LEN; i++) {
    int px = hx + tx * i;
    int py = hy + ty * i;

    // wrap-friendly: skip out-of-bounds tail pixels
    if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT) continue;

    // "Fade" by making tail sparser and thinner
    // Near head: dense; far: sparse
    if (i > 12 && (i % 2 == 0)) continue;  // sparser far away
    if (i > 16 && (i % 3 != 0)) continue;  // even sparser at the end

    // Thickness: 2px near head, 1px farther
    if (i <= 6) {
      display.drawPixel(px, py, SSD1306_WHITE);
      if (py + 1 < SCREEN_HEIGHT) display.drawPixel(px, py + 1, SSD1306_WHITE);
    } else {
      display.drawPixel(px, py, SSD1306_WHITE);
    }
  }

  // Draw head (bright)
  display.fillCircle(hx, hy, 2, SSD1306_WHITE);
}

void setup() {
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  display.clearDisplay();
  display.display();
}

void loop() {
  display.clearDisplay();

  drawComet(x, y, vx, vy);

  display.display();
  delay(10);

  // Move
  x += vx;
  y += vy;

  // Wrap around screen edges
  if (x < -4) x = SCREEN_WIDTH + 4;
  if (x > SCREEN_WIDTH + 4) x = -4;
  if (y < -4) y = SCREEN_HEIGHT + 4;
  if (y > SCREEN_HEIGHT + 4) y = -4;

  // Optional: tiny drift changes to make it feel alive
  // (comment out if you want perfectly straight motion)
  static int tick = 0;
  tick++;
  if (tick % 90 == 0) {
    vy = (vy == 1) ? 0 : 1; // subtle wobble between 0 and 1
  }
}
