#include "MD_RobotEyesAdapter.h"
#include <SPI.h>
#include <MD_MAX72xx.h>
#include "MD_RobotEyes.h"

// MAX7219 / MD_MAX72XX pins (ESP32 GPIO numbers)
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 2
static constexpr uint8_t DATA_PIN = 13;
static constexpr uint8_t CLK_PIN  = 14;
static constexpr uint8_t CS_PIN   = 15;

struct MD_RobotEyesAdapter::Impl {
  MD_MAX72XX M{HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES};
  MD_RobotEyes E;
};

// Adapter does not define global accessors; `Globals.cpp` provides them.

MD_RobotEyesAdapter::MD_RobotEyesAdapter() { p = new Impl(); }
void MD_RobotEyesAdapter::begin() {
  p->M.begin();
  p->E.begin(&p->M);
}

static MD_RobotEyes::emotion_t mapEmotion(IEyes::Emotion e) {
  using E = IEyes::Emotion;
  using R = MD_RobotEyes;
  switch (e) {
    case E::NEUTRAL: return R::E_NEUTRAL;
    case E::BLINK: return R::E_BLINK;
    case E::WINK: return R::E_WINK;
    case E::LOOK_L: return R::E_LOOK_L;
    case E::LOOK_R: return R::E_LOOK_R;
    case E::LOOK_U: return R::E_LOOK_U;
    case E::LOOK_D: return R::E_LOOK_D;
    case E::ANGRY: return R::E_ANGRY;
    case E::SAD: return R::E_SAD;
    case E::EVIL: return R::E_EVIL;
    case E::EVIL2: return R::E_EVIL2;
    case E::SQUINT: return R::E_SQUINT;
    case E::DEAD: return R::E_DEAD;
    case E::SCAN_UD: return R::E_SCAN_UD;
    case E::SCAN_LR: return R::E_SCAN_LR;
    default: return R::E_NEUTRAL;
  }
}

void MD_RobotEyesAdapter::setAnimation(IEyes::Emotion e, bool reset) {
  p->M.control(MD_MAX72XX::INTENSITY, 1);
  p->E.setAnimation(mapEmotion(e), reset);
}

void MD_RobotEyesAdapter::setText(const char* txt) { p->E.setText(txt); }
bool MD_RobotEyesAdapter::runAnimation() { return p->E.runAnimation(); }
void MD_RobotEyesAdapter::clear() { p->M.clear(); p->E.setAnimation(MD_RobotEyes::E_NEUTRAL, true); }
void MD_RobotEyesAdapter::setIntensity(int intensity) { p->M.control(MD_MAX72XX::INTENSITY, intensity); }
