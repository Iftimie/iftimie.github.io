// Task-based launcher: create lightweight tasks for Conversation and BT/Audio
#include <Arduino.h>
#include "config.h"
#include "conversation_manager.h"
#include "deepseek_client.h"
#include "bluetooth_audio.h" // kept for reference; bluetooth/audio startup disabled below
#include "oled_display.h"
#include "touch_input.h"
#include "freertos/task.h"

// Task prototypes implemented in tasks/*.cpp
void taskConversation(void* arg);
void taskBTAudio(void* arg);

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("DeskMate: starting task-based launcher");

  // Initialize display and touch before starting tasks so UI calls are safe
  initDisplay();
  displaySplashScreen();
  initTouchSensor();

  // Create conversation task
  xTaskCreatePinnedToCore(taskConversation, "conv", 8192, nullptr, 2, nullptr, 1);

  // Start Bluetooth+SD starter task (creates BT manager + audio worker)
  xTaskCreatePinnedToCore(taskBTAudio, "btaudio", 4096, nullptr, 1, nullptr, 1);

  Serial.println("Tasks created");
}

void loop() {
  // Nothing here - tasks handle all work
  vTaskDelay(pdMS_TO_TICKS(1000));
}
