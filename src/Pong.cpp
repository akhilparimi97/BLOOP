#include "Pong.h"
#include "platform.h"
#include <cstdlib>

using namespace Platform;

namespace {
  constexpr int PADDLE_HEIGHT = 10;
  constexpr int PADDLE_WIDTH  = 2;
  constexpr int BALL_SIZE     = 2;
  constexpr int PADDLE_OFFSET = 3;
  constexpr int PADDLE_SPEED  = 2;
  constexpr unsigned TICK_MS  = 15;

  struct Paddle { int x,y; };
  struct Ball { int x,y, vx, vy; };

  static Paddle player, cpu;
  static Ball   ball;
  static int    playerScore = 0;
  static bool   gameActive  = false;

  static unsigned long lastTick = 0;
  static unsigned long exitHoldStart = 0;
  static bool          inited = false;

  // Get Ready gating
  static unsigned long readyUntil = 0;
  static bool clearedAfterReady = false;

  static void resetGame() {
    player.x = SCREEN_WIDTH - PADDLE_WIDTH - PADDLE_OFFSET;
    player.y = STATUS_BAR_HEIGHT + PLAYFIELD_HEIGHT/2 - PADDLE_HEIGHT/2;
    cpu.x    = PADDLE_OFFSET;
    cpu.y    = STATUS_BAR_HEIGHT + PLAYFIELD_HEIGHT/2 - PADDLE_HEIGHT/2;
    ball.x   = SCREEN_WIDTH/2;
    ball.y   = STATUS_BAR_HEIGHT + PLAYFIELD_HEIGHT/2;
    ball.vx  = 0; ball.vy = 0;
    playerScore = 0;
    gameActive  = false;
    lastTick    = Millis();
  }

  static void serveBall() {
    ball.vx = -1;
    ball.vy = (RandomInt(0,2) == 0) ? 1 : -1;
    gameActive = true;
  }

  static void updateCPU() {
    if (!gameActive) return;
    int bc = ball.y + BALL_SIZE/2;
    int cc = cpu.y + PADDLE_HEIGHT/2;
    int speed = std::abs(ball.vx);
    if (bc < cc) cpu.y -= speed;
    else if (bc > cc) cpu.y += speed;
    if (cpu.y < STATUS_BAR_HEIGHT) cpu.y = STATUS_BAR_HEIGHT;
    if (cpu.y > SCREEN_HEIGHT - PADDLE_HEIGHT) cpu.y = SCREEN_HEIGHT - PADDLE_HEIGHT;
  }

  static bool updateBall() {
    if (!gameActive) return true;
    ball.x += ball.vx; ball.y += ball.vy;

    if (ball.y <= STATUS_BAR_HEIGHT || ball.y >= SCREEN_HEIGHT - BALL_SIZE) ball.vy = -ball.vy;

    // CPU collision
    if (ball.x <= cpu.x + PADDLE_WIDTH &&
        ball.y + BALL_SIZE >= cpu.y && ball.y <= cpu.y + PADDLE_HEIGHT) {
      ball.x = cpu.x + PADDLE_WIDTH;
      ball.vx = -ball.vx;
    }
    // Player collision
    if (ball.x + BALL_SIZE >= player.x &&
        ball.y + BALL_SIZE >= player.y && ball.y <= player.y + PADDLE_HEIGHT) {
      ball.x = player.x - BALL_SIZE;
      ball.vx = -ball.vx;
      playerScore++;
      drawStatusBar("PONG", playerScore, getHighScore(GameID::PONG));
    }
    if (ball.x > SCREEN_WIDTH || ball.x < 0) return false;
    return true;
  }

  static void drawDashedCourt() {
    // dashed rectangle border
    const int dash = 2, gap = 2;

    // top
    for (int x=0; x<SCREEN_WIDTH; x += dash+gap)
      for (int i=0; i<dash && x+i<SCREEN_WIDTH; ++i)
        DrawPixel(x+i, STATUS_BAR_HEIGHT, true);

    // bottom
    for (int x=0; x<SCREEN_WIDTH; x += dash+gap)
      for (int i=0; i<dash && x+i<SCREEN_WIDTH; ++i)
        DrawPixel(x+i, SCREEN_HEIGHT-1, true);

    // left
    for (int y=STATUS_BAR_HEIGHT; y<SCREEN_HEIGHT; y += dash+gap)
      for (int i=0; i<dash && y+i<SCREEN_HEIGHT; ++i)
        DrawPixel(0, y+i, true);

    // right
    for (int y=STATUS_BAR_HEIGHT; y<SCREEN_HEIGHT; y += dash+gap)
      for (int i=0; i<dash && y+i<SCREEN_HEIGHT; ++i)
        DrawPixel(SCREEN_WIDTH-1, y+i, true);

    // center dotted line
    for (int y = STATUS_BAR_HEIGHT; y < SCREEN_HEIGHT; y += 4)
      DrawPixel(SCREEN_WIDTH/2, y, true);
  }

  static void drawGame() {
    clearPlayfield();
    drawStatusBar("PONG", playerScore, getHighScore(GameID::PONG));
    drawDashedCourt();
    FillRect(cpu.x,    cpu.y,    PADDLE_WIDTH, PADDLE_HEIGHT, true);
    FillRect(player.x, player.y, PADDLE_WIDTH, PADDLE_HEIGHT, true);
    FillRect(ball.x,   ball.y,   BALL_SIZE,    BALL_SIZE,     true);
    Present();
  }

} // anon

void startPong() {
  inited = true;
  resetGame();
  showGetReady("PONG", "A: Up, B: Down");
  readyUntil = Millis() + 1000; // 1s
  clearedAfterReady = false;
  exitHoldStart = 0;
}

bool stepPong(int& outScore, bool& exitRequested, bool& gameOver) {
  if (!inited) startPong();

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
    return true; // <-- pause gameplay while holding
  } else {
    exitHoldStart = 0;
  }

  // Move player
  bool moved = false;
  if (in.buttonA && player.y > STATUS_BAR_HEIGHT + 2) { player.y -= PADDLE_SPEED; moved = true; }
  if (in.buttonB && player.y < SCREEN_HEIGHT - PADDLE_HEIGHT - 2) { player.y += PADDLE_SPEED; moved = true; }
  if (!gameActive && moved) serveBall();

  // Tick update
  if (Millis() - lastTick >= TICK_MS) {
    updateCPU();
    if (!updateBall()) { gameOver = true; outScore = playerScore; return true; }
    lastTick = Millis();
  }

  drawGame();
  outScore = playerScore;
  gameOver = false;
  return true;
}
