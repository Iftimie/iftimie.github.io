#ifndef CONVERSATION_MANAGER_H
#define CONVERSATION_MANAGER_H

#include <Arduino.h>
#include <vector>

// History: last 5 assistant turns (facts/continuations)
extern std::vector<String> history;
extern String currentSubject;
extern String currentFact;

void initConversationManager();
void startNewSubject();
void onUserInput(const String& input); // "more" or "other"
void clearHistory();
void saveFactToHistory(const String& fact);
void printCurrentFactSerial();

#endif // CONVERSATION_MANAGER_H