#include "deepseek_client.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "esp_heap_caps.h"
// Bluetooth/SD enabled: include bluetooth_audio so we can close/unmount before TLS
#include "bluetooth_audio.h"
// For local subject picks when mocking
#include "subjects.h"

// Forward declarations (ensure availability even if header inclusion order changes)
void suspendBluetoothManager();
void resumeBluetoothManager();
void suspendAudioTask();
void resumeAudioTask();
void unmountSD();
void stopAudioTaskNow();
void stopBluetoothManager();
void startAudioTask();
void startBluetoothManager();
void shutdownBluetoothController();
bool restartBluetoothController();

// Attempt to defragment internal heap: try allocating a large internal block
// (decreasing sizes) and free it to encourage coalescing. This is heuristic.
static void attempt_defrag_internal_heap() {
    const size_t tries[] = {65536, 32768, 16384, 8192, 4096};
    for (size_t i = 0; i < sizeof(tries)/sizeof(tries[0]); ++i) {
        size_t s = tries[i];
        void* p = heap_caps_malloc(s, MALLOC_CAP_INTERNAL);
        if (p) {
            Serial.print("Defrag: allocated and freeing "); Serial.println(String(s));
            heap_caps_free(p);
            // small pause to let allocator coalesce
            delay(50);
            break;
        }
    }
}

ConversationState convState = {false, false, false, "", ""};

bool connectToWiFi() {
    Serial.println("Connecting to WiFi...");
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("\nFailed to connect to WiFi");
        return false;
    }
}

String sendToDeepSeek(const String& userMessage) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return "WiFi error";
    }
    // If mock mode is enabled, return a quick canned/plain-text response
    // so the rest of the flow (parsing, audio enqueue) behaves the same.
#if MOCK_DEEPSEEK
    Serial.println("MOCK: returning canned DeepSeek response");
    String subj = pickRandomSubject();
    String mockFact = "Mock: " + subj + " are surprisingly interesting.";
    return mockFact;
