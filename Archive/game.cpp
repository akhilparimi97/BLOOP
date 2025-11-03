#include "game.h"

Game game;

Game::Game() {
  state = MENU;
  score = 0;
  lastUpdate = 0;
  leftPressed = false;
  rightPressed = false;
}

void Game::setup() {
#ifdef ARDUINO
  // Initialize display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for(;;); // Don't proceed, loop forever
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Initialize buttons
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
#endif
  
  reset();
}

void Game::reset() {
  // Initialize player
  player.x = SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2;
  player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - 2;
  player.width = PLAYER_WIDTH;
  player.height = PLAYER_HEIGHT;
  player.alive = true;
  
  // Clear obstacles
  for(int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i].active = false;
  }
  
  score = 0;
  lastUpdate = bridgeNow();
  state = MENU;
}

void Game::loop() {
  handleInput();
  
  unsigned long now = bridgeNow();
  if(now - lastUpdate >= GAME_SPEED) {
    update();
    render();
    lastUpdate = now;
  }
}

void Game::handleInput() {
  bool leftNow = bridgeButtonPressed(BUTTON_LEFT);
  bool rightNow = bridgeButtonPressed(BUTTON_RIGHT);
  
  // Detect button press (not hold)
  bool leftPress = leftNow && !leftPressed;
  bool rightPress = rightNow && !rightPressed;
  
  leftPressed = leftNow;
  rightPressed = rightNow;
  
  switch(state) {
    case MENU:
      if(leftPress || rightPress) {
        state = PLAYING;
        reset();
        state = PLAYING;
      }
      break;
      
    case PLAYING:
      if(leftPress && player.x > 0) {
        player.x -= 8;
      }
      if(rightPress && player.x < SCREEN_WIDTH - PLAYER_WIDTH) {
        player.x += 8;
      }
      if(leftPress && rightPress) {
        state = PAUSED;
      }
      break;
      
    case GAME_OVER:
      if(leftPress || rightPress) {
        reset();
      }
      break;
      
    case PAUSED:
      if(leftPress || rightPress) {
        state = PLAYING;
      }
      break;
  }
}

void Game::update() {
  if(state != PLAYING) return;
  
  updatePlayer();
  updateObstacles();
  checkCollisions();
  
  // Spawn new obstacles
  if(bridgeNow() % 1000 < GAME_SPEED) { // Roughly every second
    spawnObstacle();
  }
  
  score++;
}

void Game::updatePlayer() {
  // Keep player on screen
  if(player.x < 0) player.x = 0;
  if(player.x > SCREEN_WIDTH - PLAYER_WIDTH) {
    player.x = SCREEN_WIDTH - PLAYER_WIDTH;
  }
}

void Game::updateObstacles() {
  for(int i = 0; i < MAX_OBSTACLES; i++) {
    if(obstacles[i].active) {
      obstacles[i].y += 2;
      
      // Remove obstacles that are off screen
      if(obstacles[i].y > SCREEN_HEIGHT) {
        obstacles[i].active = false;
      }
    }
  }
}

void Game::spawnObstacle() {
  // Find inactive obstacle slot
  for(int i = 0; i < MAX_OBSTACLES; i++) {
    if(!obstacles[i].active) {
      obstacles[i].x = (bridgeNow() % (SCREEN_WIDTH - OBSTACLE_WIDTH));
      obstacles[i].y = -OBSTACLE_HEIGHT;
      obstacles[i].width = OBSTACLE_WIDTH;
      obstacles[i].height = OBSTACLE_HEIGHT;
      obstacles[i].active = true;
      break;
    }
  }
}

void Game::checkCollisions() {
  for(int i = 0; i < MAX_OBSTACLES; i++) {
    if(obstacles[i].active) {
      // Simple rectangle collision
      if(player.x < obstacles[i].x + obstacles[i].width &&
         player.x + player.width > obstacles[i].x &&
         player.y < obstacles[i].y + obstacles[i].height &&
         player.y + player.height > obstacles[i].y) {
        state = GAME_OVER;
        player.alive = false;
      }
    }
  }
}

void Game::render() {
  bridgeClearDisplay();
  
  switch(state) {
    case MENU:
      drawMenu();
      break;
    case PLAYING:
    case PAUSED:
      drawPlayer();
      drawObstacles();
      drawUI();
      if(state == PAUSED) {
        // Draw pause indicator
        bridgeDrawPixel(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, true);
        bridgeDrawPixel(SCREEN_WIDTH/2+1, SCREEN_HEIGHT/2, true);
        bridgeDrawPixel(SCREEN_WIDTH/2+3, SCREEN_HEIGHT/2, true);
        bridgeDrawPixel(SCREEN_WIDTH/2+4, SCREEN_HEIGHT/2, true);
      }
      break;
    case GAME_OVER:
      drawGameOver();
      break;
  }
  
  bridgeDisplay();
}

void Game::drawPlayer() {
  // Draw simple player rectangle
  for(int x = 0; x < player.width; x++) {
    for(int y = 0; y < player.height; y++) {
      bridgeDrawPixel(player.x + x, player.y + y, true);
    }
  }
}

void Game::drawObstacles() {
  for(int i = 0; i < MAX_OBSTACLES; i++) {
    if(obstacles[i].active) {
      for(int x = 0; x < obstacles[i].width; x++) {
        for(int y = 0; y < obstacles[i].height; y++) {
          bridgeDrawPixel(obstacles[i].x + x, obstacles[i].y + y, true);
        }
      }
    }
  }
}

void Game::drawUI() {
  // Draw score (simple dots)
  int dots = (score / 100) % 20; // Show progress with dots
  for(int i = 0; i < dots; i++) {
    bridgeDrawPixel(i * 6 + 2, 2, true);
  }
}

void Game::drawMenu() {
  // Draw simple menu - title and start instruction
  // BLOOP title (simplified pixel art)
  bridgeDrawPixel(20, 20, true); bridgeDrawPixel(21, 20, true); bridgeDrawPixel(22, 20, true);
  bridgeDrawPixel(20, 21, true); bridgeDrawPixel(22, 21, true);
  bridgeDrawPixel(20, 22, true); bridgeDrawPixel(21, 22, true); bridgeDrawPixel(22, 22, true);
  bridgeDrawPixel(20, 23, true); bridgeDrawPixel(22, 23, true);
  bridgeDrawPixel(20, 24, true); bridgeDrawPixel(21, 24, true); bridgeDrawPixel(22, 24, true);
  
  // Press button message (dots)
  for(int i = 0; i < 8; i++) {
    bridgeDrawPixel(40 + i * 4, 40, true);
  }
}

void Game::drawGameOver() {
  // Draw X pattern for game over
  for(int i = 0; i < 10; i++) {
    bridgeDrawPixel(30 + i, 25 + i, true);
    bridgeDrawPixel(30 + i, 35 - i, true);
    bridgeDrawPixel(70 + i, 25 + i, true);  
    bridgeDrawPixel(70 + i, 35 - i, true);
  }
}

// Entry points
void gameSetup() {
  game.setup();
}

void gameLoop() {
  game.loop();
}
