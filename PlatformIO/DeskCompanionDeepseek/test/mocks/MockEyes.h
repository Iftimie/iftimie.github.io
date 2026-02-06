#pragma once
#include "../../src/interfaces/IEyes.h"
#include <string>

class MockEyes : public IEyes::IInterface {
public:
  MockEyes() : lastEmotion(IEyes::NEUTRAL), resetCalled(false), lastText(), runFinish(false), intensity(0) {}
  void begin() override {}
  void setAnimation(IEyes::Emotion e, bool reset) override { lastEmotion = e; resetCalled = reset; }
  void setText(const char* txt) override { lastText = txt ? txt : std::string(); }
  bool runAnimation() override { return runFinish; }
  void clear() override { lastText.clear(); }
  void setIntensity(int i) override { intensity = i; }

  // test helpers
  void finishOnce() { runFinish = true; }
  void resetFinish() { runFinish = false; }

  IEyes::Emotion lastEmotion;
  bool resetCalled;
  std::string lastText;
  bool runFinish;
  int intensity;
};
