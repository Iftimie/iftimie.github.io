#pragma once
#include "IEyes.h"

// Adapter that wraps MD_RobotEyes + MD_MAX72XX and exposes IEyes::IInterface
class MD_RobotEyesAdapter : public IEyes::IInterface {
public:
  MD_RobotEyesAdapter();
  void begin() override;
  void setAnimation(IEyes::Emotion e, bool reset) override;
  void setText(const char* txt) override;
  bool runAnimation() override;
  void clear() override;
  void setIntensity(int intensity) override;
private:
  struct Impl;
  Impl* p;
};
