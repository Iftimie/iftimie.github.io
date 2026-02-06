#pragma once

// Lightweight eyes abstraction for unit testing and hardware adapters
namespace IEyes {
  enum Emotion : int {
    NEUTRAL = 0,
    BLINK,
    WINK,
    LOOK_L,
    LOOK_R,
    LOOK_U,
    LOOK_D,
    ANGRY,
    SAD,
    EVIL,
    EVIL2,
    SQUINT,
    DEAD,
    SCAN_UD,
    SCAN_LR,
  };

  struct IInterface {
    virtual ~IInterface() {}
    virtual void begin() = 0;
    virtual void setAnimation(Emotion e, bool reset) = 0;
    virtual void setText(const char* txt) = 0;
    // runAnimation should be called frequently; returns true when the current frame/animation finished
    virtual bool runAnimation() = 0;
    virtual void clear() = 0;
    virtual void setIntensity(int intensity) = 0;
  };

  // Global accessors (can be used in tests to inject mocks)
  IInterface* getGlobal();
  void setGlobal(IInterface* impl);
}
