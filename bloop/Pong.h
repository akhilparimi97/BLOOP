#pragma once
#include "GameManager.h"

// Non-blocking per-frame pong
void startPong();
bool stepPong(int& outScore, bool& exitRequested, bool& gameOver);
