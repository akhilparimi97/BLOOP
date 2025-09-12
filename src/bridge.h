#ifndef BRIDGE_H
#define BRIDGE_H

#ifdef ARDUINO
  #include <Arduino.h>
  #include <Adafruit_SSD1306.h>
  #include <Adafruit_GFX.h>
  
  extern Adafruit_SSD1306 display;
  
  inline void bridgeDrawPixel(int16_t x, int16_t y, bool color) {
    display.drawPixel(x, y, color ? SSD1306_WHITE : SSD1306_BLACK);
  }
  
  inline void bridgeDisplay() {
    display.display();
  }
  
  inline void bridgeClearDisplay() {
    display.clearDisplay();
  }
  
  inline bool bridgeButtonPressed(uint8_t pin) {
    return digitalRead(pin) == LOW; // Active low
  }
  
  inline unsigned long bridgeNow() {
    return millis();
  }
  
  inline void bridgeDelay(unsigned long ms) {
    delay(ms);
  }

#else // Web/Emscripten
  #include <emscripten.h>
  #include <emscripten/html5.h>
  
  // External JS functions
  extern "C" {
    void js_draw_pixel(int x, int y, int color);
    void js_display();
    void js_clear_display();
    int js_button_pressed(int pin);
    double js_now();
  }
  
  inline void bridgeDrawPixel(int16_t x, int16_t y, bool color) {
    js_draw_pixel(x, y, color ? 1 : 0);
  }
  
  inline void bridgeDisplay() {
    js_display();
  }
  
  inline void bridgeClearDisplay() {
    js_clear_display();
  }
  
  inline bool bridgeButtonPressed(uint8_t pin) {
    return js_button_pressed(pin) != 0;
  }
  
  inline unsigned long bridgeNow() {
    return (unsigned long)js_now();
  }
  
  inline void bridgeDelay(unsigned long ms) {
    // Non-blocking delay for web
    emscripten_sleep(ms);
  }

#endif

// Common constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUTTON_LEFT 5
#define BUTTON_RIGHT 6

#endif
