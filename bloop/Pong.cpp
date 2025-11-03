#include "Pong.h"
#include "platform.h"
#include <cstdlib>
#include <algorithm> 

using namespace Platform;

namespace {
  constexpr int PADDLE_HEIGHT = 10;
  constexpr int PADDLE_WIDTH  = 2;
  constexpr int BALL_SIZE     = 2;
  constexpr int PADDLE_OFFSET = 3;
  constexpr int PADDLE_SPEED_BASE  = 3;     // Increased from 2 for better responsiveness
  constexpr unsigned TICK_MS_BASE  = 25;    // Increased from 20ms for smoother gameplay

  struct Paddle { int x,y; };
  struct Ball { int x,y, vx, vy; };

  static Paddle player, cpu;
  static Ball   ball;
  static int    playerScore = 0;
  static bool   gameActive  = false;

  static unsigned long lastTick = 0;
  static unsigned long exitHoldStart = 0;
  static bool          inited = false;

  // Enhanced input handling
  static bool prevAPressed = false;
  static bool prevBPressed = false;
  static unsigned long lastInputTime = 0;
  static constexpr unsigned INPUT_COOLDOWN_MS = 50;  // Faster input response for Pong

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
    ball.vx  = 0; 
    ball.vy = 0;
    playerScore = 0;
    gameActive  = false;
    lastTick    = Millis();
    lastInputTime = 0;
    prevAPressed = prevBPressed = false;
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
    int speed = std::max(1, std::abs(ball.vx));  // Ensure minimum speed
    
    if (bc < cc) {
      cpu.y -= speed;
    } else if (bc > cc) {
      cpu.y += speed;
    }
    
    // Constrain CPU paddle
    if (cpu.y < STATUS_BAR_HEIGHT) {
      cpu.y = STATUS_BAR_HEIGHT;
    }
    if (cpu.y > SCREEN_HEIGHT - PADDLE_HEIGHT) {
      cpu.y = SCREEN_HEIGHT - PADDLE_HEIGHT;
    }
  }

  static bool updateBall() {
    if (!gameActive) return true;
    ball.x += ball.vx; 
    ball.y += ball.vy;

    // Bounce off top/bottom
    if (ball.y <= STATUS_BAR_HEIGHT || ball.y >= SCREEN_HEIGHT - BALL_SIZE) {
      ball.vy = -ball.vy;
    }

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
    
    // Ball out of bounds
    if (ball.x > SCREEN_WIDTH || ball.x < 0) {
      return false;
    }
    return true;
  }

  static void drawDashedCourt() {
    const int dash=2, gap=2;
    // Top and bottom borders
    for (int x=0; x<SCREEN_WIDTH; x+=dash+gap) {
      for (int i=0; i<dash && x+i<SCREEN_WIDTH; ++i) {
        DrawPixel(x+i, STATUS_BAR_HEIGHT, true); 
        DrawPixel(x+i, SCREEN_HEIGHT-1, true); 
      }
    }
    // Side borders
    for (int y=STATUS_BAR_HEIGHT; y<SCREEN_HEIGHT; y+=dash+gap) {
      for (int i=0; i<dash && y+i<SCREEN_HEIGHT; ++i) {
        DrawPixel(0, y+i, true); 
        DrawPixel(SCREEN_WIDTH-1, y+i, true); 
      }
    }
    // Center line
    for (int y = STATUS_BAR_HEIGHT; y < SCREEN_HEIGHT; y += 4) {
      DrawPixel(SCREEN_WIDTH/2, y, true);
    }
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

  // Enhanced paddle movement with debouncing
  unsigned long now = Millis();
  bool canProcessInput = (now - lastInputTime) > INPUT_COOLDOWN_MS;
  
  if (canProcessInput) {
    bool currentAPressed = in.buttonA;
    bool currentBPressed = in.buttonB;
    
    // Calculate paddle speed with scaling
    int paddleSpeed = std::max(1, (int)(PADDLE_SPEED_BASE / SpeedScale()));
    bool moved = false;
    
    // Continuous movement while button is held
    if (currentAPressed && player.y > STATUS_BAR_HEIGHT + 2) { 
      player.y -= paddleSpeed; 
      moved = true; 
      lastInputTime = now;
    }
    if (currentBPressed && player.y < SCREEN_HEIGHT - PADDLE_HEIGHT - 2) { 
      player.y += paddleSpeed; 
      moved = true; 
      lastInputTime = now;
    }
    
    // Serve ball on first movement
    if (!gameActive && moved) {
      serveBall();
    }
    
    // Update previous state
    prevAPressed = currentAPressed;
    prevBPressed = currentBPressed;
  }

  // Tick update with scaling
  const unsigned tickMs = (unsigned)(TICK_MS_BASE * SpeedScale());
  if (Millis() - lastTick >= tickMs) {
    updateCPU();
    if (!updateBall()) { 
      gameOver = true; 
      outScore = playerScore;
      // Reset input states
      prevAPressed = prevBPressed = false;
      return true; 
    }
    lastTick = Millis();
  }

  drawGame();
  outScore = playerScore;
  gameOver = false;
  return true;
}