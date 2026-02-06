#include "CommandUtils.h"
#include <string.h>
#include <ctype.h>

// Prefer ArduinoJson when available (PlatformIO build). For host test harnesses
// that don't have ArduinoJson installed, fall back to a tiny parser that
// handles the limited array-of-array-of-strings format used by our tests.
#if __has_include(<ArduinoJson.h>)
#include <ArduinoJson.h>
#define HAVE_ARDUINOJSON 1
#else
#define HAVE_ARDUINOJSON 0
#endif

static const struct { const char* name; int freq; } NOTE_TABLE[] = {
  {"C3", 131}, {"CS3", 139}, {"D3", 147}, {"DS3", 156}, {"E3", 165},
  {"F3", 175}, {"FS3", 185}, {"G3", 196}, {"GS3", 208}, {"A3", 220}, {"AS3", 233}, {"B3", 247},
  {"C4", 262}, {"CS4", 277}, {"D4", 294}, {"DS4", 311}, {"E4", 330},
  {"F4", 349}, {"FS4", 370}, {"G4", 392}, {"GS4", 415}, {"A4", 440}, {"AS4", 466}, {"B4", 494},
  {"C5", 523}, {"CS5", 554}, {"D5", 587}, {"DS5", 622}, {"E5", 659},
  {"F5", 698}, {"FS5", 740}, {"G5", 784}, {"GS5", 831}, {"A5", 880}, {"AS5", 932}, {"B5", 988},
  {"REST", 0},
};

int noteToFreq(const char* note) {
  if (!note) return 0;
  char up[16];
  size_t n = strlen(note);
  if (n >= sizeof(up)) n = sizeof(up) - 1;
  for (size_t i = 0; i < n; ++i) up[i] = toupper((unsigned char)note[i]);
  up[n] = '\0';

  for (auto &e : NOTE_TABLE) if (strcmp(up, e.name) == 0) return e.freq;
  return atoi(note);
}

bool parsePair(const char* tok, int& freq, int& ms) {
  if (!tok) return false;
  const char* comma = strchr(tok, ',');
  if (!comma) return false;
  char note[16] = {0};
  size_t nlen = (size_t)(comma - tok);
  if (nlen >= sizeof(note)) nlen = sizeof(note) - 1;
  memcpy(note, tok, nlen);
  note[nlen] = '\0';
  // trim
  const char* noteTrim = note;
  while (*noteTrim == ' ') ++noteTrim;
  freq = noteToFreq(noteTrim);
  ms = atoi(comma + 1);
  if (ms < 0) ms = 0;
  return true;
}

std::pair<std::string,int> parseMotorPayload(const char* payload) {
  std::pair<std::string,int> r{"",0};
  if (!payload) return r;
  std::string s(payload);
  // trim spaces
  while (!s.empty() && isspace((unsigned char)s.front())) s.erase(s.begin());
  while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
  size_t pos = s.find_last_of(',');
  if (pos != std::string::npos) {
    std::string tail = s.substr(pos + 1);
    bool allDigits = !tail.empty();
    for (char c : tail) if (!isdigit((unsigned char)c)) { allDigits = false; break; }
    if (allDigits) {
      r.second = atoi(tail.c_str());
      r.first = s.substr(0, pos);
    } else {
      r.first = s;
    }
  } else {
    r.first = s;
  }
  // trim result
  while (!r.first.empty() && isspace((unsigned char)r.first.back())) r.first.pop_back();
  while (!r.first.empty() && isspace((unsigned char)r.first.front())) r.first.erase(r.first.begin());
  return r;
}

std::vector<std::vector<std::string>> parseGroupedJson(const std::string& json) {
  std::vector<std::vector<std::string>> out;
  if (json.empty()) return out;
#if HAVE_ARDUINOJSON
  DynamicJsonDocument doc(1024);
  auto err = deserializeJson(doc, json);
  if (err) return out;
  if (!doc.is<JsonArray>()) return out;
  for (JsonVariant g : doc.as<JsonArray>()) {
    if (!g.is<JsonArray>()) continue;
    std::vector<std::string> group;
    for (JsonVariant item : g.as<JsonArray>()) {
      if (item.is<const char*>()) group.emplace_back(std::string(item.as<const char*>()));
      else if (item.is<std::string>()) group.emplace_back(item.as<std::string>());
    }
    out.emplace_back(std::move(group));
  }
  return out;
#else
  // Very small, lenient parser for JSON arrays of string arrays like:
  // [["eyes:angry","audio:C4,200"],["move:F,300"]]
  enum State { S_START, S_IN_OUTER, S_IN_GROUP, S_IN_STR, S_ESC };
  State st = S_START;
  std::vector<std::string> curGroup;
  std::string curStr;
  for (size_t i = 0; i < json.size(); ++i) {
    char c = json[i];
    switch (st) {
      case S_START:
        if (c == '[') st = S_IN_OUTER;
        break;
      case S_IN_OUTER:
        if (c == '[') { curGroup.clear(); st = S_IN_GROUP; }
        else if (c == ']') return out;
        break;
      case S_IN_GROUP:
        if (c == '"') { curStr.clear(); st = S_IN_STR; }
        else if (c == ']') { out.emplace_back(curGroup); st = S_IN_OUTER; }
        break;
      case S_IN_STR:
        if (c == '\\') { st = S_ESC; }
        else if (c == '"') { curGroup.emplace_back(curStr); st = S_IN_GROUP; }
        else curStr.push_back(c);
        break;
      case S_ESC:
        curStr.push_back(c);
        st = S_IN_STR;
        break;
    }
  }
  // If ended while inside a group, commit it
  if (!curGroup.empty()) out.emplace_back(curGroup);
  return out;
#endif
}
