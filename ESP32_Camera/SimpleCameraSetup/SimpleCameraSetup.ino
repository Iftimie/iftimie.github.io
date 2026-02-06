#include <Arduino.h>
#include "esp_camera.h"
#include "esp_system.h"
#include "esp_err.h"

// ===== Select your camera model pins (AI Thinker) =====
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// --- LED helpers (paste once, e.g., above setup()) ---
static inline void ensure_led_setup() {
#ifdef LED_GPIO_NUM
  static bool inited = false;
  if (!inited) {
    pinMode(LED_GPIO_NUM, OUTPUT);
    digitalWrite(LED_GPIO_NUM, LOW); // LED off initially (active-HIGH on most ESP32-CAM)
    inited = true;
  }
#endif
}

static inline void led_on()  {
#ifdef LED_GPIO_NUM
  digitalWrite(LED_GPIO_NUM, HIGH);
#endif
}

static inline void led_off() {
#ifdef LED_GPIO_NUM
  digitalWrite(LED_GPIO_NUM, LOW);
#endif
}

static void blink_short_triple() {
#ifdef LED_GPIO_NUM
  ensure_led_setup();
  for (int i = 0; i < 3; ++i) {
    led_on();  delay(120);
    led_off(); delay(120);
  }
#endif
}

static void blink_long_once() {
#ifdef LED_GPIO_NUM
  ensure_led_setup();
  led_on();  delay(600);
  led_off(); // no trailing delay needed
#endif
}
// --- end LED helpers ---


// ---------- Helpers ----------
static void print_memory(const char* tag) {
  Serial.printf("[%s] HEAP free=%u, PSRAM total=%u, PSRAM free=%u\n",
                tag,
                (unsigned)esp_get_free_heap_size(),
                (unsigned)ESP.getPsramSize(),
                (unsigned)ESP.getFreePsram());
}

static void print_sensor_info() {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) {
    Serial.println("sensor_t* is NULL (no sensor?)");
    return;
  }
  Serial.printf("Sensor PID: 0x%04x\n", s->id.PID);
  Serial.printf("XCLK set to: %u Hz\n", s->xclk_freq_hz);
  Serial.printf("Current pixformat: %u (0=RGB565, 3=JPEG, etc.)\n", s->pixformat);
  Serial.printf("Status -> framesize:%u quality:%d brightness:%d contrast:%d saturation:%d\n",
                s->status.framesize, s->status.quality, s->status.brightness,
                s->status.contrast, s->status.saturation);
}

static void print_fb_info(const camera_fb_t* fb) {
  if (!fb) return;
  // width/height are available in newer esp32-camera; if unavailable they will be 0
  Serial.printf("FB len=%u bytes, format=%u, w=%u h=%u\n",
                (unsigned)fb->len, (unsigned)fb->format,
                (unsigned)fb->width, (unsigned)fb->height);
  Serial.printf("Timestamp: %ld.%06ld\n", (long)fb->timestamp.tv_sec, (long)fb->timestamp.tv_usec);
}

// Try to capture a frame with retries
static bool try_capture_once(uint32_t timeout_ms) {
  const uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) {
      Serial.println("Capture: OK");
      print_fb_info(fb);
      esp_camera_fb_return(fb);
      return true;
    }
    delay(10);
  }
  Serial.println("Capture: FAILED (timeout)");
  return false;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  delay(300);
  Serial.println();
  Serial.println("=== ESP32-CAM Safe Capture Test ===");

  print_memory("boot");

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  // AI Thinker pin map (from camera_pins.h)
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;   // 32 on AI-Thinker
  config.pin_reset = RESET_GPIO_NUM; // -1 on AI-Thinker

  // Conservative clock and tiny frame to minimize RAM/throughput issues
  config.xclk_freq_hz = 20000000;        // 10 MHz is “safe”; OV2640 usually fine at 10–20 MHz
  config.frame_size   = FRAMESIZE_QQVGA; // 160x120
  config.pixel_format = PIXFORMAT_JPEG;  // lowest RAM usage with OV2640
  config.jpeg_quality = 15;              // higher number = more compression, smaller buffer
  config.fb_count     = 1;               // single frame buffer
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
    Serial.println("PSRAM detected -> using PSRAM for frame buffer.");
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
    Serial.println("No PSRAM detected -> using DRAM for frame buffer.");
  }

  // Optional: power-cycle the sensor via PWDN to clear odd states
  if (config.pin_pwdn >= 0) {
    pinMode(config.pin_pwdn, OUTPUT);
    digitalWrite(config.pin_pwdn, HIGH);
    delay(50);
    digitalWrite(config.pin_pwdn, LOW);
    delay(50);
  }

  print_memory("pre-init");

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("esp_camera_init failed: 0x%x (%s)\n", err, esp_err_to_name(err));
    Serial.println("Retrying with XCLK=20 MHz and lower JPEG quality...");
    // Retry once with slightly different clock/quality
    config.xclk_freq_hz = 20000000;
    config.jpeg_quality  = 20;
    err = esp_camera_init(&config);
  }

  if (err != ESP_OK) {
    Serial.printf("Second init attempt failed: 0x%x (%s)\n", err, esp_err_to_name(err));
    Serial.println("As a last resort, switch to RGB565 (larger buffer) but same tiny framesize.");
    // One more attempt in RGB565, still QQVGA (some modules behave differently)
    camera_config_t config2 = config;
    config2.pixel_format = PIXFORMAT_RGB565;
    config2.jpeg_quality = 15; // ignored in RGB565
    err = esp_camera_init(&config2);
  }

  if (err != ESP_OK) {
    Serial.printf("Final init failed: 0x%x (%s)\n", err, esp_err_to_name(err));
    Serial.println("Stopping here. Check pins, 3.3V stability, ribbon cable, and sensor.");
    return;
  }

  Serial.println("Camera initialized.");
  print_memory("post-init");
  print_sensor_info();

  // Gentle sensor tweaks (optional, safe)
  sensor_t* s = esp_camera_sensor_get();
  if (s && s->id.PID == OV2640_PID) {
    s->set_framesize(s, FRAMESIZE_QQVGA);  // ensure tiny
    s->set_quality(s, 15);
    s->set_brightness(s, 0);
    s->set_saturation(s, 0);
    s->set_gain_ctrl(s, 1);      // AGC on
    s->set_exposure_ctrl(s, 1);  // AEC on
  }

  // Try a few captures
  for (int i = 1; i <= 5; ++i) {
    Serial.printf("--- Capture attempt %d ---\n", i);
    bool ok = try_capture_once(500 /*ms*/);
    if (!ok) {
      Serial.println("Note: If this keeps failing, try:");
      Serial.println(" - Re-seat camera ribbon");
      Serial.println(" - Lower XCLK to 8–10 MHz");
      Serial.println(" - Keep PIXFORMAT_JPEG + QQVGA");
      Serial.println(" - Ensure 5V supply is solid (>=500mA), board 3.3V regulator not sagging");
      blink_short_triple();   // << blink 3× on failure
    } else {
      blink_long_once();      // << blink once long on success
    }
    delay(300);
  }

  Serial.println("Done. (Loop will idle.)");
}

void loop() {
  // Idle – nothing fancy here.
  delay(1000);
}
