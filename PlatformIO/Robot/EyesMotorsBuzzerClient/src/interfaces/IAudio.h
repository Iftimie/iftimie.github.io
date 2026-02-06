#pragma once

namespace IAudio {
  struct IInterface {
    virtual ~IInterface() {}
    virtual void begin() = 0;
    virtual void playSequence(const char* seq) = 0;
    virtual void stop() = 0;
    // run should be polled by tasks when needed; returns true if sequence finished
    virtual bool run() = 0;
  };

  IInterface* getGlobal();
  void setGlobal(IInterface* impl);
}
