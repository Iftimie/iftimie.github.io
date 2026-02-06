#pragma once

// Minimal WebSocket abstraction for sending messages (tests may mock)
namespace IWebSocket {
  struct IInterface {
    virtual ~IInterface() {}
    virtual void begin() = 0;
    virtual void sendText(const char* txt) = 0;
  };

  IInterface* getGlobal();
  void setGlobal(IInterface* impl);
}
