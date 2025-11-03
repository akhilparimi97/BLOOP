// bloop/bloop.ino - Main Arduino sketch file
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Create display instance
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Include game code from same directory
#include "bloop_entry.h"

void setup() {
  bloop_setup();
}

void loop() {
  bloop_loop();
}