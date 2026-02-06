#include "TaskCommon.h"
#include "WebSocketTask.h"

#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <vector>

// WiFi + WebSocket config (kept local to task)
static const char* WIFI_SSID = "DIGI-Gua4";
static const char* WIFI_PASS = "REEeD5sMad";
static const char* WS_HOST = "192.168.1.177";
static const uint16_t WS_PORT = 8765;
static const char* WS_PATH = "/ws";

static WebSocketsClient g_ws;

// queues are defined in main.cpp
extern QueueHandle_t g_cmd_q;
extern QueueHandle_t g_audio_q;
extern QueueHandle_t g_motor_q;
extern QueueHandle_t g_done_q;

static uint32_t s_next_id = 1;

static uint32_t nextId() { return s_next_id++; }


// Helpers for enqueueing commands (returns assigned id or 0)
static uint32_t enqueueAudioCmd(const char* txt, bool assignId) {
  if (!g_audio_q) return 0;
  AudioCmd ac{};
  strncpy(ac.text, txt, sizeof(ac.text) - 1);
  ac.text[sizeof(ac.text) - 1] = '\0';
  ac.id = assignId ? nextId() : 0;
  xQueueSend(g_audio_q, &ac, 0);
  return ac.id;
}

static uint32_t enqueueMotorCmd(const char* txt, bool assignId) {
  if (!g_motor_q) return 0;
  MotorCmd mc{};
  strncpy(mc.text, txt, sizeof(mc.text) - 1);
  mc.text[sizeof(mc.text) - 1] = '\0';
  mc.id = assignId ? nextId() : 0;
  xQueueSend(g_motor_q, &mc, 0);
  return mc.id;
}

static uint32_t enqueueEyesCmd(const char* txt, bool assignId) {
  if (!g_cmd_q) return 0;
  EyesCmd ec{};
  strncpy(ec.text, txt, sizeof(ec.text) - 1);
  ec.text[sizeof(ec.text) - 1] = '\0';
  ec.id = assignId ? nextId() : 0;
  xQueueSend(g_cmd_q, &ec, 0);
  return ec.id;
}

static void processGroupedJson(const char* msg) {
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial.printf("[WS] JSON parse error: %s\n", err.c_str());
    return;
  }

  std::vector<uint32_t> stash; // done IDs received early

  JsonArray outer = doc.as<JsonArray>();
  for (JsonVariant innerVar : outer) {
    if (!innerVar.is<JsonArray>()) continue;
    JsonArray inner = innerVar.as<JsonArray>();

    // send all commands in this inner array in parallel
    std::vector<uint32_t> pending;

    for (JsonVariant cmdVar : inner) {
      if (!cmdVar.is<const char*>()) continue;
      const char* cmdStr = cmdVar.as<const char*>();
      if (!cmdStr) continue;

      if (startsWithNoCase(cmdStr, "audio:")) {
        uint32_t id = enqueueAudioCmd(cmdStr, true);
        if (id) pending.push_back(id);
      } else if (startsWithNoCase(cmdStr, "move:")) {
        uint32_t id = enqueueMotorCmd(cmdStr, true);
        if (id) pending.push_back(id);
      } else {
        uint32_t id = enqueueEyesCmd(cmdStr, true);
        if (id) pending.push_back(id);
      }
    }

    // remove ids already in stash
    for (auto it = pending.begin(); it != pending.end();) {
      bool found = false;
      for (auto sIt = stash.begin(); sIt != stash.end(); ++sIt) {
        if (*sIt == *it) { found = true; stash.erase(sIt); break; }
      }
      if (found) it = pending.erase(it); else ++it;
    }

    // wait for pending ids
    while (!pending.empty()) {
      DoneEvent de{};
      if (xQueueReceive(g_done_q, &de, portMAX_DELAY) == pdTRUE) {
        auto m = std::find(pending.begin(), pending.end(), de.id);
        if (m != pending.end()) {
          pending.erase(m);
          continue;
        }
        stash.push_back(de.id);
      }
    }
  }
}

static void processSingleCommand(const char* msg) {
  if (startsWithNoCase(msg, "audio:")) {
    enqueueAudioCmd(msg, false);
  } else if (startsWithNoCase(msg, "move:")) {
    enqueueMotorCmd(msg, false);
  } else {
    enqueueEyesCmd(msg, false);
  }
}

static void onWsEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("[WS] connected");
      g_ws.sendTXT("robot-online");
      break;

    case WStype_DISCONNECTED:
      Serial.println("[WS] disconnected");
      break;

    case WStype_TEXT: {
      // receive raw message into buffer
      char msg[512];
      size_t n = (length >= sizeof(msg)) ? (sizeof(msg) - 1) : length;
      memcpy(msg, payload, n);
      msg[n] = '\0';

      Serial.printf("[WS] rx: %s\n", msg);

      const char* p = skipSpaces(msg);
      if (*p == '[') {
        processGroupedJson(msg);
      } else {
        processSingleCommand(msg);
      }
      break;
    }

    default:
      break;
  }
}

void taskWebSocket(void* /*arg*/) {
  Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    vTaskDelay(pdMS_TO_TICKS(250));
  }
  Serial.printf("\n[WiFi] OK. IP=%s\n", WiFi.localIP().toString().c_str());

  g_ws.begin(WS_HOST, WS_PORT, WS_PATH);
  g_ws.onEvent(onWsEvent);
  g_ws.setReconnectInterval(2000);
  g_ws.enableHeartbeat(15000, 3000, 2);

  for (;;) {
    g_ws.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
