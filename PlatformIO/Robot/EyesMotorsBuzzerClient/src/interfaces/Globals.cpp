#include "IEyes.h"
#include "IAudio.h"
#include "IMotors.h"
#include "IWebSocket.h"

static IEyes::IInterface* g_eyes = nullptr;
IEyes::IInterface* IEyes::getGlobal() { return g_eyes; }
void IEyes::setGlobal(IInterface* impl) { g_eyes = impl; }

static IAudio::IInterface* g_audio = nullptr;
IAudio::IInterface* IAudio::getGlobal() { return g_audio; }
void IAudio::setGlobal(IAudio::IInterface* impl) { g_audio = impl; }

static IMotors::IInterface* g_motors = nullptr;
IMotors::IInterface* IMotors::getGlobal() { return g_motors; }
void IMotors::setGlobal(IMotors::IInterface* impl) { g_motors = impl; }

static IWebSocket::IInterface* g_ws = nullptr;
IWebSocket::IInterface* IWebSocket::getGlobal() { return g_ws; }
void IWebSocket::setGlobal(IWebSocket::IInterface* impl) { g_ws = impl; }
