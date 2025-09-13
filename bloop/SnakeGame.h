#pragma once
#include "GameManager.h"

// Non-blocking per-frame snake
void startSnake();
bool stepSnake(int& outScore, bool& exitRequested, bool& gameOver);
