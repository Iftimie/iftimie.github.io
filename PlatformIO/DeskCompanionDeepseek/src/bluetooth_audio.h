#ifndef BLUETOOTH_AUDIO_H
#define BLUETOOTH_AUDIO_H

#include <Arduino.h>
#include <BluetoothA2DPSource.h>
#include <SD_MMC.h>
#include "config.h"

// Audio callback function
int32_t get_audio_data(uint8_t *data, int32_t len);

// Function declarations
void initBluetooth();
bool connectToHeadset();
void playNotificationSound();
void initSDCard();
void setupAudioCallback();
void startBluetoothManager();
void startAudioTask();
bool enqueueAudioNotification();

// Helpers to temporarily suspend/resume background audio/BT tasks
void suspendBluetoothManager();
void resumeBluetoothManager();
void suspendAudioTask();
void resumeAudioTask();

// Unmount/close SD to free internal heap before TLS operations
void unmountSD();

// Full Bluetooth controller shutdown / restart helpers
void shutdownBluetoothController();
bool restartBluetoothController();

// Stop/start background tasks entirely (deletes task handles so stacks are freed)
void stopBluetoothManager();
void stopAudioTaskNow();

// Startup helpers
bool waitForBTAndAudioStartup(uint32_t timeout_ms);
bool isBTManagerStarted();
bool isSDMounted();
// Wait until a headset is connected (A2DP connected). Returns true if connected.
bool waitForHeadsetConnection(uint32_t timeout_ms);
// Pause/resume A2DP callbacks to reduce driver activity during TLS
void pauseBluetoothForTLS();
void resumeBluetoothAfterTLS();

// Global variables
extern BluetoothA2DPSource a2dp;
extern File audioFile;
extern bool audioInitialized;

#endif // BLUETOOTH_AUDIO_H