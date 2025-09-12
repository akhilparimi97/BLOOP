#include "Pong.h"
#include "GameManager.h"
#include <Arduino.h>

// Game constants
#define PADDLE_HEIGHT 10
#define PADDLE_WIDTH 2
#define BALL_SIZE 2
#define PADDLE_OFFSET 3
#define PADDLE_SPEED 2
#define GAME_SPEED 15
static unsigned long exitHoldStart = 0;
        
// Game state
struct Paddle {
    int x, y;
};

struct Ball {
    int x, y;
    int velX, velY;
};

static Paddle player, cpu;
static Ball ball;
static int playerScore;
static bool gameActive;

// ============ GAME LOGIC ============

static void resetGame() {
    // Initialize paddles
    player.x = SCREEN_WIDTH - PADDLE_WIDTH - PADDLE_OFFSET;
    player.y = PLAYFIELD_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    
    cpu.x = PADDLE_OFFSET;
    cpu.y = PLAYFIELD_HEIGHT / 2 - PADDLE_HEIGHT / 2;

    // Initialize ball
    ball.x = SCREEN_WIDTH / 2;
    ball.y = PLAYFIELD_HEIGHT / 2;
    ball.velX = 0;
    ball.velY = 0;

    playerScore = 0;
    gameActive = false;
}

static void serveBall() {
    ball.velX = -1;  // Start toward CPU
    ball.velY = random(0, 2) ? 1 : -1;
    gameActive = true;
}

static void updateCPU() {
    if (!gameActive) return;
    
    int ballCenterY = ball.y + BALL_SIZE / 2;
    int cpuCenterY = cpu.y + PADDLE_HEIGHT / 2;
    int cpuSpeed = abs(ball.velX);
    
    if (ballCenterY < cpuCenterY) {
        cpu.y -= cpuSpeed;
    } else if (ballCenterY > cpuCenterY) {
        cpu.y += cpuSpeed;
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
    
    ball.x += ball.velX;
    ball.y += ball.velY;

    // Bounce off top/bottom
    if (ball.y <= STATUS_BAR_HEIGHT || ball.y >= SCREEN_HEIGHT - BALL_SIZE) {
        ball.velY = -ball.velY;
    }

    // CPU paddle collision
    if (ball.x <= cpu.x + PADDLE_WIDTH &&
        ball.y + BALL_SIZE >= cpu.y && 
        ball.y <= cpu.y + PADDLE_HEIGHT) {
        ball.x = cpu.x + PADDLE_WIDTH;
        ball.velX = -ball.velX;
    }

    // Player paddle collision
    if (ball.x + BALL_SIZE >= player.x &&
        ball.y + BALL_SIZE >= player.y && 
        ball.y <= player.y + PADDLE_HEIGHT) {
        ball.x = player.x - BALL_SIZE;
        ball.velX = -ball.velX;
        playerScore++;
        drawStatusBar("PONG", playerScore, highScores[GAME_PONG]);
    }

    // Ball out of bounds
    if (ball.x > SCREEN_WIDTH || ball.x < 0) {
        return false; // Game over
    }

    return true;
}

// ============ INPUT HANDLING ============

static bool handleInput() {
    InputState input = getInputState();
    bool moved = false;

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
        // Reset exit timer
        //static unsigned long exitHoldStart = 0;
        exitHoldStart = 0;
    }

    // Player movement
    if (input.buttonA && player.y > STATUS_BAR_HEIGHT + 2) {
        player.y -= PADDLE_SPEED;
        moved = true;
    }
    if (input.buttonB && player.y < SCREEN_HEIGHT - PADDLE_HEIGHT - 2) {
        player.y += PADDLE_SPEED;
        moved = true;
    }

    // Serve ball on first movement
    if (!gameActive && moved) {
        serveBall();
    }

    return true;
}

// ============ RENDERING ============

static void drawGame() {
    clearPlayfield();
    drawStatusBar("PONG", playerScore, highScores[GAME_PONG]);

    // Draw court boundary
    display.drawRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, PLAYFIELD_HEIGHT, SSD1306_WHITE);

    // Draw center line
    for (int y = STATUS_BAR_HEIGHT; y < SCREEN_HEIGHT; y += 4) {
        display.drawPixel(SCREEN_WIDTH / 2, y, SSD1306_WHITE);
    }

    // Draw paddles
    display.fillRect(cpu.x, cpu.y, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);
    display.fillRect(player.x, player.y, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);

    // Draw ball
    display.fillRect(ball.x, ball.y, BALL_SIZE, BALL_SIZE, SSD1306_WHITE);

    display.display();
}

static void showGetReady() {
    clearPlayfield();
    drawStatusBar("PONG", 0, highScores[GAME_PONG]);
    
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(25, STATUS_BAR_HEIGHT + (PLAYFIELD_HEIGHT / 2) - 5);
    display.println("Move to serve!");
    display.display();
}

// ============ MAIN GAME FUNCTION ============

int playPong() {
    resetGame();
    waitForButtonRelease();
    showGetReady("PONG", "A: Up, B: Down");

    exitHoldStart = 0; // Reset exit timer

    while (true) {
        // Handle input (returns false if exit requested)
        if (!handleInput()) {
            return playerScore;
        }

        // Update game objects
        updateCPU();
        if (!updateBall()) {
            showGameOver("PONG", playerScore, highScores[GAME_PONG]);
            return playerScore;
        }

        // Render
        drawGame();
        delay(GAME_SPEED);
    }
}