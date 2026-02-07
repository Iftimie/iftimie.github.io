#include "SD_MMC.h"

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.print("Listing directory: ");
  Serial.println(dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("❌ Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("❌ Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("📁 DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("📄 FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Mount SD card (ESP32-CAM built-in)
  if (!SD_MMC.begin("/sdcard", true)) {  // true = 1-bit mode
    Serial.println("❌ SD_MMC mount failed");
    while (true) delay(1000);
  }

  Serial.println("✅ SD_MMC mounted");

  // List root directory
  listDir(SD_MMC, "/", 2);
}

void loop() {
}
