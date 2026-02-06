#include <SoftSerial.h>

// ---------------------------
// UART (RX only)
// ---------------------------
// RX on P0 (your original wiring), no TX
SoftSerial uart(0, -1);

// ---------------------------
// Motor driver inputs (YOUR layout)
// ---------------------------
const uint8_t R_FWD = 1; // Right motor forward
const uint8_t R_BWD = 2; // Right motor backward  (NOTE: not PWM-capable on ATtiny85, but we're doing software PWM)
const uint8_t L_FWD = 3; // Left motor forward
const uint8_t L_BWD = 4; // Left motor backward

// ---------------------------
// Batch / queue
// ---------------------------
const uint8_t MAX_STEPS = 16;

struct Step {
  char cmd;        // 'F','B','L','R'
  uint8_t t10ms;   // duration in 10ms units (0..255)
};

Step steps[MAX_STEPS];
uint8_t stepCount = 0;

// Parser state
char pendingCmd = 0;
bool haveCmd = false;

// ---------------------------
// Software PWM settings
// ---------------------------
// Duty: 0..255 (80 ≈ 31%, 128 ≈ 50%)
const uint8_t MOTOR_DUTY = 80;

// "PWM period" in microseconds. 2000us = 500Hz.
// Try 1000..5000 depending on motor/driver noise & smoothness.
const uint16_t PWM_US = 2000;

// Extra coast time before reversing direction (helps prevent brownout)
const uint16_t REV_DEADTIME_MS = 120;

// Track last executed command to detect reversals
char lastCmd = 0;

// ---------------------------
// Helpers
// ---------------------------
void allStop() {
  digitalWrite(R_FWD, LOW);
  digitalWrite(R_BWD, LOW);
  digitalWrite(L_FWD, LOW);
  digitalWrite(L_BWD, LOW);
}

bool isCmdChar(char c) {
  return (c == 'F' || c == 'B' || c == 'L' || c == 'R');
}

bool isReverse(char a, char b) {
  return (a == 'F' && b == 'B') || (a == 'B' && b == 'F') ||
         (a == 'L' && b == 'R') || (a == 'R' && b == 'L');
}

// Set the direction pins ON or OFF for a command
void setDirPins(char cmd, bool on) {
  uint8_t v = on ? HIGH : LOW;

  // Always clear first to avoid accidental overlap/glitches
  digitalWrite(R_FWD, LOW);
  digitalWrite(R_BWD, LOW);
  digitalWrite(L_FWD, LOW);
  digitalWrite(L_BWD, LOW);

  switch (cmd) {
    case 'F': // forward
      digitalWrite(R_FWD, v);
      digitalWrite(L_FWD, v);
      break;

    case 'B': // backward
      digitalWrite(R_BWD, v);
      digitalWrite(L_BWD, v);
      break;

    case 'L': // rotate left (right forward, left backward)
      digitalWrite(R_FWD, v);
      digitalWrite(L_BWD, v);
      break;

    case 'R': // rotate right (left forward, right backward)
      digitalWrite(L_FWD, v);
      digitalWrite(R_BWD, v);
      break;

    default:
      break;
  }
}

// Run one step using software PWM for the given duration
void runSoftPWM(char cmd, uint16_t duration_ms, uint8_t duty) {
  // Add dead-time if reversing direction
  if (lastCmd && isReverse(lastCmd, cmd)) {
    allStop();
    delay(REV_DEADTIME_MS);
  }

  uint32_t endAt = millis() + duration_ms;

  // duty 0..255 => on_us 0..PWM_US
  uint16_t on_us  = (uint32_t)PWM_US * duty / 255;
  uint16_t off_us = PWM_US - on_us;

  // Edge cases
  if (duty == 0) {
    allStop();
    delay(duration_ms);
    lastCmd = cmd;
    return;
  }

  if (duty == 255) {
    setDirPins(cmd, true);
    delay(duration_ms);
    allStop();
    lastCmd = cmd;
    return;
  }

  // Main PWM loop (blocking by design — OK for your batch runner)
  while ((int32_t)(millis() - endAt) < 0) {
    setDirPins(cmd, true);
    if (on_us) delayMicroseconds(on_us);

    setDirPins(cmd, false);
    if (off_us) delayMicroseconds(off_us);
  }

  allStop();
  lastCmd = cmd;
}

// Execute queued steps, then clear queue
void runBatch() {
  for (uint8_t i = 0; i < stepCount; i++) {
    uint16_t ms = (uint16_t)steps[i].t10ms * 10;

    runSoftPWM(steps[i].cmd, ms, MOTOR_DUTY);

    delay(50); // small gap between steps (optional)
  }

  stepCount = 0;
  haveCmd = false;
  pendingCmd = 0;
}
void loadMockSequence() {
  stepCount = 0;

  steps[stepCount++] = { 'F', 250 };  // 500 ms forward
  steps[stepCount++] = { 'L', 250 };  // 200 ms left
  steps[stepCount++] = { 'F', 250 };  // 400 ms forward
  steps[stepCount++] = { 'R', 250 };  // 200 ms right
  steps[stepCount++] = { 'B', 250 };  // 300 ms backward
}

// ---------------------------
// Setup / Loop
// ---------------------------
void setup() {
  pinMode(R_FWD, OUTPUT);
  pinMode(R_BWD, OUTPUT);
  pinMode(L_FWD, OUTPUT);
  pinMode(L_BWD, OUTPUT);

  allStop();
  uart.begin(9600);

  // loadMockSequence();
  // runBatch();
}

// void loop() {}

void loop() {
  while (uart.available()) {
    char c = uart.read();

    // End of batch
    if (c == ';' || c == '\n') {
      if (stepCount > 0) runBatch();
      continue;
    }

    // Ignore separators
    if (c == ' ' || c == ',' || c == '\r' || c == '\t') continue;

    // Command letter
    if (isCmdChar(c)) {
      pendingCmd = c;
      haveCmd = true;
      continue;
    }

    // Parse time digits after a command (0..255)
    if (haveCmd && c >= '0' && c <= '9') {
      uint16_t val = (uint8_t)(c - '0');

      // read up to 2 more digits if they arrive quickly
      for (uint8_t k = 0; k < 2; k++) {
        uint32_t start = millis();
        while (!uart.available() && (millis() - start) < 5) { /* tiny wait */ }

        if (!uart.available()) break;

        char d = uart.peek();
        if (d < '0' || d > '9') break;

        uart.read(); // consume
        val = val * 10 + (uint8_t)(d - '0');
      }

      if (val > 255) val = 255;

      if (stepCount < MAX_STEPS) {
        steps[stepCount++] = { pendingCmd, (uint8_t)val };
      }

      haveCmd = false;
      pendingCmd = 0;
      continue;
    }

    // Anything else: ignore
  }
}
