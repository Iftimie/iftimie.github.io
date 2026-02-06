#include "tasks/ConvTask.h"
#include <Arduino.h>
#include "conversation_manager.h"
#include "deepseek_client.h"

void taskConversation(void* /*arg*/) {
  Serial.println("taskConversation starting");
  initConversationManager();

  // Wait for WiFi and start first subject
  while (!connectToWiFi()) {
    Serial.println("Waiting for WiFi...");
    vTaskDelay(pdMS_TO_TICKS(1000));
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
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
