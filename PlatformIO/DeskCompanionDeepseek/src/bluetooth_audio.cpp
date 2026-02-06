#include "bluetooth_audio.h"
#include "config.h"
#include "oled_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
// ESP-IDF Bluetooth controller functions
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_err.h"

BluetoothA2DPSource a2dp;
File audioFile;
bool audioInitialized = false;

// Play-once flag: when true, audio callback will play the file a single
// time and then stop (no looping). Cleared automatically after EOF.
static volatile bool play_notification_once = false;
// Set when a one-shot notification has finished playing; prevents looping
// until the next `playNotificationSound()` call resets it.
static volatile bool notification_played = false;

// Startup status flags
static volatile bool bt_manager_started = false;
static volatile bool sd_mounted = false;
// Headset connection flag (set in A2DP connection callback)
// `audioInitialized` is already global; reuse it for connection state.
static volatile bool a2dp_paused = false;

// Install A2DP callbacks (used at init and when resuming after TLS)
static void setupA2DPCallbacks() {
    a2dp.set_data_callback(get_audio_data);
    a2dp.set_on_connection_state_changed([](esp_a2d_connection_state_t state, void*) {
        Serial.print("A2DP connection state: ");
        Serial.println((int)state);

        if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
            Serial.println("Bluetooth headset connected!");
            audioInitialized = true;
        } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
            Serial.println("Bluetooth headset disconnected!");
            audioInitialized = false;
        }
    }, nullptr);
}

static QueueHandle_t audio_q = NULL;
static TaskHandle_t audio_task_handle = NULL;

int32_t get_audio_data(uint8_t *data, int32_t len) {
    if (!audioFile) return 0;

    // If a one-shot notification already finished, return 0 to silence
    // further playback until explicitly requested again.
    if (notification_played) return 0;

    int bytes_read = audioFile.read(data, len);

    if (play_notification_once) {
        // Play once mode: when EOF reached, stop playback and mark played.
        if (bytes_read <= 0) {
            play_notification_once = false;
            notification_played = true;
            return 0;
        }
        return bytes_read;
    }

    // Normal (loop) behavior: on EOF, rewind and continue looping.
    if (bytes_read <= 0) {
        audioFile.seek(0);
        bytes_read = audioFile.read(data, len);
        if (bytes_read <= 0) return 0;
    }
    return bytes_read;
}

void initBluetooth() {
    Serial.println("Initializing Bluetooth...");
    
    // Set ESP32 local BT name
    a2dp.set_local_name("ESP32-CAM DeskMate");
    
    // Set audio callback
    setupA2DPCallbacks();
    
    Serial.println("Bluetooth initialized");
}

bool connectToHeadset() {
    Serial.println("Connecting to Bluetooth headset...");
    
    // Connect to your headphones by Bluetooth device name
    a2dp.start(BT_DEVICE_NAME);
    
    return true;
}

