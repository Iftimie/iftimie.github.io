#include "tasks/ConvTask.h"
#include <Arduino.h>
#include "conversation_manager.h"
#include "deepseek_client.h"
#include "bluetooth_audio.h"
#include "touch_input.h"

void taskConversation(void* /*arg*/) {
  Serial.println("taskConversation starting");
  initConversationManager();

  // Wait for WiFi and start first subject
  while (!connectToWiFi()) {
    Serial.println("Waiting for WiFi...");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  // Wait until BT manager and SD/audio have started before starting
  // the conversation to ensure audio output is available.
  Serial.println("Waiting for BT and audio startup...");
  bool ready = waitForBTAndAudioStartup(15000);
  if (!ready) {
    Serial.println("Warning: BT/audio did not fully start before timeout");
  }
  // Block until headset connects so the first LLM request has audio output.
  Serial.println("Waiting for headset connection (up to 30s)...");
  bool headset = waitForHeadsetConnection(30000);
  if (!headset) {
    Serial.println("Warning: headset did not connect before timeout; proceeding anyway");
  } else {
    Serial.println("Headset connected; proceeding with first request");
  }
  startNewSubject();

  // Move Serial input handling into this task
  auto readLine = []() -> String {
    String s;
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') break;
      s += c;
    }
    return s;
  };

  for (;;) {
    if (Serial.available()) {
      String line = readLine();
      line.trim();
      if (line.length()) {
        line.toLowerCase();
        onUserInput(line);
      }
    }
    // Poll touch sensor and treat a press as the "more" command
    handleTouchInput();
    if (touchJustPressed()) {
      onUserInput("more");
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
