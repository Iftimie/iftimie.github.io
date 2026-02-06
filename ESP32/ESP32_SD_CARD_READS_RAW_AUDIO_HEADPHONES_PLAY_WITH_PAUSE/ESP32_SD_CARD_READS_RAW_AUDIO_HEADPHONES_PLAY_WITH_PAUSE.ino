#include "BluetoothA2DPSource.h"
#include "SD_MMC.h"

BluetoothA2DPSource a2dp;
File audioFile;

const char* HEADSET_NAME = "G435 Bluetooth Gaming Headset";

volatile bool playEnabled = false;
volatile bool playbackDone = false;

int32_t raw_cb(uint8_t *data, int32_t len) {
  if (!playEnabled || !audioFile) {
    memset(data, 0, len);
    return len;
  }

  int bytes_read = audioFile.read(data, len);
  if (bytes_read <= 0) {
    playbackDone = true;
    playEnabled = false;
    memset(data, 0, len);
    return len;
  }

  if (bytes_read < len) {
    memset(data + bytes_read, 0, len - bytes_read);
    return len;
  }
  return bytes_read;
}

void startAndPlayOnce() {
  audioFile.seek(0);
  playbackDone = false;
  playEnabled = true;

  a2dp.set_local_name("ESP32-CAM Droid");
  a2dp.set_data_callback(raw_cb);

  Serial.println("A2DP start()");
  a2dp.start(HEADSET_NAME);
}

void stopA2dpSafe() {
  // stop feeding audio first
  playEnabled = false;
  delay(250);     // allow drain

  Serial.println("A2DP end()");
  a2dp.end();     // or a2dp.stop()
  delay(250);
}

void runCycles(int cycles) {
  for (int i = 0; i < cycles; i++) {
    Serial.printf("\nCycle %d: START+PLAY ONCE\n", i + 1);
    startAndPlayOnce();

    uint32_t t0 = millis();
    while (!playbackDone && millis() - t0 < 60000) delay(20);

    Serial.printf("Cycle %d: STOP\n", i + 1);
    stopA2dpSafe();

    delay(1500);
  }
}

void setup() {
  Serial.begin(115200);
  delay(800);

  if (!SD_MMC.begin("/sdcard", true)) while (true) delay(1000);
  audioFile = SD_MMC.open("/out2.raw", FILE_READ);
  if (!audioFile) while (true) delay(1000);

  Serial.println("Press 'c' to run cycles.");
}

void loop() {
  if (Serial.available() && Serial.read() == 'c') {
    runCycles(10);
  }
}
