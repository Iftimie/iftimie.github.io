// Small utility functions extracted for parsing and unit testing
#pragma once

#include <string>
#include <vector>
#include <stdint.h>

int noteToFreq(const char* note);
bool parsePair(const char* tok, int& freq, int& ms);

// Parse a motor payload like "F,100" or "F" -> returns pair(payload_without_delay, delayMs)
std::pair<std::string,int> parseMotorPayload(const char* payload);

// Parse grouped JSON: input is JSON array-of-arrays of strings, e.g. [["eyes:angry","audio:C4,200"],["move:F,500"]]
// Returns vector of groups, each group is vector<string>
std::vector<std::vector<std::string>> parseGroupedJson(const std::string& json);
