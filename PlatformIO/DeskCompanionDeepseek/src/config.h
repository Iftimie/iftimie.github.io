#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
extern const char* ssid;
extern const char* password;

// DeepSeek API Configuration
extern const char* DEEPSEEK_API_KEY;
extern const char* DEEPSEEK_ENDPOINT;
extern const char* DEEPSEEK_MODEL;

// Hardware Pin Configuration
#define OLED_SDA_PIN 3
#define OLED_SCL_PIN 1
#define TOUCH_SENSOR_PIN 4

#include "safe_serial.h"
#define Serial safeSerial

// Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// Bluetooth Configuration
extern const char* BT_DEVICE_NAME;

// Audio Configuration
#define AUDIO_FILE_PATH "/out2.raw"

// Conversation Configuration
#define MAX_TOKENS 100
#define RESPONSE_TEMPERATURE 0.9
#define TOUCH_DEBOUNCE_TIME 200 // milliseconds

// Enable this to mock DeepSeek HTTP responses locally (1 = mock, 0 = real)
#define MOCK_DEEPSEEK 1

// Disable all Serial output at compile time (set to 1 to strip prints)
#define DISABLE_SERIAL_OUTPUT 1

#endif // CONFIG_H