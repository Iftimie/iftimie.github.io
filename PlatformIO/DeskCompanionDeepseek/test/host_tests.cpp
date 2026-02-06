#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "../src/CommandUtils.h"
#include "../src/interfaces/IEyes.h"
#include "../src/interfaces/IAudio.h"
#include "../src/interfaces/IMotors.h"
#include "../src/interfaces/IWebSocket.h"
#include "mocks/MockEyes.h"
#include "mocks/MockAudio.h"
#include "mocks/MockMotors.h"
#include "mocks/MockWebSocket.h"

int run_command_utils_tests() {
  int failures = 0;
  // noteToFreq
  if (noteToFreq("C4") != 262) { std::cerr << "noteToFreq C4 failed\n"; ++failures; }
  if (noteToFreq("A4") != 440) { std::cerr << "noteToFreq A4 failed\n"; ++failures; }
  if (noteToFreq("REST") != 0) { std::cerr << "noteToFreq REST failed\n"; ++failures; }

  // parsePair
  int f=0, ms=0;
  if (!parsePair("C4,200", f, ms)) { std::cerr << "parsePair failed\n"; ++failures; }
  if (f != 262 || ms != 200) { std::cerr << "parsePair values incorrect\n"; ++failures; }

  if (parsePair("BADFORMAT", f, ms)) { std::cerr << "parsePair should fail on BADFORMAT\n"; ++failures; }

  // parseMotorPayload
  auto r = parseMotorPayload("F,500");
  if (r.first != "F" || r.second != 500) { std::cerr << "parseMotorPayload failed\n"; ++failures; }
  auto r2 = parseMotorPayload("L");
  if (r2.first != "L" || r2.second != 0) { std::cerr << "parseMotorPayload simple failed\n"; ++failures; }

  // parseGroupedJson
  std::string j = "[[\"eyes:angry\",\"audio:C4,200\"],[\"move:F,300\"]]";
  auto groups = parseGroupedJson(j);
  if ((int)groups.size() != 2) { std::cerr << "parseGroupedJson group count\n"; ++failures; }
  if ((int)groups[0].size() != 2) { std::cerr << "parseGroupedJson inner size\n"; ++failures; }

  return failures;
}

int run_mock_injection_tests() {
  int failures = 0;
  // Inject mocks via globals
  MockEyes me;
  MockAudio ma;
  MockMotors mm;
  MockWebSocket mw;
  IEyes::setGlobal(&me);
  IAudio::setGlobal(&ma);
  IMotors::setGlobal(&mm);
  IWebSocket::setGlobal(&mw);

  // exercise mocks
  IEyes::IInterface* eyes = IEyes::getGlobal();
  eyes->setAnimation(IEyes::ANGRY, true);
  if (me.lastEmotion != IEyes::ANGRY) { std::cerr << "MockEyes setAnimation not recorded\n"; ++failures; }

  eyes->setText("HELLO");
  if (me.lastText != "HELLO") { std::cerr << "MockEyes setText not recorded\n"; ++failures; }

  IAudio::IInterface* audio = IAudio::getGlobal();
  audio->playSequence("C4,200");
  if (ma.lastSeq != "C4,200") { std::cerr << "MockAudio playSequence not recorded\n"; ++failures; }

  IMotors::IInterface* motors = IMotors::getGlobal();
  motors->sendCommand("F,100");
  if (mm.lastCmd != "F,100") { std::cerr << "MockMotors sendCommand not recorded\n"; ++failures; }

  IWebSocket::IInterface* ws = IWebSocket::getGlobal();
  ws->sendText("OK");
  if (mw.lastSent != "OK") { std::cerr << "MockWebSocket sendText not recorded\n"; ++failures; }

  return failures;
}

int main() {
  int fails = 0;
  fails += run_command_utils_tests();
  fails += run_mock_injection_tests();
  if (fails == 0) std::cout << "ALL TESTS PASSED\n";
  else std::cout << fails << " TESTS FAILED\n";
  return fails;
}
