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
void runGameLoop();

// UI helpers
void drawStatusBar(const char* gameName, int currentScore, int highScore);
void drawStatusBarMenu();
void showExitHoldBar(float progress);
void clearPlayfield();
void showGameOver(const char* gameName, int score, int highScore);
void showGetReady(const char* gameName, const char* instructions = nullptr);

// High scores
int  getHighScore(GameID id);
void considerHighScore(GameID id, int score);

// Input
InputState getInputState();
bool getButtonAPressed();  // Edge-triggered button press
bool getButtonBPressed();  // Edge-triggered button press

// Game stepping
void startSnake();
void startPong();
bool stepSnake(int& outScore, bool& exitRequested, bool& gameOver);
bool stepPong (int& outScore, bool& exitRequested, bool& gameOver);