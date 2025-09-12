#include "GameManager.h"
#include "platform.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

// ---------- State ----------
static int   gHigh[static_cast<int>(GameID::COUNT)] = {0,0};

static enum class SysState { BOOT, MENU, IN_GAME, GAME_OVER } gState = SysState::BOOT;
static unsigned long gBootStart = 0;

static int gMenuIndex = 0;
static const char* gMenuItems[] = { "1.Snake", "2.Pong", "3.Sleep" };
static constexpr int gMenuCount = 3;

static GameID        gActiveGame = GameID::SNAKE;
static bool          gGameInited = false;
static int           gCurrentScore = 0;
static bool          gExitReq = false;
static bool          gGameOver = false;

static unsigned long lastDebounceA = 0, lastDebounceB = 0;
static constexpr unsigned DEBOUNCE_MS = 220;
static constexpr unsigned EXIT_HOLD_MS = 1500;

// GAME_OVER overlay timing
static unsigned long gGameOverUntil = 0;
static const char*   gGameOverName  = nullptr;
static int           gGameOverScore = 0;

// NEW: track previous button states for real rising-edge detection
static bool prevAState = false;
static bool prevBState = false;

// ---------- Input ----------
InputState getInputState() {
  InputState s;
  s.buttonA = Platform::ButtonPressed(Platform::Button::BTN_A);
  s.buttonB = Platform::ButtonPressed(Platform::Button::BTN_B);
  s.both    = s.buttonA && s.buttonB;
  return s;
}

static bool buttonPressedRising(bool now, bool& prev, unsigned long& lastTs) {
  unsigned long t = Platform::Millis();
  bool rising = now && !prev;
  prev = now; // update snapshot every frame
  if (rising && (t - lastTs) > DEBOUNCE_MS) { lastTs = t; return true; }
  return false;
}

// ---------- UI ----------
void drawStatusBar(const char* gameName, int currentScore, int highScore) {
  Platform::FillRect(0, 0, Platform::SCREEN_WIDTH, Platform::STATUS_BAR_HEIGHT, false);
  Platform::DrawText(0, 2, "HSC:", 1, true);
  char buf[16]; std::snprintf(buf, sizeof(buf), "%d", highScore);
  Platform::DrawText(24, 2, buf, 1, true);

  Platform::DrawText(50, 2, "Scr:", 1, true);
  std::snprintf(buf, sizeof(buf), "%d", currentScore);
  Platform::DrawText(75, 2, buf, 1, true);

  Platform::DrawText(110, 2, "BAT", 1, true);
}

void drawStatusBarMenu() {
  Platform::FillRect(0, 0, Platform::SCREEN_WIDTH, Platform::STATUS_BAR_HEIGHT, false);
  Platform::DrawText(48, 2, "BLOOP", 1, true);
  Platform::DrawText(110, 2, "BAT",  1, true);
  Platform::Present();
}

void showExitHoldBar(float progress) {
  if (progress < 0) progress = 0;
  if (progress > 1) progress = 1;
  int w = static_cast<int>(Platform::SCREEN_WIDTH * progress);
  Platform::FillRect(0, Platform::SCREEN_HEIGHT - 2, Platform::SCREEN_WIDTH, 2, false);
  Platform::DrawLine(0, Platform::SCREEN_HEIGHT - 1, w, Platform::SCREEN_HEIGHT - 1, true);
  Platform::Present();
}

void clearPlayfield() {
  Platform::FillRect(0, Platform::STATUS_BAR_HEIGHT, Platform::SCREEN_WIDTH, Platform::PLAYFIELD_HEIGHT, false);
}

void showGameOver(const char* gameName, int score, int highScore) {
  clearPlayfield();
  drawStatusBar(gameName, score, highScore);
  Platform::DrawText(30, Platform::STATUS_BAR_HEIGHT + 5,  "Game Over", 1, true);
  char buf[32];
  std::snprintf(buf, sizeof(buf), "Score: %d", score);
  Platform::DrawText(20, Platform::STATUS_BAR_HEIGHT + 20, buf, 1, true);
  std::snprintf(buf, sizeof(buf), "HighScore: %d", highScore);
  Platform::DrawText(20, Platform::STATUS_BAR_HEIGHT + 35, buf, 1, true);
  Platform::Present();
}

void showGetReady(const char* gameName, const char* instructions) {
  clearPlayfield();
  int hs = (gameName && std::strcmp(gameName, "SNAKE")==0) ? getHighScore(GameID::SNAKE) : getHighScore(GameID::PONG);
  drawStatusBar(gameName, 0, hs);
  Platform::DrawText(35, Platform::STATUS_BAR_HEIGHT + 15, "Get Ready...", 1, true);
  if (instructions) Platform::DrawText(15, Platform::STATUS_BAR_HEIGHT + 30, instructions, 1, true);
  Platform::Present();
}

// ---------- High scores (persist via Platform::Storage*) ----------
int getHighScore(GameID id) {
  return gHigh[static_cast<int>(id)];
}
void considerHighScore(GameID id, int score) {
  int idx = static_cast<int>(id);
  if (score > gHigh[idx]) {
    gHigh[idx] = score;
    if (id == GameID::SNAKE) Platform::StorageSet("hs_snake", score);
    if (id == GameID::PONG)  Platform::StorageSet("hs_pong",  score);
  }
}

// ---------- Menu ----------
static void showMenu() {
  Platform::ClearDisplay();
  drawStatusBarMenu();
  for (int i = 0; i < gMenuCount; ++i) {
    int y = Platform::STATUS_BAR_HEIGHT + 5 + i * 10;
    Platform::DrawText(0,  y, (i == gMenuIndex) ? "> " : "  ", 1, true);
    Platform::DrawText(12, y, gMenuItems[i], 1, true);
  }
  Platform::Present();
}

// ---------- Boot ----------
static void showBootAnimationFrame() {
  unsigned long t = Platform::Millis() - gBootStart;
  int i = static_cast<int>((t / 20) % (Platform::SCREEN_WIDTH / 4 + 1)) * 4;
  Platform::ClearDisplay();
  Platform::DrawText(i, 25, "BLOOP", 2, true);
  Platform::Present();
}

// ---------- Sleep (web mock / no-op on HW) ----------
static void sleepModeWeb() {
  Platform::ClearDisplay();
  Platform::DrawText(20, 25, "Sleeping...", 1, true);
  Platform::Present();
  Platform::Delay(500);
}

// ---------- Manager ----------
void initGameManager() {
  int v;
  if (Platform::StorageGet("hs_snake", v)) gHigh[static_cast<int>(GameID::SNAKE)] = v;
  if (Platform::StorageGet("hs_pong",  v)) gHigh[static_cast<int>(GameID::PONG)]  = v;

  gState = SysState::BOOT;
  gBootStart = Platform::Millis();
  gMenuIndex = 0;
  prevAState = prevBState = false;  // reset debounce
}

void runGameLoop() {
  if (gState == SysState::BOOT) {
    if (Platform::Millis() - gBootStart < 1000) { showBootAnimationFrame(); return; }
    gState = SysState::MENU; prevAState = prevBState = false; showMenu(); return;
  }

  if (gState == SysState::GAME_OVER) {
    if (Platform::Millis() < gGameOverUntil) return;
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
      gGameOverUntil = Platform::Millis() + 1500;
      gState = SysState::GAME_OVER;
      return;
    }
    return;
  }
}
