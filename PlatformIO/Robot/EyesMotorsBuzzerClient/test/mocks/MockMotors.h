#pragma once
#include "../../src/interfaces/IMotors.h"
#include <string>

class MockMotors : public IMotors::IInterface {
public:
  MockMotors() : lastCmd() {}
  void begin() override {}
  void sendCommand(const char* cmd) override { lastCmd = cmd ? cmd : std::string(); }

  std::string lastCmd;
};
