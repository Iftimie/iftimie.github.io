#include "tasks/BTAudioTask.h"
#include <Arduino.h>
#include "bluetooth_audio.h"
#include "freertos/task.h"

// Ensure prototypes visible in this translation unit (safe forward declarations)
void startBluetoothManager();
void startAudioTask();

void taskBTAudio(void* /*arg*/) {
  Serial.println("taskBTAudio starting");

  // Initialize Bluetooth subsystem
  initBluetooth();

  // Ensure the Bluetooth controller and bluedroid stack are initialized
  // at startup so the manager can bring up A2DP and connect to headsets.
  Serial.println("Ensuring BT controller and stack are started...");
  bool ctrl_ok = restartBluetoothController();
  if (!ctrl_ok) {
    Serial.println("Warning: Bluetooth controller startup failed; manager may retry later");
  }

  // Start the background manager in bluetooth_audio (it will create its own task)
  startBluetoothManager();

  // Start the audio worker (mounts SD and creates its own task)
  startAudioTask();

  // This starter task can exit
  vTaskDelete(NULL);
}
