#include <Arduino.h>
#include <FS.h>

#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

static const int LED_PIN = 2; // D4 built-in LED on many ESP8266 boards (active LOW)

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;

unsigned long lastBlink = 0;
bool ledOn = false;

void listFS() {
  Serial.println("SPIFFS files:");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    Serial.printf("  %s  size=%u\n", dir.fileName().c_str(), dir.fileSize());
  }
}

void openAndPlay() {
  if (file) { delete file; file = nullptr; }
  file = new AudioFileSourceSPIFFS("/S0_loud2.mp3");

  if (!file->isOpen()) {
    Serial.println("MP3 file NOT open! (Check /data upload + filename)");
    return;
  }

  Serial.println("MP3 file opened. Starting playback...");
  mp3->begin(file, out);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED OFF

  Serial.println("\nBooting...");

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS begin failed!");
  } else {
    Serial.println("SPIFFS OK");
    listFS();
  }

  out = new AudioOutputI2S();
  // out->SetPinout( // THIS DOESN'T SEEM TO MAKE ANY DIFFERENCE
  //   14, // BCLK D5 (GPIO14)
  //   15, // LRC  D8 (GPIO15)
  //   13  // DIN  D7 (GPIO13)
  // );
  out->SetGain(0.7);

  mp3 = new AudioGeneratorMP3();

  openAndPlay();
}


void loop() {
  // LED heartbeat
  unsigned long now = millis();
  if (now - lastBlink >= 500) {
    lastBlink = now;
    ledOn = !ledOn;
    digitalWrite(LED_PIN, ledOn ? LOW : HIGH);
  }

  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      Serial.println("mp3->loop() failed -> stopping + restarting");
      mp3->stop();
      delay(200);
      openAndPlay();   // reopen and try again
    }
  } else {
    static unsigned long lastTry = 0;
    if (millis() - lastTry > 1000) {
      lastTry = millis();
      Serial.println("Not running -> retry");
      openAndPlay();
    }
  }
}
