#include "SnakeGame.h"
#include "platform.h"
#include <algorithm>
#include <cstdlib>

using namespace Platform;

namespace {
  constexpr int GRID_SIZE   = 4;
  constexpr int GRID_HEIGHT = PLAYFIELD_HEIGHT / GRID_SIZE;
  constexpr int GRID_WIDTH  = SCREEN_WIDTH / GRID_SIZE;
  constexpr int MAX_SNAKE_LENGTH = 64;
  constexpr unsigned MOVE_DELAY_MS_BASE = 200;   // Reduced from 300ms for better responsiveness
  constexpr int INITIAL_SNAKE_LENGTH = 3;

  enum Dir { RIGHT, DOWN, LEFT, UP };
  struct Pt { int x, y; };

  static Pt  snake[MAX_SNAKE_LENGTH];
  static int snakeLen;
  static Dir dir;
  static Pt  food;
  static unsigned long lastMoveTime;
  static unsigned long exitHoldStart = 0;
  static bool inited = false;

  // Enhanced input handling
  static bool prevAPressed = false;
  static bool prevBPressed = false;
  static unsigned long lastInputTime = 0;
  static constexpr unsigned INPUT_COOLDOWN_MS = 100;  // Prevent double presses

  // Get Ready gating
  static unsigned long readyUntil = 0;
  static bool clearedAfterReady = false;

  static bool isValid(Dir nd, Dir cd) {
    return !((nd==UP && cd==DOWN) || (nd==DOWN && cd==UP) ||
             (nd==LEFT && cd==RIGHT) || (nd==RIGHT && cd==LEFT));
  }

  static void placeFood() {
    bool ok;
    int attempts = 0;
    do {
      ok = true;
      food.x = RandomInt(0, GRID_WIDTH);
      food.y = RandomInt(0, GRID_HEIGHT);
      for (int i=0;i<snakeLen;i++) {
        if (snake[i].x==food.x && snake[i].y==food.y) { 
          ok=false; 
          break; 
        }
      }
      attempts++;
    } while(!ok && attempts < 100);  // Prevent infinite loop
  }

  static void resetSnake() {
    snakeLen = INITIAL_SNAKE_LENGTH;
    snake[0] = {GRID_WIDTH/2, GRID_HEIGHT/2};
    snake[1] = {GRID_WIDTH/2 - 1, GRID_HEIGHT/2};
    snake[2] = {GRID_WIDTH/2 - 2, GRID_HEIGHT/2};
    dir = RIGHT;
    placeFood();
    lastMoveTime = Millis();
    lastInputTime = 0;
    prevAPressed = prevBPressed = false;
  }

  static bool moveSnake() {
    // Remember old tail before shifting so growth can append it
    Pt oldTail = snake[snakeLen - 1];

    // Shift body forward
    for (int i = snakeLen - 1; i > 0; --i) {
      snake[i] = snake[i - 1];
    }

    // Advance head
    switch (dir) { 
      case UP:    snake[0].y--; break;
      case DOWN:  snake[0].y++; break;
      case LEFT:  snake[0].x--; break;
      case RIGHT: snake[0].x++; break; 
    }

    // Wrap around screen
    if (snake[0].x < 0) snake[0].x = GRID_WIDTH - 1;
    if (snake[0].x >= GRID_WIDTH) snake[0].x = 0;
    if (snake[0].y < 0) snake[0].y = GRID_HEIGHT - 1;
    if (snake[0].y >= GRID_HEIGHT) snake[0].y = 0;

    // Self-collision check
    for (int i = 1; i < snakeLen; ++i) {
      if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
        return false;
      }
    }

    // Food collision
    if (snake[0].x == food.x && snake[0].y == food.y) {
      if (snakeLen < MAX_SNAKE_LENGTH) {
        snakeLen = std::min(snakeLen + 1, MAX_SNAKE_LENGTH);
        snake[snakeLen - 1] = oldTail;
      }
      placeFood();
    }

    return true;
  }

  static void drawSnake(int score) {
    // Full redraw each frame to prevent ghost artifacts
    clearPlayfield();
    drawStatusBar("SNAKE", score, getHighScore(GameID::SNAKE));

    // Draw snake body
    for (int i=0;i<snakeLen;i++) {
      FillRect(snake[i].x*GRID_SIZE, snake[i].y*GRID_SIZE + STATUS_BAR_HEIGHT, GRID_SIZE, GRID_SIZE, true);
    }

    // Draw food
    FillRect(food.x*GRID_SIZE, food.y*GRID_SIZE + STATUS_BAR_HEIGHT, GRID_SIZE, GRID_SIZE, true);
    Present();
  }

} // anon

void startSnake() {
  inited = true;
  resetSnake();
  showGetReady("SNAKE", "A: Left, B: Right");
  exitHoldStart = 0;
  readyUntil = Millis() + 1000;  // 1s get-ready
  clearedAfterReady = false;
}

bool stepSnake(int& outScore, bool& exitRequested, bool& gameOver) {
  if (!inited) startSnake();

  // Pause during Get Ready
  if (Millis() < readyUntil) return true;
  if (!clearedAfterReady) { 
    clearPlayfield(); 
    Present(); 
    clearedAfterReady = true; 
  }

  InputState in = getInputState();

  // Hold-to-exit (PAUSES GAME)
  if (in.both) {
    if (exitHoldStart == 0) exitHoldStart = Millis();
    float prog = (float)(Millis() - exitHoldStart) / 1500.0f;
    if (prog >= 1.0f) { 
      exitRequested = true; 
      exitHoldStart = 0;
      // Reset input states to prevent menu interference
      prevAPressed = prevBPressed = false;
      return true; 
    }
    showExitHoldBar(prog);
    Delay(50);
    return true;
  } else {
    exitHoldStart = 0;
  }

  // Enhanced turn handling with debouncing
  unsigned long now = Millis();
  bool canProcessInput = (now - lastInputTime) > INPUT_COOLDOWN_MS;
  
  if (canProcessInput) {
    bool currentAPressed = in.buttonA;
    bool currentBPressed = in.buttonB;
    
    // Detect rising edge (button press, not hold)
    bool aPressedNow = currentAPressed && !prevAPressed;
    bool bPressedNow = currentBPressed && !prevBPressed;
    
    if (aPressedNow) {
      Dir newDir = (Dir)((dir + 3) % 4); // Counter-clockwise
      if (isValid(newDir, dir)) {
        dir = newDir;
        lastInputTime = now;
      }
    } else if (bPressedNow) {
      Dir newDir = (Dir)((dir + 1) % 4); // Clockwise
      if (isValid(newDir, dir)) {
        dir = newDir;
        lastInputTime = now;
      }
    }
    
    // Update previous state
    prevAPressed = currentAPressed;
    prevBPressed = currentBPressed;
  }

  int score = snakeLen - INITIAL_SNAKE_LENGTH;

  // Movement timing with scaling
  const unsigned moveDelay = (unsigned)(MOVE_DELAY_MS_BASE * SpeedScale());
  if (Millis() - lastMoveTime > moveDelay) {
    if (!moveSnake()) { 
      gameOver = true; 
      outScore = score; 
      // Reset input states
      prevAPressed = prevBPressed = false;
      return true; 
    }
    lastMoveTime = Millis();
  }

  drawSnake(score);
  outScore = score;
  gameOver = false;
  return true;
}