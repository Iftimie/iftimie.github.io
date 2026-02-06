#ifndef DEEPSEEK_CLIENT_H
#define DEEPSEEK_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Conversation structure
struct ConversationState {
    bool waitingForResponse;
    bool needsMoreDetails;
    bool canContinue;
    String currentSubject;
    String lastFact;
};

extern ConversationState convState;

// Function declarations
bool connectToWiFi();
String sendToDeepSeek(const String& userMessage);
String parseDeepSeekResponse(const String& jsonResponse);
bool extractFactFromResponse(const String& response, String& factOut);
void updateConversationState(const String& response);
bool isWaitingForUserInput();
void resetConversation();

#endif // DEEPSEEK_CLIENT_H