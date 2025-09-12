#ifdef ARDUINO

#include "platform.h"
#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_SSD1306.h>

// ---- CONFIG ----
static const int PIN_BTN_A = 5;
static const int PIN_BTN_B = 6;
static const int OLED_RESET = -1;
static const int OLED_ADDR  = 0x3C;

// Global display
static Adafruit_SSD1306 display(Platform::SCREEN_WIDTH, Platform::SCREEN_HEIGHT, &Wire, OLED_RESET);

namespace Platform {

  // EEPROM layout (8 bytes total)
  static const int EE_MAGIC_ADDR = 0;   // 2 bytes
  static const int EE_SNAKE_ADDR = 2;   // 4 bytes
  static const int EE_PONG_ADDR  = 6;   // 4 bytes (overlaps if MCU has 1k EEPROM adjust accordingly)
  // If MCU has smaller EEPROM adjust addresses/usage.

  void Init() {
    pinMode(PIN_BTN_A, INPUT_PULLUP);
    pinMode(PIN_BTN_B, INPUT_PULLUP);

    Wire.begin();
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    display.clearDisplay();
    display.display();

    // Mark/init EEPROM if magic not set (0xB7 0x10)
    uint8_t m0 = EEPROM.read(EE_MAGIC_ADDR);
    uint8_t m1 = EEPROM.read(EE_MAGIC_ADDR+1);
    if (!(m0 == 0xB7 && m1 == 0x10)) {
      EEPROM.write(EE_MAGIC_ADDR,   0xB7);
      EEPROM.write(EE_MAGIC_ADDR+1, 0x10);
      int zero = 0;
      EEPROM.put(EE_SNAKE_ADDR, zero);
      EEPROM.put(EE_PONG_ADDR,  zero);
    }
  }

  bool ButtonPressed(Button b) {
    int pin = (b == Button::BTN_A) ? PIN_BTN_A : PIN_BTN_B;
    return digitalRead(pin) == LOW; // active low
  }

  unsigned long Millis() { return ::millis(); }
  void          Delay(unsigned ms) { ::delay(ms); }

  float SpeedScale() { return 1.0f; }

  int RandomInt(int min_inclusive, int max_exclusive) {
    if (max_exclusive <= min_inclusive) return min_inclusive;
    long r = random((long)min_inclusive, (long)max_exclusive);
    return (int)r;
  }

  void ClearDisplay() { display.clearDisplay(); }
  void Present()      { display.display(); }

  void DrawPixel(int x, int y, bool on) { display.drawPixel(x, y, on ? SSD1306_WHITE : SSD1306_BLACK); }
  void DrawRect (int x, int y, int w, int h, bool on) { display.drawRect(x,y,w,h, on?SSD1306_WHITE:SSD1306_BLACK); }
  void FillRect (int x, int y, int w, int h, bool on) { display.fillRect(x,y,w,h, on?SSD1306_WHITE:SSD1306_BLACK); }
  void DrawLine (int x0,int y0,int x1,int y1,bool on){ display.drawLine(x0,y0,x1,y1, on?SSD1306_WHITE:SSD1306_BLACK); }

  void DrawText(int x, int y, const char* text, int scale, bool on) {
    display.setTextSize(scale <= 0 ? 1 : scale);
    display.setTextColor(on ? SSD1306_WHITE : SSD1306_BLACK);
    display.setCursor(x, y);
    // handle newlines manually since setCursor persists
    for (const char* p = text; *p; ++p) {
      if (*p == '\n') {
        y += 8 * (scale <= 0 ? 1 : scale);
        display.setCursor(x, y);
      } else {
        display.write(*p);
      }
    }
  }

  bool StorageGet(const char* key, int& outVal) {
    if (!key) return false;
    if (strcmp(key, "hs_snake") == 0) { EEPROM.get(EE_SNAKE_ADDR, outVal); return true; }
    if (strcmp(key, "hs_pong")  == 0) { EEPROM.get(EE_PONG_ADDR,  outVal); return true; }
    return false;
  }

  void StorageSet(const char* key, int value) {
    if (!key) return;
    if (strcmp(key, "hs_snake") == 0) EEPROM.put(EE_SNAKE_ADDR, value);
    if (strcmp(key, "hs_pong")  == 0) EEPROM.put(EE_PONG_ADDR,  value);
  }

} // namespace Platform

#endif // ARDUINO
