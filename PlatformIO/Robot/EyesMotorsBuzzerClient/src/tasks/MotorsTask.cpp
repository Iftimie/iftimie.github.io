#include "TaskCommon.h"
#include "MotorsTask.h"

// Motor UART (ESP32 -> ATtiny, TX only)
#define MOTOR_TX_PIN  12
#define MOTOR_BAUD    9600

void taskMotors(void* /*arg*/) {
  Serial2.begin(MOTOR_BAUD, SERIAL_8N1, -1, MOTOR_TX_PIN);

  MotorCmd cmd{};
  for (;;) {
    if (xQueueReceive(g_motor_q, &cmd, portMAX_DELAY) == pdTRUE) {
      const char* s = skipSpaces(cmd.text);
      Serial.printf("[MOTOR] tx: %s\n", s);

      if (startsWithNoCase(s, "move:")) {
        const char* payload = skipSpaces(afterPrefix(s, "move:"));
        // copy payload to mutable buffer
        char buf[CMD_MAX_LEN];
        strncpy(buf, payload, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        // check for optional trailing delay in ms after a comma (but don't remove it;
        // ATtiny expects the duration token to be present, e.g. "F,100")
        int delayMs = 0;
        char* lastComma = strrchr(buf, ',');
        if (lastComma) {
          char* tail = lastComma + 1;
          bool allDigits = true;
          for (char* p = tail; *p; ++p) if (!isdigit((unsigned char)*p)) { allDigits = false; break; }
          if (allDigits && tail[0] != '\0') {
            delayMs = atoi(tail);
            // keep the comma and digits in the payload so ATtiny can parse the step
          }
        }

        // send full payload (including delay) so the ATtiny receives the command and duration
        Serial2.print(buf);
        Serial2.print('\n');

        if (delayMs > 0) vTaskDelay(pdMS_TO_TICKS(delayMs));

        // signal done for this motor command if requested
        if (cmd.id != 0 && g_done_q) {
          DoneEvent de{cmd.id};
          xQueueSend(g_done_q, &de, 0);
        }
      }
    }
  }
}
