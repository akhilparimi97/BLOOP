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

// Enhanced button debouncing with exit state tracking
static unsigned long lastDebounceA = 0, lastDebounceB = 0;
static constexpr unsigned DEBOUNCE_MS = 200;  // Increased debounce time
static constexpr unsigned EXIT_HOLD_MS = 1500;

// Proper edge detection with state tracking
static bool prevAState = false, prevBState = false;
static bool currentAState = false, currentBState = false;

// Exit state management
static bool wasInExitSequence = false;
static unsigned long exitSequenceEndTime = 0;
static constexpr unsigned EXIT_COOLDOWN_MS = 500;  // Cooldown after exit

// Frame rate limiting for smooth gameplay
static unsigned long lastFrameTime = 0;
static constexpr unsigned TARGET_FRAME_MS = 16;  // ~60 FPS

static bool buttonPressedRising(bool now, bool& prev, unsigned long& lastTs) { 
  unsigned long t = Millis();
  bool rising = now && !prev;
  prev = now; // Always update state
  
  // Ignore inputs during exit cooldown period
  if (wasInExitSequence && (t - exitSequenceEndTime) < EXIT_COOLDOWN_MS) {
    return false;
  }
  
  if (rising && (t - lastTs) > DEBOUNCE_MS) { 
    lastTs = t; 
    return true;
  } 
  return false; 
}

// GAME_OVER overlay timing
static unsigned long gGameOverUntil = 0;
static const char*   gGameOverName  = nullptr;
static int           gGameOverScore = 0;

// Frame rate control with boot animation exception
static void limitFrameRate(bool allowFastUpdates = false) {
  if (allowFastUpdates) {
    // For boot animation, allow faster updates
    Delay(10);
    return;
  }
  
  unsigned long now = Millis();
  unsigned long elapsed = now - lastFrameTime;
  
  if (elapsed < TARGET_FRAME_MS) {
    Delay(TARGET_FRAME_MS - elapsed);
  }
  lastFrameTime = Millis();
}

// Wait for all buttons to be released with exit sequence tracking
static void waitForButtonRelease() {
  // Mark that we're ending an exit sequence
  if (wasInExitSequence) {
    exitSequenceEndTime = Millis();
  }
  
  unsigned long startWait = Millis();
  while (true) {
    InputState s = getInputState();
    if (!s.buttonA && !s.buttonB) {
      // Wait extra time to ensure clean release
      Delay(150);
      
      // Double-check after delay
      s = getInputState();
      if (!s.buttonA && !s.buttonB) {
        break;
      }
    }
    
    // Prevent infinite loop
    if (Millis() - startWait > 3000) {
      break; // Safety timeout after 3 seconds
    }
    
    Delay(50);
  }
  
  // Reset all button states after release
  prevAState = prevBState = false;
  currentAState = currentBState = false;
  lastDebounceA = lastDebounceB = Millis();
  
  // Clear exit sequence flag after cooldown
  wasInExitSequence = false;
}

// ---------- Input ----------
InputState getInputState() {
  InputState s;
  
  // Read current button states
  currentAState = ButtonPressed(Button::BTN_A);
  currentBState = ButtonPressed(Button::BTN_B);
  
  s.buttonA = currentAState;
  s.buttonB = currentBState;
  s.both    = s.buttonA && s.buttonB;
  return s;
}

// Edge-triggered button press detection
bool getButtonAPressed() {
  return buttonPressedRising(currentAState, prevAState, lastDebounceA);
}

