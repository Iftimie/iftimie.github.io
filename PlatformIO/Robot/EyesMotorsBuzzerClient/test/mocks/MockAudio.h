#pragma once
#include "../../src/interfaces/IAudio.h"
#include <string>

class MockAudio : public IAudio::IInterface {
public:
  MockAudio() : lastSeq(), stopped(false), finished(false) {}
  void begin() override {}
  void playSequence(const char* seq) override { lastSeq = seq ? seq : std::string(); stopped = false; finished = false; }
  void stop() override { stopped = true; }
  bool run() override { return finished; }

  // helpers
  void finishOnce() { finished = true; }
  void reset() { finished = false; stopped = false; lastSeq.clear(); }

  std::string lastSeq;
  bool stopped;
  bool finished;
};
