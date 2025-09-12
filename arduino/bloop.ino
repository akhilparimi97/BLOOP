#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Create display instance
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Include game code
#include "../src/game.h"

void setup() {
  Serial.begin(115200);
  gameSetup();
}

void loop() {
  gameLoop();
}