void playNotificationSound() {
    // If audio system not yet connected, still attempt to initialize SD when needed.
    if (!audioInitialized) {
        Serial.println("Audio not initialized - cannot play sound (will still try to open SD)");
        // Try to lazily initialize SD in case user inserted it later
        if (!audioFile) {
            initSDCard();
        }
        return;
    }

    // Ensure audio file is available; if not, try SD init once
    if (!audioFile) initSDCard();

    // Choose which folder to play from: 50/50 between /beeps and /serenity
    const char* folderA = "/beeps";
    const char* folderB = "/serenity";

    String chosenPath = "";
    // Try to pick a random file from selected folder
    const int pick = random(0, 2); // 0 or 1
    const char* selFolder = (pick == 0) ? folderA : folderB;
    chosenPath = ""; // will be set by helper
    
    // Helper: collect files in directory and pick one at random
    auto pickFromDir = [&](const char* dirPath) -> String {
        File dir = SD_MMC.open(dirPath);
        if (!dir) return String("");
        const int MAX_FILES = 64;
        String files[MAX_FILES];
        int count = 0;
        File entry;
        // iterate directory
        while ((entry = dir.openNextFile())) {
            if (!entry.isDirectory()) {
                if (count < MAX_FILES) files[count++] = String(dirPath) + "/" + String(entry.name());
            }
            entry.close();
        }
        dir.close();
        if (count == 0) return String("");
        int idx = random(0, count);
        return files[idx];
    };

    chosenPath = pickFromDir(selFolder);
    // If chosen folder empty, try the other folder
    if (chosenPath.length() == 0) {
        const char* other = (selFolder == folderA) ? folderB : folderA;
        chosenPath = pickFromDir(other);
    }

    // Fallback to default file if still empty
    if (chosenPath.length() == 0) {
        Serial.println("No files found in beeps/serenity; falling back to /out2.raw");
        chosenPath = String(AUDIO_FILE_PATH);
    }

    // Close any previously opened audio file
    if (audioFile) {
        audioFile.close();
    }

    audioFile = SD_MMC.open(chosenPath, FILE_READ);
    if (audioFile) {
        Serial.print("Playing notification: "); Serial.println(chosenPath);
        audioFile.seek(0);
        notification_played = false;
        play_notification_once = true;
    } else {
        Serial.print("Failed to open "); Serial.println(chosenPath);
    }
}

void initSDCard() {
    Serial.println("Initializing SD card...");
    // Try to mount SD without disabling OLED first so display stays
    // visible when possible. If mounting fails, then disable the OLED
    // (freeing I2C pins) and retry once.

    bool mounted = false;
    if (SD_MMC.begin("/sdcard", true)) {
        Serial.println("✅ SD_MMC mounted (no OLED change)");
        mounted = true;
    } else {
        Serial.println("SD_MMC mount failed on first attempt; trying after disabling OLED");
        // Remember whether OLED was enabled so we can restore it after mount
        bool wasOLED = isOLEDEnabled();
        // Temporarily release OLED pins and retry
        disableOLED();
        delay(150);
        if (SD_MMC.begin("/sdcard", true)) {
            Serial.println("✅ SD_MMC mounted after disabling OLED");
            mounted = true;
            // If we disabled the OLED only to mount SD, try to restore it now
            if (wasOLED) {
                enableOLED();
                Serial.println("Restored OLED after SD mount");
            }
        } else {
            Serial.println("❌ SD_MMC mount failed after disabling OLED");
            // leave OLED disabled state as we attempted to free pins
            mounted = false;
        }
    }

    if (!mounted) {
        sd_mounted = false;
        return;
    }
    sd_mounted = true;

    // Open RAW PCM from SD root
    audioFile = SD_MMC.open(AUDIO_FILE_PATH, FILE_READ);
    if (!audioFile) {
        Serial.print("❌ Missing ");
        Serial.println(AUDIO_FILE_PATH);
        Serial.println(" on SD root");
        return;
    }
    Serial.print("✅ Opened ");
    Serial.println(AUDIO_FILE_PATH);
}

void setupAudioCallback() {
    // This is called in initBluetooth()
    Serial.println("Audio callback setup complete");
}

