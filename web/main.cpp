#include <emscripten.h>
#include <emscripten/html5.h>
#include "../bloop/bloop_entry.h"

// Global game loop function
void main_loop() {
  bloop_loop();
}

int main() {
  bloop_setup();
  
  // Set up the main loop
  emscripten_set_main_loop(main_loop, 60, 1); // 60 FPS
  
  return 0;
}

// JavaScript interface functions
extern "C" {
  EMSCRIPTEN_KEEPALIVE
  void js_draw_pixel(int x, int y, int color) {
    EM_ASM({
      drawPixel($0, $1, $2);
    }, x, y, color);
  }
  
  EMSCRIPTEN_KEEPALIVE
  void js_display() {
    EM_ASM({
      updateDisplay();
    });
  }
  
  EMSCRIPTEN_KEEPALIVE
  void js_clear_display() {
    EM_ASM({
      clearDisplay();
    });
  }
  
  EMSCRIPTEN_KEEPALIVE
  int js_button_pressed(int pin) {
    return EM_ASM_INT({
      return isButtonPressed($0);
    }, pin);
  }
  
  EMSCRIPTEN_KEEPALIVE
  double js_now() {
    return EM_ASM_DOUBLE({
      return performance.now();
    });
  }
}
