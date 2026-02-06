#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// I2C address is usually 0x3C
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  // Start I2C (ESP8266 default pins)
  Wire.begin(D2, D1); // SDA, SCL

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (true); // stop if OLED not found
  }

  display.clearDisplay();

  // Text settings
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Print text
  display.println("Hello ESP8266!");
  display.println();
  display.println("OLED works :)");

  display.display();
}

void loop() {
  // nothing here
}
