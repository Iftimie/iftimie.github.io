#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "img_converters.h"
// Camera model pins etc.
#include "board_config.h"

// WiFi
const char* ssid     = "DIGI-Gua4";
const char* password = "REEeD5sMad";

// Server placeholder (we'll implement later)
const char* uploadUrl = "http://128.140.71.111/upload"; // TODO
const char* uploadToken = "change-me";   // must match UPLOAD_TOKEN in server.py

static uint32_t lastOkMs = 0;

// Optional tuning
static const int  WIFI_TIMEOUT_MS = 20000;
static const bool USE_HTTPS_INSECURE_FOR_NOW = true; // TODO: remove later

static const int W = 640;
static const int H = 480;


bool sendFrameAsJpegFromGray(uint8_t* gray, size_t len) {
  Serial.println("[send] enter");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[send] WiFi NOT connected");
    return false;
  }

  Serial.printf("[send] raw ptr=%p len=%u (expected %u)\n",
                gray, (unsigned)len, (unsigned)(W * H));

  if (!gray) {
    Serial.println("[send] gray == NULL");
    return false;
  }

  if (len != (size_t)(W * H)) {
    Serial.println("[send] raw size mismatch");
    return false;
  }

  Serial.printf("[send] free heap before jpeg: %u\n", ESP.getFreeHeap());

  uint8_t* jpg_buf = nullptr;
  size_t   jpg_len = 0;

  const int quality = 30;

  Serial.println("[send] calling fmt2jpg()");

  bool okConv = fmt2jpg(
      gray, len,
      W, H,
      PIXFORMAT_GRAYSCALE,
      quality,
      &jpg_buf, &jpg_len
  );

  Serial.printf("[send] fmt2jpg ok=%d jpg_buf=%p jpg_len=%u\n",
                okConv, jpg_buf, (unsigned)jpg_len);

  if (!okConv || !jpg_buf || jpg_len == 0) {
    Serial.println("[send] JPEG conversion FAILED");
    if (jpg_buf) free(jpg_buf);
    return false;
  }

  Serial.printf("[send] free heap after jpeg: %u\n", ESP.getFreeHeap());

  HTTPClient http;
  http.setTimeout(5000);

  Serial.println("[send] http.begin()");

  if (!http.begin(uploadUrl)) {
    Serial.println("[send] http.begin FAILED");
    free(jpg_buf);
    http.end();
    return false;
  }

  Serial.println("[send] adding headers");

  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("X-Token", uploadToken);
  http.addHeader("Connection", "close");

  Serial.printf("[send] POSTing %u bytes\n", (unsigned)jpg_len);

  int code = http.POST(jpg_buf, jpg_len);

  Serial.printf("[send] HTTP response code: %d\n", code);

  http.end();
  free(jpg_buf);

  Serial.printf("[send] free heap after POST: %u\n", ESP.getFreeHeap());

  bool ok = (code >= 200 && code < 300);
  Serial.printf("[send] DONE ok=%d\n", ok);

  return ok;
}



void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    if (millis() - start > WIFI_TIMEOUT_MS) {
      // If WiFi fails, restart (simple + robust for embedded)
      ESP.restart();
    }
  }
}

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk  = XCLK_GPIO_NUM;
  config.pin_pclk  = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href  = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;

  // For pushing frames, use JPEG directly
  config.pixel_format = PIXFORMAT_GRAYSCALE;

  // Pick a sane default; you can raise later.
  config.frame_size   = FRAMESIZE_VGA;   // 640x480
  config.jpeg_quality = 12;              // 0(best)-63(worst), typical 10-15
  config.fb_count     = psramFound() ? 2 : 1;

  config.grab_mode    = psramFound() ? CAMERA_GRAB_LATEST : CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location  = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    // Fail hard; camera misconfig is not recoverable at runtime usually
    ESP.restart();
  }

  // Optional sensor tweaks
  sensor_t* s = esp_camera_sensor_get();
  Serial.printf("PID: 0x%04x\n", s->id.PID);
  s->set_framesize(s, config.frame_size);
  // s->set_vflip(s, 1);
  // s->set_hmirror(s, 1);
  // s->set_brightness(s, 1);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  initCamera();
  connectWiFi();

  Serial.print("WiFi OK, IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { delay(10); return; }

  // fb->buf is raw grayscale bytes
  bool ok = sendFrameAsJpegFromGray(fb->buf, fb->len);

  esp_camera_fb_return(fb);

  if (!ok) {
    Serial.printf("Failed sending");
    delay(200);
    }
    Serial.printf("Sent");
  delay(1);
}

