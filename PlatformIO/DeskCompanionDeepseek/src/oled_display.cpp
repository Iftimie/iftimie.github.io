#include "oled_display.h"
#include "config.h"
#include "bluetooth_audio.h"

TwoWire customWire = TwoWire(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &customWire, -1);

static volatile bool oled_enabled = false;

void initDisplay() {
    enableOLED();
}

void enableOLED() {
    if (oled_enabled) return;
    Serial.println("Enabling OLED (probing I2C pins)");
    // Try configured pins first, then a small list of fallback SDA/SCL pairs
    const int attempts[][2] = {
        { OLED_SDA_PIN, OLED_SCL_PIN },
        { 5, 16 },   // common software I2C choice
        { 13, 16 },  // previous config used this pair
    };
    const int attemptCount = sizeof(attempts) / sizeof(attempts[0]);

    bool ok = false;
    for (int i = 0; i < attemptCount; ++i) {
        int sda = attempts[i][0];
        int scl = attempts[i][1];
        Serial.printf("Trying I2C SDA=%d SCL=%d\n", sda, scl);

        // Re-init Wire for this pair
        customWire.end();
        delay(50);
        customWire.begin(sda, scl);
        customWire.setClock(100000);

        if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
            Serial.printf("OLED init OK on SDA=%d SCL=%d\n", sda, scl);
            ok = true;
            break;
        } else {
            Serial.printf("OLED not on SDA=%d SCL=%d\n", sda, scl);
            // small pause before trying next pair
            delay(50);
        }
    }

    if (!ok) {
        Serial.println("OLED not found at any tested pins");
        return;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Hello ESP32-CAM!");
    display.display();

    oled_enabled = true;
    Serial.println("OLED initialized successfully");
}

void disableOLED() {
    if (!oled_enabled) return;
    Serial.println("Disabling OLED to free pins for SD use");
    display.clearDisplay();
    display.display();
    delay(50);
    // End the custom Wire to release SDA/SCL GPIOs
    customWire.end();
    oled_enabled = false;
    // small pause so hardware settles
    delay(150);
}

bool isOLEDEnabled() { return oled_enabled; }

void clearDisplay() {
    if (!isOLEDEnabled()) return;
    display.clearDisplay();
    display.setCursor(0, 0);
}

void displayText(const String& text) {
    // Enable OLED and unmount SD if needed
    enableOLED();
    clearDisplay();
    // Ensure consistent text styling
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println(text);
    display.display();
    Serial.println("Display: " + text);
    // Keep text visible briefly, then return pins to SD
    delay(1000);
}

void displayWrappedText(const String& text) {
    // Simplified: just display the text plainly
    displayText(text);
}

void scrollText(const String& text, int line) {
    // Scrolling not needed; simply display the text on the requested line
    if (!isOLEDEnabled()) return;
    clearDisplay();
    display.setCursor(0, line * 8);
    display.println(text);
    display.display();
}

void displaySplashScreen() {
    if (!isOLEDEnabled()) return;
    clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 10);
    display.println("DeskMate");
    
    display.setTextSize(1);
    display.setCursor(10, 35);
    display.println("AI Companion");
    display.setCursor(20, 45);
    display.println("ESP32-CAM");
    
    display.display();
    delay(2000);
    
    // Reset text size
    display.setTextSize(1);
}