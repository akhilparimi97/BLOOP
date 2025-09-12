#ifndef GAME_H
#define GAME_H

#include "bridge.h"
#include "config.h"

// Game state
enum GameState {
  MENU,
  PLAYING,
  GAME_OVER,
  PAUSED
};

// Player structure
struct Player {
  int16_t x, y;
  int16_t width, height;
  bool alive;
};

// Obstacle structure  
struct Obstacle {
  int16_t x, y;
  int16_t width, height;
  bool active;
};

// Game class
class Game {
public:
  Game();
  void setup();
  void loop();
  void reset();
  void update();
  void render();
  void handleInput();
  
private:
  GameState state;
  Player player;
  Obstacle obstacles[MAX_OBSTACLES];
  unsigned long lastUpdate;
  unsigned long score;
  bool leftPressed, rightPressed;
  
  void updatePlayer();
  void updateObstacles();
  void checkCollisions();
  void spawnObstacle();
  void drawPlayer();
  void drawObstacles();
  void drawUI();
  void drawMenu();
  void drawGameOver();
};

// Global game instance
extern Game game;

// Entry points
void gameSetup();
void gameLoop();

#endif
