#pragma once
#include "../../src/interfaces/IWebSocket.h"
#include <string>

class MockWebSocket : public IWebSocket::IInterface {
public:
  MockWebSocket() : lastSent() {}
  void begin() override {}
  void sendText(const char* txt) override { lastSent = txt ? txt : std::string(); }

  std::string lastSent;
};
