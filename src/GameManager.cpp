#include "GameManager.h"
#include "platform.h"
#include <algorithm>
#include <cstring>

using namespace Platform;

static int   gHigh[static_cast<int>(GameID::COUNT)] = {0,0};
static bool  gJustBooted = true;

static enum class SysState { BOOT, MENU, IN_GAME } gState = SysState::BOOT;
static unsigned long gBootStart = 0;

static int      gMenuIndex = 0;
static const char* gMenuItems[] = { "1.Snake", "2.Pong", "3.Sleep" };
static constexpr int gMenuCount = 3;

static GameID        gActiveGame = GameID::SNAKE;
static bool          gGameInited = false;
static int           gCurrentScore = 0;
static bool          gExitReq = false;
static bool          gGameOver = false;

static unsigned long lastDebounceA = 0, lastDebounceB = 0;
static constexpr unsigned DEBOUNCE_MS = 200;
static constexpr unsigned EXIT_HOLD_MS = 1500;

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
  DrawText(0, 2, "HSC:", 1, true);     // high score
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%d", highScore);
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
  drawStatusBar(gameName, 0, (gameName && std::strcmp(gameName,"SNAKE")==0) ? getHighScore(GameID::SNAKE) : getHighScore(GameID::PONG));
  DrawText(35, STATUS_BAR_HEIGHT + 15, "Get Ready...", 1, true);
  if (instructions) DrawText(15, STATUS_BAR_HEIGHT + 30, instructions, 1, true);
  Present();
}

// ---------- High scores ----------
int getHighScore(GameID id) {
  return gHigh[static_cast<int>(id)];
}

void considerHighScore(GameID id, int score) {
  gHigh[static_cast<int>(id)] = std::max(gHigh[static_cast<int>(id)], score);
}

// ---------- Menu drawing ----------
static void showMenu() {
  ClearDisplay();
  drawStatusBarMenu();
  for (int i = 0; i < gMenuCount; ++i) {
    int y = STATUS_BAR_HEIGHT + 5 + i * 10;
    DrawText(0, y, (i == gMenuIndex) ? "> " : "  ", 1, true);
    DrawText(12, y, gMenuItems[i], 1, true);
  }
  Present();
}

// ---------- Boot anim ----------
static void showBootAnimationFrame() {
  unsigned long t = Millis() - gBootStart;
  int i = static_cast<int>((t / 20) % (SCREEN_WIDTH / 4 + 1)) * 4;
  ClearDisplay();
  DrawText(i, 25, "BLOOP", 2, true);
  Present();
}

// ---------- Game forward declarations (step functions declared in header) ----------
void startSnake();
void startPong();

// ---------- Sleep pseudo-mode (web) ----------
static void sleepModeWeb() {
  ClearDisplay();
  DrawText(20, 25, "Sleeping...", 1, true);
  Present();
  // No real sleep in web; just return to menu after a short pause
  Delay(500);
}

// ---------- Manager ----------
void initGameManager() {
  gState = SysState::BOOT;
  gBootStart = Millis();
  gMenuIndex = 0;
}

void runGameLoop() {
  if (gState == SysState::BOOT) {
    // 1 second boot animation
    if (Millis() - gBootStart < 1000) {
      showBootAnimationFrame();
      return;
    }
    gState = SysState::MENU;
    showMenu();
    return;
  }

  if (gState == SysState::MENU) {
    InputState s = getInputState();
    if (buttonPressedEdge(s.buttonB, lastDebounceB)) {
      gMenuIndex = (gMenuIndex + 1) % gMenuCount;
      showMenu();
      return;
    }
    if (buttonPressedEdge(s.buttonA, lastDebounceA)) {
      const char* pick = gMenuItems[gMenuIndex];
      if (std::strcmp(pick, "3.Sleep") == 0) {
        sleepModeWeb();
        gMenuIndex = 0;
        showMenu();
        return;
      }
      if (std::strcmp(pick, "1.Snake") == 0) {
        gActiveGame = GameID::SNAKE; gGameInited = false; gCurrentScore = 0; gExitReq = gGameOver = false;
      } else {
        gActiveGame = GameID::PONG;  gGameInited = false; gCurrentScore = 0; gExitReq = gGameOver = false;
      }
      gState = SysState::IN_GAME;
      return;
    }
    return;
  }

  if (gState == SysState::IN_GAME) {
    bool stepOk;
    if (!gGameInited) {
      if (gActiveGame == GameID::SNAKE) startSnake(); else startPong();
      gGameInited = true;
    }
    if (gActiveGame == GameID::SNAKE)
      stepOk = stepSnake(gCurrentScore, gExitReq, gGameOver);
    else
      stepOk = stepPong (gCurrentScore, gExitReq, gGameOver);

    if (!stepOk || gExitReq) {
      // Exit back to menu (no score update when forced exit)
      gState = SysState::MENU;
      showMenu();
      return;
    }
    if (gGameOver) {
      considerHighScore(gActiveGame, gCurrentScore);
      showGameOver((gActiveGame == GameID::SNAKE) ? "SNAKE" : "PONG",
                   gCurrentScore, getHighScore(gActiveGame));
      Delay(1500);
      gState = SysState::MENU;
      showMenu();
      return;
    }
    return;
  }
}
