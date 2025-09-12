#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Create display instance
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Include game code
#include "src/bloop_entry.h"

void setup() {
  bloop_setup();
}

void loop() {
  bloop_loop();
  // No extra delay here; the game uses Millis()/Delay() internally for pacing.
}
