#include "GameManager.h"
#include "platform.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

using namespace Platform;

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

// ---------- Input ----------
InputState getInputState() {
  InputState s;
  s.buttonA = ButtonPressed(Button::BTN_A);
  s.buttonB = ButtonPressed(Button::BTN_B);
  s.both    = s.buttonA && s.buttonB;
  return s;
}

static bool buttonPressedEdge(bool now, unsigned long& lastTs) {
  unsigned long t = Millis();
  if (now && (t - lastTs) > DEBOUNCE_MS) { lastTs = t; return true; }
  return false;
}

// ---------- UI ----------
void drawStatusBar(const char* gameName, int currentScore, int highScore) {
  FillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, false);
  DrawText(0, 2, "HSC:", 1, true);
  char buf[16]; std::snprintf(buf, sizeof(buf), "%d", highScore);
  DrawText(24, 2, buf, 1, true);

  DrawText(50, 2, "Scr:", 1, true);
  std::snprintf(buf, sizeof(buf), "%d", currentScore);
  DrawText(75, 2, buf, 1, true);

  DrawText(110, 2, "BAT", 1, true);
}

void drawStatusBarMenu() {
  FillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, false);
  DrawText(48, 2, "BLOOP", 1, true);
  DrawText(110, 2, "BAT",  1, true);
  Present();
}

void showExitHoldBar(float progress) {
  if (progress < 0) progress = 0;
  if (progress > 1) progress = 1;
  int w = static_cast<int>(SCREEN_WIDTH * progress);
  FillRect(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH, 2, false);
  DrawLine(0, SCREEN_HEIGHT - 1, w, SCREEN_HEIGHT - 1, true);
  Present();
}

void clearPlayfield() {
  FillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, PLAYFIELD_HEIGHT, false);
}

void showGameOver(const char* gameName, int score, int highScore) {
  clearPlayfield();
  drawStatusBar(gameName, score, highScore);
  DrawText(30, STATUS_BAR_HEIGHT + 5,  "Game Over", 1, true);
  char buf[32];
  std::snprintf(buf, sizeof(buf), "Score: %d", score);
  DrawText(20, STATUS_BAR_HEIGHT + 20, buf, 1, true);
  std::snprintf(buf, sizeof(buf), "HighScore: %d", highScore);
  DrawText(20, STATUS_BAR_HEIGHT + 35, buf, 1, true);
  Present();
}

void showGetReady(const char* gameName, const char* instructions) {
  clearPlayfield();
  int hs = (gameName && std::strcmp(gameName, "SNAKE")==0) ? getHighScore(GameID::SNAKE) : getHighScore(GameID::PONG);
  drawStatusBar(gameName, 0, hs);
  DrawText(35, STATUS_BAR_HEIGHT + 15, "Get Ready...", 1, true);
  if (instructions) DrawText(15, STATUS_BAR_HEIGHT + 30, instructions, 1, true);
  Present();
}

// ---------- High scores (persist to localStorage on web) ----------
int getHighScore(GameID id) {
  return gHigh[static_cast<int>(id)];
}
void considerHighScore(GameID id, int score) {
  int idx = static_cast<int>(id);
  if (score > gHigh[idx]) {
    gHigh[idx] = score;
    if (id == GameID::SNAKE) StorageSet("hs_snake", score);
    if (id == GameID::PONG)  StorageSet("hs_pong",  score);
  }
}

// ---------- Menu ----------
static void showMenu() {
  ClearDisplay();
  drawStatusBarMenu();
  for (int i = 0; i < gMenuCount; ++i) {
    int y = STATUS_BAR_HEIGHT + 5 + i * 10;
    DrawText(0,  y, (i == gMenuIndex) ? "> " : "  ", 1, true);
    DrawText(12, y, gMenuItems[i], 1, true);
  }
  Present();
}

// ---------- Boot ----------
static void showBootAnimationFrame() {
  unsigned long t = Millis() - gBootStart;
  int i = static_cast<int>((t / 20) % (SCREEN_WIDTH / 4 + 1)) * 4;
  ClearDisplay();
  DrawText(i, 25, "BLOOP", 2, true);
  Present();
}

// ---------- Sleep (web mock) ----------
static void sleepModeWeb() {
  ClearDisplay();
  DrawText(20, 25, "Sleeping...", 1, true);
  Present();
  Delay(500);
}

// ---------- Manager ----------
void initGameManager() {
  // Load persisted highs (web)
  int v;
  if (StorageGet("hs_snake", v)) gHigh[static_cast<int>(GameID::SNAKE)] = v;
  if (StorageGet("hs_pong",  v)) gHigh[static_cast<int>(GameID::PONG)]  = v;

  gState = SysState::BOOT;
  gBootStart = Millis();
  gMenuIndex = 0;
}

void runGameLoop() {
  if (gState == SysState::BOOT) {
    if (Millis() - gBootStart < 1000) { showBootAnimationFrame(); return; }
    gState = SysState::MENU; showMenu(); return;
  }

  if (gState == SysState::GAME_OVER) {
    if (Millis() < gGameOverUntil) return;
    gState = SysState::MENU; showMenu(); return;
  }

  if (gState == SysState::MENU) {
    InputState s = getInputState();
    if (buttonPressedEdge(s.buttonB, lastDebounceB)) {
      gMenuIndex = (gMenuIndex + 1) % gMenuCount; showMenu(); return;
    }
    if (buttonPressedEdge(s.buttonA, lastDebounceA)) {
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

    if (!ok || gExitReq) { gState = SysState::MENU; showMenu(); return; }
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
