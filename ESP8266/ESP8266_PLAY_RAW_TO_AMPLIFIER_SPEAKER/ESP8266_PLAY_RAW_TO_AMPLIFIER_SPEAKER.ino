#include <Arduino.h>

#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;

// ===== LED =====
const int LED_PIN = 2;   // D4, built-in LED (active LOW)
unsigned long lastBlink = 0;
bool ledState = false;

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED off

  Serial.println("Booting...");

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS failed!");
  } else {
    Serial.println("SPIFFS OK");
  }

  Serial.println("Setting up audio...");

  out = new AudioOutputI2SNoDAC();
  out->SetPinout(
    14, // BCLK  D5
    15, // LRC   D8
    13  // DIN   D7
  );
  out->SetGain(0.5);

  file = new AudioFileSourceSPIFFS("/S0.mp3");
  mp3  = new AudioGeneratorMP3();

  if (!file->isOpen()) {
    Serial.println("MP3 file NOT open!");
  } else {
    Serial.println("MP3 file opened");
  }

  mp3->begin(file, out);
  Serial.println("Playback started");
}

void loop() {
  // ===== LED heartbeat (500 ms) =====
  unsigned long now = millis();
  if (now - lastBlink >= 500) {
    lastBlink = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? LOW : HIGH);
  }

  // ===== Audio loop =====
  if (mp3->isRunning()) {
    mp3->loop();
  } else {
    mp3->stop();
    Serial.println("Playback ended, restarting...");
    delay(1000);
    mp3->begin(file, out);
  }
}
