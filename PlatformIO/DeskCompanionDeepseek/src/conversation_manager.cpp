#include "conversation_manager.h"
#include <Arduino.h>
#include <vector>
#include "deepseek_client.h"
#include "subjects.h"
#include "oled_display.h"
#include "touch_input.h"

// Forward declaration to ensure the enqueue function is visible to this file
bool enqueueAudioNotification();

std::vector<String> history;
String currentSubject = "";
String currentFact = "";
static const size_t HISTORY_MAX = 5;

void initConversationManager() {
  history.clear();
  // Seed PRNG to make prompt nonces less predictable
  randomSeed((unsigned long)micros());
  Serial.println("ConvMgr init");
}

void saveFactToHistory(const String& fact) {
  history.push_back(fact);
  while (history.size() > HISTORY_MAX) history.erase(history.begin());
}

void printCurrentFactSerial() {
  if (!currentFact.isEmpty()) Serial.println(currentFact);
}

void startNewSubject() {
  // Discard history so the model picks a fresh subject
  history.clear();
  currentSubject = ""; currentFact = "";
  Serial.println("Requesting new subject/fact");
  // Pick a local random subject and add a seed to break server-side caching.
  String subj = pickRandomSubject();
  String seed = String(millis()) + "_" + String(random(0, 1000000));
  String prompt = "Give one short intriguing sentence (8-12 words) about: " + subj + ". Respond with a single sentence only. Seed:" + seed;
  String resp = sendToDeepSeek(prompt);
  String fact;
  if (extractFactFromResponse(resp, fact)) {
    currentFact = fact;
    currentSubject = ""; // subject not extracted from plain-text response
    printCurrentFactSerial();
    // Display on OLED
    displayWrappedText(currentFact);
    // Signal audio notification
    enqueueAudioNotification();
    saveFactToHistory(fact);
  } else {
    Serial.println("Failed to get a fact from response");
  }
}

void onUserInput(const String& input) {
  if (input == "more") {
    // Build a context containing up to the last 5 facts so the model can continue coherently
    String context = "Previous facts:\n";
    for (size_t i = 0; i < history.size(); ++i) {
      context += "- ";
      context += history[i];
      context += "\n";
    }
    // Ask the model explicitly not to repeat recent facts and add a seed
    String avoid = "Avoid repeating these recent facts. ";
    String seed = String(millis()) + "_" + String(random(0, 1000000));
    String prompt = avoid + "\n" + context + "Continue about the current subject; provide one short sentence (8-12 words). Seed:" + seed;
    String resp = sendToDeepSeek(prompt);
    String fact2;
    if (extractFactFromResponse(resp, fact2)) {
      currentFact = fact2;
      printCurrentFactSerial();
      // Show the continued fact on the OLED as well
      displayWrappedText(currentFact);
      // Signal audio notification
      enqueueAudioNotification();
      saveFactToHistory(fact2);
    } else {
      Serial.println("No continuation");
    }
  } else if (input == "other") {
    startNewSubject();
  }
}

void clearHistory() { history.clear(); }
