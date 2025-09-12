#include "GameManager.h"
#include "SnakeGame.h"
#include "Pong.h"
#include <Wire.h>
#include <EEPROM.h>
#include "esp_sleep.h"

// Global variables
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int highScores[NUM_GAMES];
bool justWoke = false;

// Static variables
static enum SystemState { STATE_LOCKED, STATE_MENU } currentState = STATE_LOCKED;
static uint8_t menuIndex = 0;
static String menuItems[] = {"1.Snake", "2.Pong", "3.Sleep"};
static const int menuCount = sizeof(menuItems) / sizeof(menuItems[0]);
static unsigned long lastPressA = 0;
static unsigned long lastPressB = 0;

// ============ INPUT HANDLING ============

bool buttonPressed(int pin) {
    unsigned long now = millis();
    unsigned long *lastPress = (pin == BUTTON_A) ? &lastPressA : &lastPressB;
    
    if (digitalRead(pin) == LOW && now - *lastPress > DEBOUNCE_DELAY) {
        *lastPress = now;
        return true;
    }
    return false;
}

InputState getInputState() {
    InputState state = {0};
    state.buttonA = digitalRead(BUTTON_A) == LOW;
    state.buttonB = digitalRead(BUTTON_B) == LOW;
    state.bothPressed = state.buttonA && state.buttonB;
    return state;
}

void waitForButtonRelease() {
    while (digitalRead(BUTTON_A) == LOW || digitalRead(BUTTON_B) == LOW) {
        delay(10);
    }
}

// ============ UI FUNCTIONS ============

void showBootAnimation() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    for (int i = 0; i < SCREEN_WIDTH; i += 4) {
        display.clearDisplay();
        display.setCursor(i, 25);
        display.println("BLOOP");
        display.display();
        delay(20);
    }
    delay(500);
}

void showLockedScreen() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(30, 25);
    display.println("BLOOP");
    display.display();
}

void drawStatusBar(const char* gameName, int currentScore, int highScore) {
    display.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 2);
    display.print("HSC:");
    display.print(highScore);

    display.setCursor(50, 2);
    display.print("Scr:");
    display.print(currentScore);

    display.setCursor(110, 2);
    display.print("BAT");
}

void drawStatusBarMenu() {
    display.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(48, 2);
    display.print("BLOOP");

    display.setCursor(110, 2);
    display.print("BAT");
    display.display();
}

void showExitHoldBar(float progress) {
    int barWidth = SCREEN_WIDTH * progress;
    display.fillRect(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH, 2, SSD1306_BLACK);
    display.drawLine(0, SCREEN_HEIGHT - 1, barWidth, SCREEN_HEIGHT - 1, SSD1306_WHITE);
    display.display();
}

void clearPlayfield() {
    display.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, PLAYFIELD_HEIGHT, SSD1306_BLACK);
}

void showGameOver(const char* gameName, int score, int highScore) {
    clearPlayfield();
    drawStatusBar(gameName, score, highScore);
    
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(30, STATUS_BAR_HEIGHT + 5);
    display.println("Game Over");
    display.setCursor(20, STATUS_BAR_HEIGHT + 20);
    display.print("Score: ");
    display.println(score);
    display.setCursor(20, STATUS_BAR_HEIGHT + 35);
    display.print("HighScore: ");
    display.println(highScore);
    display.display();
    delay(1500);
}

void showGetReady(const char* gameName, const char* instructions) {
    clearPlayfield();
    drawStatusBar(gameName, 0, (gameName == "SNAKE") ? highScores[GAME_SNAKE] : highScores[GAME_PONG]);
    
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Main "Get Ready" text
    display.setCursor(35, STATUS_BAR_HEIGHT + 15);
    display.println("Get Ready...");
    
    // Optional instructions
    if (instructions != nullptr) {
        display.setCursor(15, STATUS_BAR_HEIGHT + 30);
        display.println(instructions);
    }
    
    display.display();
    delay(2000);
    clearPlayfield();
    display.display();
}

// ============ MENU SYSTEM ============

void showMenu() {
    display.clearDisplay();
    drawStatusBarMenu();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    for (int i = 0; i < menuCount; i++) {
        display.setCursor(0, STATUS_BAR_HEIGHT + 5 + i * 10);
        display.print((i == menuIndex) ? "> " : "  ");
        display.println(menuItems[i]);
    }
    display.display();
}

// ============ SLEEP MODE ============

void sleepMode() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 25);
    display.println("Sleeping...");
    display.display();
    delay(500);

    display.ssd1306_command(SSD1306_DISPLAYOFF);

    gpio_wakeup_enable((gpio_num_t)BUTTON_A, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable((gpio_num_t)BUTTON_B, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    esp_light_sleep_start();

    // Wake up sequence
    display.ssd1306_command(SSD1306_DISPLAYON);
    justWoke = true;
    waitForButtonRelease();

    showBootAnimation();
    menuIndex = 0;
    showMenu();
}

// ============ HIGH SCORE MANAGEMENT ============

void saveHighScore(GameID gameId, int score) {
    if (score > highScores[gameId]) {
        highScores[gameId] = score;
        EEPROM.put(0, highScores);  // Always save entire array at offset 0
        EEPROM.commit();
    }
}

void loadHighScores() {
    EEPROM.get(0, highScores);
    // Sanity check
    for (int i = 0; i < NUM_GAMES; i++) {
        if (highScores[i] < 0 || highScores[i] > 9999) {
            highScores[i] = 0;
        }
    }
    EEPROM.put(0, highScores);
    EEPROM.commit();
}

// ============ GAME LAUNCHING ============

void launchGame(const String& gameName) {
    waitForButtonRelease();
    int score = 0;
    
    if (gameName == "1.Snake") {
        score = playSnake();
        saveHighScore(GAME_SNAKE, score);
    } else if (gameName == "2.Pong") {
        score = playPong();
        saveHighScore(GAME_PONG, score);
    }
    
    showMenu();
}

// ============ INITIALIZATION ============

void initGameManager() {
    Wire.begin(2, 3); // SDA, SCL
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        while (true); // Halt on display init failure
    }

    EEPROM.begin(32);
    loadHighScores();
    showBootAnimation();
}

// ============ MAIN LOOP ============

void runGameLoop() {
    if (currentState == STATE_LOCKED) {
        if (buttonPressed(BUTTON_A) || buttonPressed(BUTTON_B)) {
            currentState = STATE_MENU;
            menuIndex = 0;
            showMenu();
        }
    } else if (currentState == STATE_MENU) {
        if (buttonPressed(BUTTON_B)) {
            menuIndex = (menuIndex + 1) % menuCount;
            showMenu();
            waitForButtonRelease();
        }
        
        if (buttonPressed(BUTTON_A)) {
            if (menuItems[menuIndex] == "3.Sleep") {
                sleepMode();
            } else {
                launchGame(menuItems[menuIndex]);
            }
        }
    }
}