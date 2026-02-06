#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// Custom Wire instance for software I2C on pins 13/16
extern TwoWire customWire;
extern Adafruit_SSD1306 display;

void initDisplay();
void clearDisplay();
void displayText(const String& text);
void displayWrappedText(const String& text);
void scrollText(const String& text, int line = 0);
void displaySplashScreen();
// Hardware arbitration: enable/disable OLED when sharing pins with SD
void enableOLED();
void disableOLED();
bool isOLEDEnabled();

#endif // OLED_DISPLAY_H