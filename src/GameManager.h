#pragma once
#include "platform.h"
#include <cstdint>

enum class GameID : uint8_t { SNAKE = 0, PONG = 1, COUNT = 2 };

struct InputState {
  bool buttonA = false;
  bool buttonB = false;
  bool both    = false;
};

void initGameManager();
void runGameLoop(); // call every frame

// UI helpers used by games
void drawStatusBar(const char* gameName, int currentScore, int highScore);
void drawStatusBarMenu();
void showExitHoldBar(float progress);
void clearPlayfield();
void showGameOver(const char* gameName, int score, int highScore);
void showGetReady(const char* gameName, const char* instructions = nullptr);

// High scores (kept in RAM for web; hook real storage later)
int  getHighScore(GameID id);
void considerHighScore(GameID id, int score);

// Input helpers
InputState getInputState();

// Launchers called by manager
void startSnake();
void startPong();
bool stepSnake(int& outScore, bool& exitRequested, bool& gameOver); // non-blocking
bool stepPong (int& outScore, bool& exitRequested, bool& gameOver); // non-blocking