bool getButtonBPressed() {
  return buttonPressedRising(currentBState, prevBState, lastDebounceB);
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
  
  // Multi-phase animation
  if (t < 800) {
    // Phase 1: Sliding text
    int textWidth = 5 * 6 * 2; // "BLOOP" is 5 chars * 6 pixels * scale 2
    int maxOffset = SCREEN_WIDTH + textWidth;
    int offset = (int)((t * maxOffset) / 800) - textWidth;
    
    ClearDisplay();
    if (offset < SCREEN_WIDTH) {
      DrawText(offset, 25, "BLOOP", 2, true);
    }
    Present();
  } else if (t < 1200) {
    // Phase 2: Centered with blinking
    ClearDisplay();
    bool blink = ((t - 800) / 100) % 2 == 0; // Blink every 100ms
    if (blink) {
      int centerX = (SCREEN_WIDTH - (5 * 6 * 2)) / 2;
      DrawText(centerX, 25, "BLOOP", 2, true);
    }
    Present();
  } else {
    // Phase 3: Final display with version
    ClearDisplay();
    int centerX = (SCREEN_WIDTH - (5 * 6 * 2)) / 2;
    DrawText(centerX, 20, "BLOOP", 2, true);
    DrawText(centerX + 20, 40, "v1.0", 1, true);
    Present();
  }
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
  // Initialize high scores to 0 first
  gHigh[static_cast<int>(GameID::SNAKE)] = 0;
  gHigh[static_cast<int>(GameID::PONG)] = 0;
  
  // Then try to load from storage
  int v;
  if (StorageGet("hs_snake", v) && v >= 0 && v < 99999) { // Sanity check
    gHigh[static_cast<int>(GameID::SNAKE)] = v;
  }
  if (StorageGet("hs_pong", v) && v >= 0 && v < 99999) { // Sanity check
    gHigh[static_cast<int>(GameID::PONG)] = v;
  }

  gState = SysState::BOOT;
  gBootStart = Millis();
  gMenuIndex = 0;
  
  // Reset button states and exit tracking
  prevAState = prevBState = false;
  currentAState = currentBState = false;
  wasInExitSequence = false;
  exitSequenceEndTime = 0;
  lastFrameTime = Millis();
}

void runGameLoop() {
  // Update input state first
  InputState s = getInputState();
  
  if (gState == SysState::BOOT) {
    if (Millis() - gBootStart < 2000) {  // Extended boot time for better animation
      showBootAnimationFrame(); 
      limitFrameRate(true);  // Allow faster updates for smooth animation
      return; 
    }
    gState = SysState::MENU; 
    waitForButtonRelease();  // Ensure clean transition
    showMenu(); 
    limitFrameRate();
    return;
  }

  if (gState == SysState::GAME_OVER) {
    if (Millis() < gGameOverUntil) {
      limitFrameRate();
      return;
    }
    gState = SysState::MENU; 
    waitForButtonRelease();  // Ensure clean transition
    showMenu(); 
    limitFrameRate();
    return;
  }

  if (gState == SysState::MENU) {
    // Check if we're still in exit cooldown
    if (wasInExitSequence && (Millis() - exitSequenceEndTime) < EXIT_COOLDOWN_MS) {
      limitFrameRate();
      return;
    }
    
    if (getButtonBPressed()) {
      gMenuIndex = (gMenuIndex + 1) % gMenuCount; 
      showMenu(); 
      limitFrameRate();
      return;
    }
    
    if (getButtonAPressed()) {
      const char* pick = gMenuItems[gMenuIndex];
      if (std::strcmp(pick, "3.Sleep") == 0) {
        sleepModeWeb(); 
        gMenuIndex = 0; 
        showMenu(); 
        limitFrameRate();
        return;
      }
      gActiveGame = (std::strcmp(pick, "1.Snake")==0) ? GameID::SNAKE : GameID::PONG;
      gGameInited = false; 
      gCurrentScore = 0; 
      gExitReq = gGameOver = false;
      gState = SysState::IN_GAME;
      waitForButtonRelease();  // Prevent immediate input in game
      limitFrameRate();
      return;
    }
    
    limitFrameRate();
    return;
  }

  if (gState == SysState::IN_GAME) {
    if (!gGameInited) { 
      if (gActiveGame == GameID::SNAKE) startSnake(); 
      else startPong(); 
      gGameInited = true; 
    }
    
    bool ok = (gActiveGame == GameID::SNAKE)
              ? stepSnake(gCurrentScore, gExitReq, gGameOver)
              : stepPong (gCurrentScore, gExitReq, gGameOver);

    if (!ok || gExitReq) { 
      // Mark that we're exiting from an exit sequence
      wasInExitSequence = true;
      gState = SysState::MENU; 
      waitForButtonRelease();  // Critical: wait for button release before menu
      showMenu(); 
      limitFrameRate();
      return; 
    }
    
    if (gGameOver) {
      considerHighScore(gActiveGame, gCurrentScore);
      gGameOverName  = (gActiveGame == GameID::SNAKE) ? "SNAKE" : "PONG";
      gGameOverScore = gCurrentScore;
      showGameOver(gGameOverName, gGameOverScore, getHighScore(gActiveGame));
      gGameOverUntil = Millis() + 1500;
      gState = SysState::GAME_OVER;
      waitForButtonRelease();  // Ensure clean transition
      limitFrameRate();
      return;
    }
    
    limitFrameRate();
    return;
  }
}