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
  constexpr unsigned MOVE_DELAY_MS = 300;
  constexpr int INITIAL_SNAKE_LENGTH = 3;

  enum Dir { RIGHT, DOWN, LEFT, UP };
  struct Pt { int x, y; };

  static Pt  snake[MAX_SNAKE_LENGTH];
  static int snakeLen;
  static Dir dir;
  static Pt  food;
  static unsigned long lastMoveTime;
  static bool turnMade;
  static int  safeSteps;
  static Pt  prevTail = {-1,-1};

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
    safeSteps = 2;
    turnMade = false;
    prevTail = {-1,-1};
    lastMoveTime = Millis();
  }

  static bool moveSnake() {
    prevTail = snake[snakeLen-1];
    for (int i=snakeLen-1;i>0;--i) snake[i]=snake[i-1];
    switch(dir){case UP: snake[0].y--;break;case DOWN: snake[0].y++;break;case LEFT: snake[0].x--;break;case RIGHT: snake[0].x++;break;}
    if (snake[0].x < 0) snake[0].x = GRID_WIDTH - 1;
    if (snake[0].x >= GRID_WIDTH) snake[0].x = 0;
    if (snake[0].y < 0) snake[0].y = GRID_HEIGHT - 1;
    if (snake[0].y >= GRID_HEIGHT) snake[0].y = 0;
    for (int i=1;i<snakeLen;i++) if (snake[0].x==snake[i].x && snake[0].y==snake[i].y) return false;
    if (snake[0].x==food.x && snake[0].y==food.y) {
      snakeLen = std::min(snakeLen+1, MAX_SNAKE_LENGTH);
      placeFood();
      prevTail = {-1,-1};
    }
    return true;
  }

  static void drawSnake(int score) {
    drawStatusBar("SNAKE", score, getHighScore(GameID::SNAKE));
    if (prevTail.x != -1)
      FillRect(prevTail.x*GRID_SIZE, prevTail.y*GRID_SIZE + STATUS_BAR_HEIGHT, GRID_SIZE, GRID_SIZE, false);
    FillRect(snake[0].x*GRID_SIZE, snake[0].y*GRID_SIZE + STATUS_BAR_HEIGHT, GRID_SIZE, GRID_SIZE, true);
    FillRect(food.x*GRID_SIZE,  food.y*GRID_SIZE  + STATUS_BAR_HEIGHT, GRID_SIZE, GRID_SIZE, true);
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
    return true; // <-- do NOT advance game while holding
  } else {
    exitHoldStart = 0;
  }

  // Turn handling (rate-limited)
  static bool turnedThisFrame = false;
  static int  safe = 2;
  if (!turnedThisFrame && safe <= 0) {
    if (in.buttonA) { Dir nd = (Dir)((dir + 3) % 4); if (isValid(nd, dir)) { dir = nd; turnedThisFrame = true; } }
    else if (in.buttonB) { Dir nd = (Dir)((dir + 1) % 4); if (isValid(nd, dir)) { dir = nd; turnedThisFrame = true; } }
  }

  int score = snakeLen - INITIAL_SNAKE_LENGTH;

  if (Millis() - lastMoveTime > MOVE_DELAY_MS) {
    if (!moveSnake()) { gameOver = true; outScore = score; return true; }
    lastMoveTime = Millis();
    safe--; turnedThisFrame = false;
  }

  drawSnake(score);
  outScore = score;
  gameOver = false;
  return true;
}
