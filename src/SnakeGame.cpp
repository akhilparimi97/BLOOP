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
  constexpr unsigned MOVE_DELAY_MS_BASE = 300;   // base timing (scaled by SpeedScale)
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

  // Get Ready gating
  static unsigned long readyUntil = 0;
  static bool clearedAfterReady = false;

  static bool isValid(Dir nd, Dir cd) {
    return !((nd==UP && cd==DOWN) || (nd==DOWN && cd==UP) ||
             (nd==LEFT && cd==RIGHT) || (nd==RIGHT && cd==LEFT));
  }

  static void placeFood() {
    bool ok;
    do {
      ok = true;
      food.x = RandomInt(0, GRID_WIDTH);
      food.y = RandomInt(0, GRID_HEIGHT);
      for (int i=0;i<snakeLen;i++) if (snake[i].x==food.x && snake[i].y==food.y) { ok=false; break; }
    } while(!ok);
  }

  static void resetSnake() {
    snakeLen = INITIAL_SNAKE_LENGTH;
    snake[0] = {GRID_WIDTH/2, GRID_HEIGHT/2};
    snake[1] = {GRID_WIDTH/2 - 1, GRID_HEIGHT/2};
    snake[2] = {GRID_WIDTH/2 - 2, GRID_HEIGHT/2};
    dir = RIGHT;
    placeFood();
    lastMoveTime = Millis();
  }

  static bool moveSnake() {
  // remember old tail before shifting so growth can append it
  Pt oldTail = snake[snakeLen - 1];

  // shift body forward
  for (int i = snakeLen - 1; i > 0; --i) snake[i] = snake[i - 1];

  // advance head
  switch (dir) { case UP:    snake[0].y--; break;
                 case DOWN:  snake[0].y++; break;
                 case LEFT:  snake[0].x--; break;
                 case RIGHT: snake[0].x++; break; }

  // wrap
  if (snake[0].x < 0) snake[0].x = GRID_WIDTH - 1;
  if (snake[0].x >= GRID_WIDTH) snake[0].x = 0;
  if (snake[0].y < 0) snake[0].y = GRID_HEIGHT - 1;
  if (snake[0].y >= GRID_HEIGHT) snake[0].y = 0;

  // self-hit
  for (int i = 1; i < snakeLen; ++i)
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) return false;

  // eat
  if (snake[0].x == food.x && snake[0].y == food.y) {
    if (snakeLen < MAX_SNAKE_LENGTH) {
      // grow by appending the previous tail position so no junk coords appear
      snakeLen = std::min(snakeLen + 1, MAX_SNAKE_LENGTH);
      snake[snakeLen - 1] = oldTail;
    }
    placeFood();
  }

  return true;
}

  static void drawSnake(int score) {
    // FULL redraw each frame -> no ghost artifacts
    clearPlayfield();
    drawStatusBar("SNAKE", score, getHighScore(GameID::SNAKE));

    // Draw snake body
    for (int i=0;i<snakeLen;i++)
      FillRect(snake[i].x*GRID_SIZE, snake[i].y*GRID_SIZE + STATUS_BAR_HEIGHT, GRID_SIZE, GRID_SIZE, true);

    // Food
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
  if (!clearedAfterReady) { clearPlayfield(); Present(); clearedAfterReady = true; }

  InputState in = getInputState();

  // Hold-to-exit (PAUSES GAME)
  if (in.both) {
    if (exitHoldStart == 0) exitHoldStart = Millis();
    float prog = (float)(Millis() - exitHoldStart) / 1500.0f;
    if (prog >= 1.0f) { exitRequested = true; exitHoldStart = 0; return true; }
    showExitHoldBar(prog);
    Delay(50);
    return true;
  } else {
    exitHoldStart = 0;
  }

  // Turn handling
  static bool turnedThisFrame = false;
  static int  safe = 1;
  if (!turnedThisFrame && safe <= 0) {
    if (in.buttonA) { Dir nd = (Dir)((dir + 3) % 4); if (isValid(nd, dir)) { dir = nd; turnedThisFrame = true; } }
    else if (in.buttonB) { Dir nd = (Dir)((dir + 1) % 4); if (isValid(nd, dir)) { dir = nd; turnedThisFrame = true; } }
  }

  int score = snakeLen - INITIAL_SNAKE_LENGTH;

  const unsigned moveDelay = (unsigned)(MOVE_DELAY_MS_BASE * SpeedScale());
  if (Millis() - lastMoveTime > moveDelay) {
    if (!moveSnake()) { gameOver = true; outScore = score; return true; }
    lastMoveTime = Millis();
    safe--; turnedThisFrame = false;
  }

  drawSnake(score);
  outScore = score;
  gameOver = false;
  return true;
}

