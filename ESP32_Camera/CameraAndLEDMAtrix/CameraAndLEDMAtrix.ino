#include "esp_camera.h"
#include <WiFi.h>


// ===========================
// Select camera model in board_config.h
// ===========================
#include "board_config.h"

// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssid = "DIGI-Gua4";
const char* password = "REEeD5sMad";


#define EYES true

#if EYES
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 2
#define DATA_PIN  13
#define CLK_PIN   14
#define CS_PIN    15
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "MD_RobotEyes.h"
MD_MAX72XX M = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

MD_RobotEyes E;


typedef struct
{
  MD_RobotEyes::emotion_t e;
  uint16_t timePause;  // in milliseconds
} sampleItem_t;

const sampleItem_t eSeq[] =
{
  { MD_RobotEyes::E_BLINK, 3000 },
};
#endif

void startCameraServer();
void setupLedFlash();

#define MOTOR_TX_PIN  12      // pick your TX GPIO
#define MOTOR_BAUD    9600

void setup() {


  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  Serial2.begin(MOTOR_BAUD, SERIAL_8N1, -1, MOTOR_TX_PIN); // TX only

  
  Serial.println("Before config");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
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
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_GRAYSCALE;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif
  Serial.println("Before camera init");
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

#if EYES
    
    Serial.println("Before SPI.begin");
    SPI.begin(CLK_PIN, -1, DATA_PIN, CS_PIN);
    Serial.println("Before M.begin");
    M.begin();
    Serial.println("Before E.begin");
    E.begin(&M);
    Serial.println("Before E.setText");
    E.setText("Hello");
    Serial.println("Before do while");
    while (!E.runAnimation()) {
      delay(1);   // yield to WiFi/camera tasks
    }
    Serial.println("After do while");
#endif
}

void loop() {
  // Do nothing. Everything is done in another task by the web server
  // was a delay here


#if EYES
  {
    static uint32_t timeStartDelay;
    static uint8_t index = ARRAY_SIZE(eSeq);
    static enum { S_IDLE, S_TEXT, S_ANIM, S_PAUSE } state = S_IDLE;

    bool b = E.runAnimation();    // always run the animation

    switch (state)
    {
    case S_IDLE:
      index++;
      if (index >= ARRAY_SIZE(eSeq)) 
        index = 0;
      state = S_TEXT;
      break;

    case S_TEXT: // wait for the text to finish
      if (b)  // text animation is finished
      {
        E.setAnimation(eSeq[index].e, true);
        state = S_ANIM;
      }
      break;

    case S_ANIM:  // checking animation is completed
      if (b)  // animation is finished
      {
        timeStartDelay = millis();
        state = S_PAUSE;
      }
      break;
    
    case S_PAUSE: // non blocking waiting for a period between animations
      if (millis() - timeStartDelay >= eSeq[index].timePause)
        state = S_IDLE;
      break;

    default:
      state = S_IDLE;
      break;
    }
  }
#endif
  delay(1);
}
