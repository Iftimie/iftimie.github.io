#include <Arduino.h>
#include "tasks/TaskCommon.h"
#include "tasks/WebSocketTask.h"
#include "tasks/EyesTask.h"
#include "tasks/AudioTask.h"
#include "tasks/MotorsTask.h"
#include "freertos/task.h"

// define the shared queues (declared extern in TaskCommon.h)
QueueHandle_t g_cmd_q   = nullptr;
QueueHandle_t g_audio_q = nullptr;
QueueHandle_t g_motor_q = nullptr;
QueueHandle_t g_done_q  = nullptr;

// keep setup/loop tiny — tasks live in src/tasks

// ======================================================
// Arduino entry points
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  g_cmd_q   = xQueueCreate(CMD_QUEUE_LEN, sizeof(EyesCmd));
  g_audio_q = xQueueCreate(AUDIO_QUEUE_LEN, sizeof(AudioCmd));
  g_motor_q = xQueueCreate(MOTOR_QUEUE_LEN, sizeof(MotorCmd));
  g_done_q  = xQueueCreate(16, sizeof(DoneEvent));

  if (!g_cmd_q || !g_audio_q || !g_motor_q) {
    Serial.println("Failed to create queues");
    for (;;) delay(1000);
  }

  xTaskCreatePinnedToCore(taskWebSocket, "ws",     8192, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(taskEyes,      "eyes",   4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(taskAudio,     "audio",  4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(taskMotors,    "motors", 2048, nullptr, 2, nullptr, 1);

  Serial.println("Boot OK");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
