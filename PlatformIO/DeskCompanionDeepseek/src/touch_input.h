#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include <Arduino.h>
#include "config.h"

// Touch sensor states
enum TouchState {
    TOUCH_IDLE,
    TOUCH_PRESSED,
    TOUCH_DEBOUNCING
};

// Function declarations
void initTouchSensor();
bool isTouchDetected();
void handleTouchInput();
String getTouchResponse(); // Currently hardcoded to "Yes"
bool touchJustPressed();

// Global variables
extern TouchState currentTouchState;
extern unsigned long lastTouchTime;
extern bool touchDetectedFlag;

#endif // TOUCH_INPUT_H