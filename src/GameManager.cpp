// ... includes unchanged
using namespace Platform;

// (globals unchanged)

static unsigned long lastDebounceA = 0, lastDebounceB = 0;
static constexpr unsigned DEBOUNCE_MS = 220;
static constexpr unsigned EXIT_HOLD_MS = 1500;

// NEW: track previous button states for real rising-edge detection
static bool prevAState = false;
static bool prevBState = false;

static bool buttonPressedRising(bool now, bool& prev, unsigned long& lastTs) {
  unsigned long t = Millis();
  bool rising = now && !prev;
  prev = now; // update snapshot every frame
  if (rising && (t - lastTs) > DEBOUNCE_MS) { lastTs = t; return true; }
  return false;
}

// ... UI/highscore helpers unchanged ...

void initGameManager() {
  int v;
  if (StorageGet("hs_snake", v)) gHigh[static_cast<int>(GameID::SNAKE)] = v;
  if (StorageGet("hs_pong",  v)) gHigh[static_cast<int>(GameID::PONG)]  = v;

  gState = SysState::BOOT;
  gBootStart = Millis();
  gMenuIndex = 0;
  prevAState = prevBState = false;  // reset debounce
}

void runGameLoop() {
  if (gState == SysState::BOOT) {
    if (Millis() - gBootStart < 1000) { showBootAnimationFrame(); return; }
    gState = SysState::MENU; prevAState = prevBState = false; showMenu(); return;
  }

  if (gState == SysState::GAME_OVER) {
    if (Millis() < gGameOverUntil) return;
    gState = SysState::MENU; prevAState = prevBState = false; showMenu(); return;
  }

  if (gState == SysState::MENU) {
    InputState s = getInputState();

    if (buttonPressedRising(s.buttonB, prevBState, lastDebounceB)) {
      gMenuIndex = (gMenuIndex + 1) % gMenuCount; showMenu(); return;
    }
    if (buttonPressedRising(s.buttonA, prevAState, lastDebounceA)) {
      const char* pick = gMenuItems[gMenuIndex];
      if (std::strcmp(pick, "3.Sleep") == 0) {
        sleepModeWeb(); gMenuIndex = 0; showMenu(); return;
      }
      gActiveGame = (std::strcmp(pick, "1.Snake")==0) ? GameID::SNAKE : GameID::PONG;
      gGameInited = false; gCurrentScore = 0; gExitReq = gGameOver = false;
      gState = SysState::IN_GAME; return;
    }
    return;
  }

  if (gState == SysState::IN_GAME) {
    if (!gGameInited) { if (gActiveGame == GameID::SNAKE) startSnake(); else startPong(); gGameInited = true; }
    bool ok = (gActiveGame == GameID::SNAKE)
              ? stepSnake(gCurrentScore, gExitReq, gGameOver)
              : stepPong (gCurrentScore, gExitReq, gGameOver);

    if (!ok || gExitReq) { gState = SysState::MENU; prevAState = prevBState = false; showMenu(); return; }
    if (gGameOver) {
      considerHighScore(gActiveGame, gCurrentScore);
      gGameOverName  = (gActiveGame == GameID::SNAKE) ? "SNAKE" : "PONG";
      gGameOverScore = gCurrentScore;
      showGameOver(gGameOverName, gGameOverScore, getHighScore(gActiveGame));
      gGameOverUntil = Millis() + 1500;
      gState = SysState::GAME_OVER;
      return;
    }
    return;
  }
}
