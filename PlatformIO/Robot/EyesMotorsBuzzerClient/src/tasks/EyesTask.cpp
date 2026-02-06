#include "TaskCommon.h"
#include "EyesTask.h"

#include "../interfaces/IEyes.h"
#include "../interfaces/MD_RobotEyesAdapter.h"

// parseEmotion moved here (uses MD_RobotEyes::emotion_t)
static bool parseEmotion(const char* s, IEyes::Emotion& out) {
  if (!s) return false;
  s = skipSpaces(s);
  if (*s == '\0') return false;
  if (strcasecmp(s, "neutral") == 0)   { out = IEyes::NEUTRAL; return true; }
  if (strcasecmp(s, "blink") == 0)     { out = IEyes::BLINK;   return true; }
  if (strcasecmp(s, "wink") == 0)      { out = IEyes::WINK;    return true; }

  if (strcasecmp(s, "left") == 0 || strcasecmp(s, "look_l") == 0 || strcasecmp(s, "look:left") == 0)
    { out = IEyes::LOOK_L; return true; }
  if (strcasecmp(s, "right") == 0 || strcasecmp(s, "look_r") == 0 || strcasecmp(s, "look:right") == 0)
    { out = IEyes::LOOK_R; return true; }
  if (strcasecmp(s, "up") == 0 || strcasecmp(s, "look_u") == 0 || strcasecmp(s, "look:up") == 0)
    { out = IEyes::LOOK_U; return true; }
  if (strcasecmp(s, "down") == 0 || strcasecmp(s, "look_d") == 0 || strcasecmp(s, "look:down") == 0)
    { out = IEyes::LOOK_D; return true; }

  if (strcasecmp(s, "angry") == 0)     { out = IEyes::ANGRY;   return true; }
  if (strcasecmp(s, "sad") == 0)       { out = IEyes::SAD;     return true; }
  if (strcasecmp(s, "evil") == 0)      { out = IEyes::EVIL;    return true; }
  if (strcasecmp(s, "evil2") == 0)     { out = IEyes::EVIL2;   return true; }
  if (strcasecmp(s, "squint") == 0)    { out = IEyes::SQUINT;  return true; }
  if (strcasecmp(s, "dead") == 0)      { out = IEyes::DEAD;    return true; }

  if (strcasecmp(s, "scan_ud") == 0 || strcasecmp(s, "scanv") == 0)
    { out = IEyes::SCAN_UD; return true; }
  if (strcasecmp(s, "scan_lr") == 0 || strcasecmp(s, "scanh") == 0)
    { out = IEyes::SCAN_LR; return true; }

  return false;
}

// Eyes sequence parsing (local)
enum EyeStepType : uint8_t { STEP_EMO, STEP_TEXT, STEP_CLEAR };
struct EyeStep { EyeStepType type; IEyes::Emotion emo; char text[CMD_MAX_LEN]; uint16_t pauseMs; };
static constexpr int MAX_EYE_STEPS = 24;
static constexpr uint16_t DEFAULT_STEP_PAUSE_MS = 200;

static uint16_t clampU16(int v, int lo, int hi) {
  if (v < lo) v = lo;
  if (v > hi) v = hi;
  return (uint16_t)v;
}

static bool parseEyeStep(const char* token, EyeStep& out) {
  out.type = STEP_EMO;
  out.pauseMs = DEFAULT_STEP_PAUSE_MS;
  out.text[0] = '\0';
  out.emo = IEyes::NEUTRAL;

  if (!token) return false;
  token = skipSpaces(token);
  if (*token == '\0') return false;

  const char* comma = strrchr(token, ',');
  char head[CMD_MAX_LEN];
  head[0] = '\0';

  if (comma) {
    size_t n = (size_t)(comma - token);
    if (n >= sizeof(head)) n = sizeof(head) - 1;
    memcpy(head, token, n);
    head[n] = '\0';
    out.pauseMs = clampU16(atoi(skipSpaces(comma + 1)), 0, 5000);
  } else {
    strncpy(head, token, sizeof(head) - 1);
    head[sizeof(head) - 1] = '\0';
  }

  const char* h = skipSpaces(head);

  if (strcasecmp(h, "clear") == 0) {
    out.type = STEP_CLEAR;
    return true;
  }

  if (startsWithNoCase(h, "text:")) {
    const char* t = skipSpaces(afterPrefix(h, "text:"));
    if (*t == '\0') return false;
    out.type = STEP_TEXT;
    strncpy(out.text, t, sizeof(out.text) - 1);
    out.text[sizeof(out.text) - 1] = '\0';
    return true;
  }

  IEyes::Emotion emo;
  if (parseEmotion(h, emo)) {
    out.type = STEP_EMO;
    out.emo = emo;
    return true;
  }

  return false;
}

