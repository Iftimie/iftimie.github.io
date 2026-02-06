#include "BluetoothA2DPSource.h"
#include "SD_MMC.h"

BluetoothA2DPSource a2dp;
File audioFile;

// Stream bytes from out.raw (raw PCM, no header)
int32_t raw_cb(uint8_t *data, int32_t len) {
  if (!audioFile) return 0;

  int bytes_read = audioFile.read(data, len);

  // loop on EOF: go back to start of file
  if (bytes_read <= 0) {
    audioFile.seek(0);
    bytes_read = audioFile.read(data, len);
    if (bytes_read <= 0) return 0;
  }
  return bytes_read;
}

void setup() {
  Serial.begin(115200);
  delay(800);

  // Mount ESP32-CAM built-in SD slot (1-bit mode is often most reliable)
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("❌ SD_MMC mount failed");
    while (true) delay(1000);
  }
  Serial.println("✅ SD_MMC mounted");

  // Open RAW PCM from SD root
  audioFile = SD_MMC.open("/out2.raw", FILE_READ);
  if (!audioFile) {
    Serial.println("❌ Missing /out2.raw on SD root");
    while (true) delay(1000);
  }
  Serial.println("✅ Opened /out2.raw");

  // ESP32 local BT name
  a2dp.set_local_name("ESP32-CAM Droid");

  // Feed audio from the RAW file
  a2dp.set_data_callback(raw_cb);

  // Optional: connection state prints
  a2dp.set_on_connection_state_changed([](esp_a2d_connection_state_t state, void*) {
    Serial.print("A2DP connection state: ");
    Serial.println((int)state);
  }, nullptr);

  Serial.println("Starting A2DP, connecting to headset...");

  // Connect to your headphones by Bluetooth device name
  a2dp.start("G435 Bluetooth Gaming Headset");
}

void loop() {
  // nothing needed; library pulls audio via callback
}