// Bluetooth manager task: tries to (re)connect periodically so audio persists
static TaskHandle_t bt_manager_handle = NULL;
static volatile bool bt_manager_suspended = false;
static void bt_manager_task(void* pv) {
    (void)pv;
    unsigned long last_ctrl_attempt = 0;
    for (;;) {
        if (bt_manager_suspended) {
            // Gentle suspend: keep task alive but skip heavy operations.
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if (!audioInitialized) {
            // Check controller + bluedroid status before starting A2DP
            esp_bt_controller_status_t ctrl = esp_bt_controller_get_status();
            esp_bluedroid_status_t bd = esp_bluedroid_get_status();
            if (ctrl == ESP_BT_CONTROLLER_STATUS_ENABLED && bd == ESP_BLUEDROID_STATUS_ENABLED) {
                Serial.println("BT manager: controller ready; attempting connect to headset...");
                a2dp.start(BT_DEVICE_NAME);
            } else {
                Serial.print("BT manager: stack not ready (ctrl="); Serial.print((int)ctrl);
                Serial.print(" bd="); Serial.print((int)bd);
                Serial.println(") - will retry later and now nonetheless");
                a2dp.start(BT_DEVICE_NAME);

            }
        }

        // Back off more to avoid spamming A2DP start requests during
        // controller bring-up/reconnect storms.
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}

void startBluetoothManager() {
    if (bt_manager_handle == NULL) {
        xTaskCreatePinnedToCore(bt_manager_task, "bt_mgr", 4096, NULL, 1, &bt_manager_handle, 1);
        Serial.println("Started Bluetooth manager task");
        bt_manager_started = true;
    }
}

bool isBTManagerStarted() {
    return bt_manager_started;
}

bool isSDMounted() {
    return sd_mounted;
}

bool waitForHeadsetConnection(uint32_t timeout_ms) {
    unsigned long start = millis();
    while (millis() - start < timeout_ms) {
        if (audioInitialized) return true;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return audioInitialized;
}

void pauseBluetoothForTLS() {
    if (a2dp_paused) return;
    Serial.println("Pausing A2DP callbacks for TLS");
    // Remove callbacks to reduce driver activity and prevent stray calls
    a2dp.set_on_connection_state_changed(nullptr, nullptr);
    a2dp.set_data_callback(nullptr);
    a2dp_paused = true;
}

void resumeBluetoothAfterTLS() {
    if (!a2dp_paused) return;
    Serial.println("Restoring A2DP callbacks after TLS");
    setupA2DPCallbacks();
    a2dp_paused = false;
}

bool waitForBTAndAudioStartup(uint32_t timeout_ms) {
    unsigned long start = millis();
    while (millis() - start < timeout_ms) {
        if (bt_manager_started && sd_mounted) return true;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return (bt_manager_started && sd_mounted);
}

// Audio task: maintains SD file availability and responds to enqueue events
static void audio_task(void* pv) {
    (void)pv;
    // Try to mount SD once at start (user may insert later)
    initSDCard();
    for (;;) {
        uint8_t msg;
        if (audio_q && xQueueReceive(audio_q, &msg, portMAX_DELAY) == pdTRUE) {
            // Ensure audio file is ready
            if (!audioFile) initSDCard();
            // Trigger playback; playNotificationSound() will be safe to call
            playNotificationSound();
        }
    }
}

void startAudioTask() {
    if (audio_q == NULL) {
        audio_q = xQueueCreate(8, sizeof(uint8_t));
    }
    if (audio_task_handle == NULL) {
        xTaskCreatePinnedToCore(audio_task, "audio_task", 4096, NULL, 1, &audio_task_handle, 1);
        Serial.println("Started audio task");
    }
}

bool enqueueAudioNotification() {
    if (audio_q == NULL) return false;
    uint8_t v = 1;
    BaseType_t ok = xQueueSend(audio_q, &v, 0);
    return ok == pdTRUE;
}

// Suspend/resume helpers --------------------------------------------------
void suspendBluetoothManager() {
    // Request a gentle suspend: set a flag so the manager remains alive but
    // skips connection attempts. This avoids suspending the task (which can
    // interfere with scheduler timing) while still reducing activity.
    bt_manager_suspended = true;
    Serial.println("BT manager gentle-suspend requested");
}

void resumeBluetoothManager() {
    bt_manager_suspended = false;
    Serial.println("BT manager gentle-resume requested");
}

// Stop (delete) the bluetooth manager task to free its stack and resources.
void stopBluetoothManager() {
    if (bt_manager_handle != NULL) {
        vTaskDelete(bt_manager_handle);
        bt_manager_handle = NULL;
        Serial.println("Stopped BT manager (task deleted)");
    }
}

void suspendAudioTask() {
    if (audio_task_handle != NULL) {
        vTaskSuspend(audio_task_handle);
        Serial.println("Suspended audio task");
    }
}

void resumeAudioTask() {
    if (audio_task_handle != NULL) {
        vTaskResume(audio_task_handle);
        Serial.println("Resumed audio task");
    }
}

// Stop (delete) the audio task to free its stack and resources.
void stopAudioTaskNow() {
    if (audio_task_handle != NULL) {
        vTaskDelete(audio_task_handle);
        audio_task_handle = NULL;
        Serial.println("Stopped audio task (task deleted)");
    }
}

// Fully shut down the Bluetooth controller and bluedroid stack to release
// driver/internal allocations that persist beyond the A2DP task.
void shutdownBluetoothController() {
    Serial.println("Shutting down Bluetooth controller and stack...");

    // Remove callbacks first to avoid stray calls
    a2dp.set_on_connection_state_changed(nullptr, nullptr);
    a2dp.set_data_callback(nullptr);

    // Try disabling bluedroid then the controller
    esp_err_t err = esp_bluedroid_disable();
    if (err == ESP_OK) {
        esp_bluedroid_deinit();
    }
    err = esp_bt_controller_disable();
    if (err == ESP_OK) {
        esp_bt_controller_deinit();
    }

    Serial.println("Bluetooth controller shut down");
}

// Restart the Bluetooth controller and bluedroid stack. Returns true on success.
bool restartBluetoothController() {
    Serial.println("Restarting Bluetooth controller and stack...");
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    const int MAX_ATTEMPTS = 3;
    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
        Serial.print("BT restart attempt: "); Serial.println(attempt);

        esp_bt_controller_status_t ctrlStat = esp_bt_controller_get_status();
        esp_bluedroid_status_t bdStat = esp_bluedroid_get_status();

        // If controller already enabled, skip init/enable steps for controller
        if (ctrlStat != ESP_BT_CONTROLLER_STATUS_ENABLED) {
            if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
                Serial.println("Attempt: Failed to init BT controller");
                // small backoff and retry
                delay(200);
                continue;
            }
            if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK) {
                Serial.println("Attempt: Failed to enable BT controller");
                esp_bt_controller_deinit();
                delay(200);
                continue;
            }
        }

        // Init/enable bluedroid if not already enabled
        if (bdStat != ESP_BLUEDROID_STATUS_ENABLED) {
            if (esp_bluedroid_init() != ESP_OK) {
                Serial.println("Attempt: Failed to init bluedroid");
                // try disabling controller so next attempt starts clean
                esp_bt_controller_disable();
                esp_bt_controller_deinit();
                delay(200);
                continue;
            }
            if (esp_bluedroid_enable() != ESP_OK) {
                Serial.println("Attempt: Failed to enable bluedroid");
                esp_bluedroid_deinit();
                esp_bt_controller_disable();
                esp_bt_controller_deinit();
                delay(200);
                continue;
            }
        }

        // If we reach here both controller and bluedroid should be enabled
        esp_bt_controller_status_t finalCtrl = esp_bt_controller_get_status();
        esp_bluedroid_status_t finalBd = esp_bluedroid_get_status();
        Serial.print("BT status after restart: ctrl="); Serial.print((int)finalCtrl);
        Serial.print(" bd="); Serial.println((int)finalBd);

        // Reinitialize A2DP callbacks and name
        initBluetooth();
        Serial.println("Bluetooth controller restarted");
        return true;
    }

    Serial.println("All BT restart attempts failed");
    return false;
}

void unmountSD() {
    if (audioFile) {
        Serial.println("Closing audio file and unmounting SD");
        audioFile.close();
    }
    // End SD_MMC to free internal resources
    SD_MMC.end();
    sd_mounted = false;
    // Small delay to let controller settle
    delay(200);
}
