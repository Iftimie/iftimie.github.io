#pragma once

namespace IMotors {
  struct IInterface {
    virtual ~IInterface() {}
    virtual void begin() = 0;
    virtual void sendCommand(const char* cmd) = 0; // e.g. "move:F,100"
  };

  IInterface* getGlobal();
  void setGlobal(IInterface* impl);
}