#endif
    
    // Diagnostic: print free heap before starting TLS handshake
    Serial.print("Free heap before TLS: "); Serial.println(String(ESP.getFreeHeap()));

    // Reduce memory pressure: gently suspend audio and unmount SD. We avoid
    // stopping the BT manager entirely — keep it running to preserve the
    // Bluetooth stack state and avoid restart fragility.
    bool audio_was_deleted = false;

    suspendAudioTask();
    // Do not suspend or stop the BT manager here; leave it running.
    unmountSD();

    // Give scheduler a moment to settle and inspect heap.
    delay(150);
    Serial.print("Free heap after suspend/unmount: "); Serial.println(String(ESP.getFreeHeap()));

    // If heap still low, delete the audio task to free its stack (fallback).
    const size_t HEAP_SAFE_THRESHOLD = 110000; // heuristic; adjust if needed
    if (ESP.getFreeHeap() < HEAP_SAFE_THRESHOLD) {
        Serial.println("Heap low after suspend; deleting audio task to free stacks");
        stopAudioTaskNow();
        audio_was_deleted = true;
        // allow scheduler/driver cleanup
        delay(100);
        Serial.print("Free heap after delete: "); Serial.println(String(ESP.getFreeHeap()));
    }

    // Give the scheduler and drivers a moment to reclaim deleted stacks
    // and freed buffers, then attempt a light defrag of internal heap.
    delay(250);
    Serial.print("Free heap after stopping/unmount/shutdown: "); Serial.println(String(ESP.getFreeHeap()));
    attempt_defrag_internal_heap();
    Serial.print("Free heap after defrag attempt: "); Serial.println(String(ESP.getFreeHeap()));

    // Give the system a short moment to finish closing/unmounting and free
    // internal heap resources, then print heap status for debugging.
    delay(200);
    Serial.print("Free heap after unmount: "); Serial.println(String(ESP.getFreeHeap()));

    // Quiet A2DP activity to reduce driver/internal allocations during TLS
    pauseBluetoothForTLS();

    HTTPClient http;
    http.setTimeout(10000); // 10 second timeout

    // Use an explicit WiFiClientSecure so we can tune TLS buffer sizes and
    // reduce mbedTLS peak allocations. Smaller buffers reduce peak RAM.
    WiFiClientSecure client;
    // Some framework versions don't expose buffer-size tuning APIs; keep the
    client.setInsecure();
    http.begin(client, DEEPSEEK_ENDPOINT);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(DEEPSEEK_API_KEY));
    
        // Build request JSON in PSRAM-backed StaticJsonDocument to avoid internal-heap
        // Keep document small to reduce allocations.
        const size_t REQ_DOC_CAP = 768;
        void* reqPool = heap_caps_malloc(sizeof(StaticJsonDocument<REQ_DOC_CAP>), MALLOC_CAP_SPIRAM);
        if (!reqPool) {
            Serial.println("Failed to alloc PSRAM for request doc");
            return String("API Error");
        }
        StaticJsonDocument<REQ_DOC_CAP>* reqDoc = new (reqPool) StaticJsonDocument<REQ_DOC_CAP>();
        (*reqDoc)["model"] = DEEPSEEK_MODEL;
        (*reqDoc)["max_tokens"] = MAX_TOKENS;
        (*reqDoc)["temperature"] = RESPONSE_TEMPERATURE;
        JsonArray messages = reqDoc->createNestedArray("messages");
        JsonObject systemMsg = messages.createNestedObject();
        systemMsg["role"] = "system";
        systemMsg["content"] = "You are DeskMate, a friendly desk companion. You speak in short, clear sentences suitable for a speaker or an OLED 128x64 screen. You randomly pick subjects and give short facts. Keep sentences short (8-12 words each). Use Yes/No questions.";
        JsonObject userMsg = messages.createNestedObject();
        userMsg["role"] = "user";
        userMsg["content"] = userMessage;

        // Use a modest payload buffer; request is small so 1536 bytes should be enough.
        const size_t payloadCap = 1536;
        char* payload = (char*)heap_caps_malloc(payloadCap, MALLOC_CAP_SPIRAM);
        if (!payload) {
            Serial.println("Failed to alloc PSRAM for payload");
            reqDoc->~StaticJsonDocument();
            heap_caps_free(reqPool);
            return String("API Error");
        }
        serializeJson(*reqDoc, payload, payloadCap);
        // free request doc pool
        reqDoc->~StaticJsonDocument();
        heap_caps_free(reqPool);

        Serial.println("Sending to DeepSeek: " + userMessage);
        Serial.print("Full payload: "); Serial.println(payload);

        int httpResponseCode = http.POST((uint8_t*)payload, strlen(payload));

        // If the POST failed (likely TLS allocation), try one quick retry.
        if (httpResponseCode <= 0) {
            Serial.println("Initial HTTP POST failed; retrying once");
            http.end();
            WiFiClientSecure client2;
            http.begin(client2, DEEPSEEK_ENDPOINT);
            httpResponseCode = http.POST((uint8_t*)payload, strlen(payload));
        }

        String response = "";
        if (httpResponseCode > 0) {
                // Read HTTP response into PSRAM buffer (keep moderate size)
                size_t respCap = 4096;
                char* respBuf = (char*)heap_caps_malloc(respCap, MALLOC_CAP_SPIRAM);
                if (!respBuf) {
                    Serial.println("Failed to alloc PSRAM for response buffer");
                    // fallback to getString()
                    response = http.getString();
                    Serial.print("HTTP Response code: "); Serial.println(httpResponseCode);
                    Serial.print("Response: "); Serial.println(response);
                } else {
                    WiFiClient* stream = http.getStreamPtr();
                    size_t idx = 0;
                    int available = 0;
                    // read until stream ends or buffer full
                    while ((available = stream->available()) > 0 && idx < respCap - 1) {
                        int toRead = available;
                        if ((size_t)toRead > respCap - 1 - idx) toRead = respCap - 1 - idx;
                        int r = stream->read((uint8_t*)(respBuf + idx), toRead);
                        if (r <= 0) break;
                        idx += r;
                    }
                                        respBuf[idx] = '\0';
                                        // If stream read produced nothing, fallback to getString()
                                        if (idx == 0) {
                                            response = http.getString();
                                            size_t rlen = response.length();
                                            if (rlen > 0) {
                                                // copy into respBuf (truncate if needed)
                                                if (rlen >= respCap) rlen = respCap - 1;
                                                memcpy(respBuf, response.c_str(), rlen);
                                                respBuf[rlen] = '\0';
                                                idx = rlen;
                                            }
                                        }
                                        Serial.print("HTTP Response code: "); Serial.println(httpResponseCode);
                                        Serial.print("Response: "); Serial.println(respBuf);

                    // Parse JSON response in PSRAM via StaticJsonDocument placement-new
                    const size_t RESP_DOC_CAP = 2048;
                    void* respPool = heap_caps_malloc(sizeof(StaticJsonDocument<RESP_DOC_CAP>), MALLOC_CAP_SPIRAM);
                    if (!respPool) {
                        Serial.println("Failed to alloc PSRAM for response doc");
                        response = String(respBuf);
                    } else {
                        StaticJsonDocument<RESP_DOC_CAP>* respDoc = new (respPool) StaticJsonDocument<RESP_DOC_CAP>();
                        DeserializationError err = deserializeJson(*respDoc, respBuf);
                        if (err) {
                            Serial.print("JSON parsing failed: "); Serial.println(err.c_str());
                            response = String(respBuf);
                        } else {
                            if (respDoc->containsKey("choices") && (*respDoc)["choices"].size() > 0) {
                                response = (*respDoc)["choices"][0]["message"]["content"].as<String>();
                            } else {
                                response = String(respBuf);
                            }
                        }
                        // cleanup
                        respDoc->~StaticJsonDocument();
                        heap_caps_free(respPool);
                    }
                        heap_caps_free(respBuf);
                }
        } else {
                Serial.print("Error on HTTP request: "); Serial.println(httpResponseCode);
                response = "API Error";
        }

            http.end();
            heap_caps_free(payload);

            // Restore A2DP callbacks so the stack can operate normally again.
            resumeBluetoothAfterTLS();

            // Restore audio task depending on how it was stopped. Leave BT
            // manager running as we did not stop it.
            if (audio_was_deleted) {
                startAudioTask();
            } else {
                resumeAudioTask();
            }

            // Give audio init time
            delay(200);

            Serial.println("Restored audio task after request (BT manager left running)");

    // Resume audio/BT tasks now that TLS/HTTP work is done. Give tasks a
    // moment to restart their init paths (they will lazily remount SD).
    resumeAudioTask();

    // Diagnostic: print free heap after TLS and HTTP complete
    Serial.print("Free heap after TLS: "); Serial.println(String(ESP.getFreeHeap()));

    // `response` already contains the assistant's content (plain text) when parsed
    // above; return it directly to callers. Avoid re-parsing plain text as JSON.
    return response;
}