static int parseEyesSequence(const char* payload, EyeStep* steps, int maxSteps) {
  if (!payload) return 0;

  char buf[CMD_MAX_LEN];
  strncpy(buf, payload, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  int count = 0;
  char* saveptr = nullptr;
  char* tok = strtok_r(buf, ";", &saveptr);

  while (tok && count < maxSteps) {
    EyeStep st{};
    if (parseEyeStep(tok, st)) steps[count++] = st;
    tok = strtok_r(nullptr, ";", &saveptr);
  }
  return count;
}

void taskEyes(void* /*arg*/) {
  // ensure eyes implementation is available (adapter or injected mock)
  IEyes::IInterface* eyes = IEyes::getGlobal();
  static MD_RobotEyesAdapter defaultAdapter;
  if (!eyes) {
    IEyes::setGlobal(&defaultAdapter);
    eyes = IEyes::getGlobal();
  }
  if (eyes) {
    eyes->begin();
    eyes->setIntensity(1);
    eyes->setAnimation(IEyes::LOOK_U, true);
  }

  enum EyeMode : uint8_t { MODE_IDLE, MODE_TEXT_LOOP, MODE_SEQ };
  EyeMode mode = MODE_IDLE;

  static char loopText[CMD_MAX_LEN] = {0};
  bool textActive = false;

  static char seqBuf[CMD_MAX_LEN] = {0};
  char* seqCur = nullptr;
  bool waitingForFinish = false;

  // track pending action ids for ack
  static uint32_t pending_action_id = 0; // single emotion / text
  static uint32_t seq_action_id = 0;     // eyes_seq id

  EyesCmd cmd{};
  IEyes::Emotion emo;

  std::function<void(void)> startNextSeqStep;

  startNextSeqStep = [&]() {
    if (!seqCur || *seqCur == '\0') {
      mode = MODE_IDLE;
      waitingForFinish = false;
      // sequence ended -> signal done if requested
      if (seq_action_id != 0 && g_done_q) {
        DoneEvent de{seq_action_id};
        xQueueSend(g_done_q, &de, 0);
        seq_action_id = 0;
      }
      return;
    }

    char* tok = seqCur;
    char* semi = strchr(tok, ';');
    if (semi) {
      *semi = '\0';
      seqCur = semi + 1;
    } else {
      seqCur = tok + strlen(tok);
    }

    tok = (char*)skipSpaces(tok);
    if (*tok == '\0') { startNextSeqStep(); return; }

    if (strcasecmp(tok, "clear") == 0) {
      IEyes::IInterface* eyesImpl = IEyes::getGlobal();
      if (eyesImpl) { eyesImpl->clear(); eyesImpl->setAnimation(IEyes::NEUTRAL, true); }
      waitingForFinish = true;
      return;
    }

    if (startsWithNoCase(tok, "text:")) {
      const char* t = skipSpaces(afterPrefix(tok, "text:"));
      IEyes::IInterface* eyesImpl = IEyes::getGlobal();
      if (*t) { if (eyesImpl) eyesImpl->setText(t); waitingForFinish = true; }
      else { startNextSeqStep(); }
      return;
    }

    if (startsWithNoCase(tok, "eyes:")) {
      tok = (char*)skipSpaces(afterPrefix(tok, "eyes:"));
    }

    if (parseEmotion(tok, emo)) {
      IEyes::IInterface* eyesImpl = IEyes::getGlobal();
      if (eyesImpl) eyesImpl->setAnimation(emo, true);
      waitingForFinish = true;
      return;
    }

    Serial.printf("[EYES] seq unknown step: %s\n", tok);
    startNextSeqStep();
  };

  for (;;) {
    IEyes::IInterface* eyesImpl = IEyes::getGlobal();
    bool finished = eyesImpl ? eyesImpl->runAnimation() : true;

    if (mode == MODE_TEXT_LOOP && textActive && finished) {
      if (eyesImpl) eyesImpl->setText(loopText);
    }

    if (mode == MODE_SEQ) {
      if (waitingForFinish) {
        if (finished) { waitingForFinish = false; startNextSeqStep(); }
      } else {
        startNextSeqStep();
      }
    }

    if (xQueueReceive(g_cmd_q, &cmd, 0) == pdTRUE) {
      const char* s = skipSpaces(cmd.text);
      Serial.printf("[EYES] rx: %s\n", s);
      // reset modes
      mode = MODE_IDLE;
      textActive = false;
      loopText[0] = '\0';
      seqBuf[0] = '\0';
      seqCur = nullptr;
      waitingForFinish = false;

      // clear pending ids
      pending_action_id = cmd.id; // used for single emotion/text; seq uses seq_action_id
      seq_action_id = 0;

      if (strcasecmp(s, "clear") == 0) {
        IEyes::IInterface* eyesImpl = IEyes::getGlobal();
        if (eyesImpl) { eyesImpl->clear(); eyesImpl->setAnimation(IEyes::NEUTRAL, true); }
        vTaskDelay(pdMS_TO_TICKS(5));
        continue;
      }

      if (startsWithNoCase(s, "eyes_seq:")) {
        const char* payload = skipSpaces(afterPrefix(s, "eyes_seq:"));
        if (strcasecmp(payload, "stop") == 0) {
          IEyes::IInterface* eyesImpl = IEyes::getGlobal();
          if (eyesImpl) { eyesImpl->clear(); eyesImpl->setAnimation(IEyes::NEUTRAL, true); }
          vTaskDelay(pdMS_TO_TICKS(5));
          continue;
        }
        strncpy(seqBuf, payload, sizeof(seqBuf) - 1);
        seqBuf[sizeof(seqBuf) - 1] = '\0';
        seqCur = seqBuf;
        mode = MODE_SEQ;
        waitingForFinish = false;
        // record ack id for whole sequence
        seq_action_id = cmd.id;
        pending_action_id = 0;
        vTaskDelay(pdMS_TO_TICKS(5));
        continue;
      }

      if (startsWithNoCase(s, "text:")) {
        const char* t = skipSpaces(afterPrefix(s, "text:"));
        if (*t) {
          // if an id was requested, show text once and mark pending_action_id
          if (cmd.id != 0) {
            strncpy(loopText, t, sizeof(loopText) - 1);
            loopText[sizeof(loopText) - 1] = '\0';
            textActive = false;
            mode = MODE_IDLE;
            if (eyesImpl) eyesImpl->setText(loopText);
            waitingForFinish = true;
            // pending_action_id already set to cmd.id
          } else {
            strncpy(loopText, t, sizeof(loopText) - 1);
            loopText[sizeof(loopText) - 1] = '\0';
            textActive = true;
            mode = MODE_TEXT_LOOP;
            if (eyesImpl) eyesImpl->setText(loopText);
          }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
        continue;
      }

      if (startsWithNoCase(s, "eyes:")) {
        s = skipSpaces(afterPrefix(s, "eyes:"));
      }

      if (parseEmotion(s, emo)) {
        IEyes::IInterface* eyesImpl = IEyes::getGlobal();
        if (eyesImpl) eyesImpl->setAnimation(emo, true);
        // pending_action_id already holds cmd.id for a single emotion
        waitingForFinish = true;
      } else {
        Serial.println("[EYES] unknown cmd");
      }
    }

    // detect finished single actions and signal done
    static bool lastWaiting = false;
    if (lastWaiting && !waitingForFinish) {
      // a waiting action finished in previous tick; handled by other logic
    }

    // if an action we started (single emotion or single text) finished, signal it
    if (!waitingForFinish) {
      if (pending_action_id != 0) {
        DoneEvent de{pending_action_id};
        if (g_done_q) xQueueSend(g_done_q, &de, 0);
        pending_action_id = 0;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
