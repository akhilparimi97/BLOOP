#ifndef CONFIG_H
#define CONFIG_H

// Game settings
#define GAME_TITLE "BLOOP"
#define GAME_VERSION "1.0"

// Display settings
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Game constants
#define MAX_OBSTACLES 8
#define PLAYER_WIDTH 4
#define PLAYER_HEIGHT 6
#define OBSTACLE_WIDTH 3
#define OBSTACLE_HEIGHT 8
#define GAME_SPEED 100 // milliseconds between updates

#endif
