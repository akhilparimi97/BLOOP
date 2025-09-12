#include "SnakeGame.h"
#include "GameManager.h"
#include <Arduino.h>

// Game constants
#define GRID_SIZE 4
#define GRID_HEIGHT (PLAYFIELD_HEIGHT / GRID_SIZE)
#define GRID_WIDTH (SCREEN_WIDTH / GRID_SIZE)
#define MAX_SNAKE_LENGTH 64
#define MOVE_DELAY 300
#define INITIAL_SNAKE_LENGTH 3
static unsigned long exitHoldStart = 0;
        

// Game state
enum Direction { RIGHT, DOWN, LEFT, UP };
struct Point { int x, y; };

static Point snake[MAX_SNAKE_LENGTH];
static int snakeLength;
static Direction dir;
static Point food;
static unsigned long lastMoveTime;
static bool turnMade;
static int safeSteps;
static Point prevTail = {-1, -1};

// ============ HELPER FUNCTIONS ============

static bool isValidDirection(Direction newDir, Direction currentDir) {
    return !((newDir == UP && currentDir == DOWN) ||
             (newDir == DOWN && currentDir == UP) ||
             (newDir == LEFT && currentDir == RIGHT) ||
             (newDir == RIGHT && currentDir == LEFT));
}

static void placeFood() {
    bool validPosition;
    do {
        validPosition = true;
        food.x = random(0, GRID_WIDTH);
        food.y = random(0, GRID_HEIGHT);
        
        // Check if food spawns on snake
        for (int i = 0; i < snakeLength; i++) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                validPosition = false;
                break;
            }
        }
    } while (!validPosition);
}

static void resetSnake() {
    snakeLength = INITIAL_SNAKE_LENGTH;
    snake[0] = {GRID_WIDTH/2, GRID_HEIGHT/2};
    snake[1] = {GRID_WIDTH/2 - 1, GRID_HEIGHT/2};
    snake[2] = {GRID_WIDTH/2 - 2, GRID_HEIGHT/2};
    dir = RIGHT;
    placeFood();
    safeSteps = 2;
    turnMade = false;
    prevTail = {-1, -1};
}

// ============ GAME LOGIC ============

static bool moveSnake() {
    // Store tail for erasing
    prevTail = snake[snakeLength - 1];
    
    // Move body
    for (int i = snakeLength - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }

    // Move head
    switch(dir) {
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

    // Check self collision
    for (int i = 1; i < snakeLength; i++) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
            return false;
        }
    }

    // Check food collision
    if (snake[0].x == food.x && snake[0].y == food.y) {
        snakeLength++;
        placeFood();
        // Don't erase tail this frame since snake grew
        prevTail = {-1, -1};
    }

    return true;
}

// ============ RENDERING ============

static void drawSnake(int currentScore) {
    drawStatusBar("SNAKE", currentScore, highScores[GAME_SNAKE]);

    // Erase previous tail
    if (prevTail.x != -1) {
        display.fillRect(prevTail.x * GRID_SIZE,
                         prevTail.y * GRID_SIZE + STATUS_BAR_HEIGHT,
                         GRID_SIZE, GRID_SIZE, SSD1306_BLACK);
    }

    // Draw snake head
    display.fillRect(snake[0].x * GRID_SIZE,
                     snake[0].y * GRID_SIZE + STATUS_BAR_HEIGHT,
                     GRID_SIZE, GRID_SIZE, SSD1306_WHITE);

    // Draw food
    display.fillRect(food.x * GRID_SIZE,
                     food.y * GRID_SIZE + STATUS_BAR_HEIGHT,
                     GRID_SIZE, GRID_SIZE, SSD1306_WHITE);

    display.display();
}

static void showGetReady() {
    clearPlayfield();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(35, STATUS_BAR_HEIGHT + (PLAYFIELD_HEIGHT / 2) - 5);
    display.println("Get Ready...");
    display.display();
    delay(1000);
    clearPlayfield();
    display.display();
}

// ============ INPUT HANDLING ============

static bool handleInput() {
    InputState input = getInputState();
    
    // Exit check
    if (input.bothPressed) {
        
        if (exitHoldStart == 0) {
            exitHoldStart = millis();
        } else {
            float progress = (float)(millis() - exitHoldStart) / EXIT_HOLD_DURATION;
            if (progress >= 1.0f) {
                waitForButtonRelease();
                return false; // Exit game
            }
            showExitHoldBar(progress);
        }
        delay(50);
        return true;
    } else {
        // Reset exit timer if buttons released
        //static unsigned long exitHoldStart = 0;
        exitHoldStart = 0;
    }

    // Direction changes (only when safe and no turn made this frame)
    if (!turnMade && safeSteps <= 0) {
        if (buttonPressed(BUTTON_A)) {
            Direction newDir = (Direction)((dir + 3) % 4); // Counter-clockwise
            if (isValidDirection(newDir, dir)) {
                dir = newDir;
                turnMade = true;
            }
        } else if (buttonPressed(BUTTON_B)) {
            Direction newDir = (Direction)((dir + 1) % 4); // Clockwise
            if (isValidDirection(newDir, dir)) {
                dir = newDir;
                turnMade = true;
            }
        }
    }
    
    return true;
}

// ============ MAIN GAME FUNCTION ============

int playSnake() {
    resetSnake();
    waitForButtonRelease();
    showGetReady("SNAKE", "A: Left, B: Right");

    lastMoveTime = millis();
    exitHoldStart = 0; // Reset exit timer
    
    while (true) {
        int currentScore = snakeLength - INITIAL_SNAKE_LENGTH;
        drawSnake(currentScore);

        // Handle input (returns false if exit requested)
        if (!handleInput()) {
            return currentScore;
        }

        // Move snake
        if (millis() - lastMoveTime > MOVE_DELAY) {
            if (!moveSnake()) {
                showGameOver("SNAKE", currentScore, highScores[GAME_SNAKE]);
                return currentScore;
            }
            lastMoveTime = millis();
            safeSteps--;
            turnMade = false;
        }

        delay(1);
    }
}