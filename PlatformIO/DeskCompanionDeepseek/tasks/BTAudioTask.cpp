#include "tasks/BTAudioTask.h"
#include <Arduino.h>
#include "bluetooth_audio.h"

void taskBTAudio(void* /*arg*/) {
  Serial.println("taskBTAudio starting");

  // Initialize Bluetooth subsystem
  initBluetooth();

  // Start the background manager in bluetooth_audio (it will create its own task)
  startBluetoothManager();

  // Start the audio worker (mounts SD and creates its own task)
  startAudioTask();

  // This starter task can exit
  vTaskDelete(NULL);
}