String parseDeepSeekResponse(const String& jsonResponse) {
    if (jsonResponse == "API Error" || jsonResponse == "WiFi error") {
        return jsonResponse;
    }
    // Try parsing into PSRAM-allocated StaticJsonDocument to avoid internal heap use
    const size_t RESP_DOC_CAP = 2048;
    void* respPool = heap_caps_malloc(sizeof(StaticJsonDocument<RESP_DOC_CAP>), MALLOC_CAP_SPIRAM);
    if (respPool) {
        StaticJsonDocument<RESP_DOC_CAP>* respDoc = new (respPool) StaticJsonDocument<RESP_DOC_CAP>();
        DeserializationError error = deserializeJson(*respDoc, jsonResponse);
        if (error) {
            Serial.print("JSON parsing failed: "); Serial.println(error.c_str());
            respDoc->~StaticJsonDocument();
            heap_caps_free(respPool);
            return "Parse error";
        }
        if (respDoc->containsKey("choices") && (*respDoc)["choices"].size() > 0) {
            String content = (*respDoc)["choices"][0]["message"]["content"].as<String>();
            respDoc->~StaticJsonDocument();
            heap_caps_free(respPool);
            return content;
        }
        respDoc->~StaticJsonDocument();
        heap_caps_free(respPool);
        return "No response";
    }

    // Fallback to DynamicJsonDocument on internal heap
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonResponse);
    if (error) {
        Serial.print("JSON parsing failed: "); Serial.println(error.c_str());
        return "Parse error";
    }
    if (doc.containsKey("choices") && doc["choices"].size() > 0) {
        String content = doc["choices"][0]["message"]["content"].as<String>();
        return content;
    }
    return "No response";
}

bool extractFactFromResponse(const String& response, String& factOut) {
    // Treat the provided response as plain text and return the whole
    // response as the fact.
    factOut = "";
    String content = response;
    content.trim();
    if (content.length() == 0) return false;
    factOut = content;
    return true;
}

bool isWaitingForUserInput() { return false; }

void resetConversation() {}
