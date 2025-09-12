#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#ifndef __EMSCRIPTEN__
#include <Adafruit_SSD1306.h>
#define BUTTON_A 5
#define BUTTON_B 6
#else
#include "bloop_adapter.h"   // brings in DisplayShim + BUTTON_A/B redefs
#endif

// Constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define STATUS_BAR_HEIGHT 16
#define PLAYFIELD_HEIGHT (SCREEN_HEIGHT - STATUS_BAR_HEIGHT)
#define OLED_RESET -1
#define OLED_ADDR 0x3C
#define NUM_GAMES 2
#define DEBOUNCE_DELAY 200
#define EXIT_HOLD_DURATION 1500

// Game IDs for high score indexing
enum GameID {
    GAME_SNAKE = 0,
    GAME_PONG = 1
};

// Input helper structure
struct InputState {
    bool buttonA;
    bool buttonB;
    bool bothPressed;
    bool exitRequested;
};

#ifndef __EMSCRIPTEN__
extern Adafruit_SSD1306 display;
#else
extern DisplayShim display;
#endif

extern int highScores[NUM_GAMES];
extern bool justWoke;

// Core functions
void initGameManager();
void runGameLoop();

// UI functions
void drawStatusBar(const char* gameName, int currentScore, int highScore);
void drawStatusBarMenu();
void showExitHoldBar(float progress);

// Input functions  
InputState getInputState();
void waitForButtonRelease();
bool buttonPressed(int pin);

// Utility functions
void clearPlayfield();
void showGameOver(const char* gameName, int score, int highScore);
void showGetReady(const char* gameName, const char* instructions = nullptr);

#endif
