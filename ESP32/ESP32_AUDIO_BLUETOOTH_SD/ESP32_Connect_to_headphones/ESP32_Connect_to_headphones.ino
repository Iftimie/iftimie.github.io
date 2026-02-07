#include "BluetoothA2DPSource.h"

BluetoothA2DPSource a2dp;

// Dummy audio callback: sends silence so the stream is valid
int32_t silence_cb(uint8_t *data, int32_t len) {
  memset(data, 0, len);
  return len;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Name of your ESP32 as seen over Bluetooth
  a2dp.set_local_name("ESP32-CAM Droid");

  // Provide audio (silence) so A2DP can run
  a2dp.set_data_callback(silence_cb);

  // OPTIONAL: print connection state changes
  a2dp.set_on_connection_state_changed([](esp_a2d_connection_state_t state, void*) {
    Serial.print("A2DP connection state: ");
    Serial.println((int)state);
  }, nullptr);

  Serial.println("Starting A2DP, connecting to headset...");

  // Connect to your headphones by Bluetooth device name
  a2dp.start("G435 Bluetooth Gaming Headset");
}

void loop() {
  // nothing needed
}
