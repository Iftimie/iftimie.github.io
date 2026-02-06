#include "esp_camera.h"
#include "esp_heap_caps.h"

// --- AI THINKER pinout (hardcoded, no board_config.h) ---
#define PWDN_GPIO_NUM     32   // try 32 first, later we'll try -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32-CAM minimal test ===");

  Serial.printf("psramFound(): %s\n", psramFound() ? "YES" : "NO");
  Serial.printf("Free PSRAM: %d bytes\n",
                heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // super small, easy mode:
  config.frame_size   = FRAMESIZE_QQVGA;      // 160x120
  config.jpeg_quality = 12;
  config.fb_count     = 1;
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location  = CAMERA_FB_IN_DRAM;    // ignore PSRAM for now

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera init OK");

  sensor_t *s = esp_camera_sensor_get();
  if (!s) {
    Serial.println("sensor_t is NULL!");
    return;
  }
  Serial.printf("Sensor PID: 0x%04X\n", s->id.PID);

  // Just to be sure:
  s->set_framesize(s, FRAMESIZE_QQVGA);
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("fb NULL: Camera capture failed");
  } else {
    Serial.printf("Got frame: %dx%d, %u bytes\n",
                  fb->width, fb->height, fb->len);
    esp_camera_fb_return(fb);
  }
  delay(1000);
}
