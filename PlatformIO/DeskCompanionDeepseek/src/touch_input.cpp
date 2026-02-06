#include "touch_input.h"
#include "config.h"

TouchState currentTouchState = TOUCH_IDLE;
unsigned long lastTouchTime = 0;
bool touchDetectedFlag = false;

void initTouchSensor() {
    pinMode(TOUCH_SENSOR_PIN, INPUT_PULLUP);
    Serial.println("Touch sensor initialized on GPIO " + String(TOUCH_SENSOR_PIN));
}

bool isTouchDetected() {
    bool currentState = !digitalRead(TOUCH_SENSOR_PIN); // Active low typically
    
    switch (currentTouchState) {
        case TOUCH_IDLE:
            if (currentState) {
                currentTouchState = TOUCH_PRESSED;
                lastTouchTime = millis();
                touchDetectedFlag = true;
                Serial.println("Touch detected!");
                return true;
            }
            break;
            
        case TOUCH_PRESSED:
            if (!currentState) {
                currentTouchState = TOUCH_IDLE;
                touchDetectedFlag = false;
            } else if (millis() - lastTouchTime > TOUCH_DEBOUNCE_TIME) {
                // Still pressed after debounce - wait for release
            }
            break;
    }
    
    return false;
}

void handleTouchInput() {
    isTouchDetected(); // Update state
}

String getTouchResponse() {
    // Currently hardcoded to "Yes"
    return "Yes";
}

bool touchJustPressed() {
    if (touchDetectedFlag) {
        touchDetectedFlag = false; // Reset flag
        return true;
    }
    return false;
}