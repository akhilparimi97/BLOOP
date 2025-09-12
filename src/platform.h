#pragma once
#include <cstdint>
#include <cstddef>

namespace Platform {
  // Screen
  static constexpr int SCREEN_WIDTH  = 128;
  static constexpr int SCREEN_HEIGHT = 64;
  static constexpr int STATUS_BAR_HEIGHT = 16;
  static constexpr int PLAYFIELD_HEIGHT  = SCREEN_HEIGHT - STATUS_BAR_HEIGHT;

  // Inputs: A=0, B=1 for Web; on Arduino you can map to pins later.
  enum Button : int { BTN_A = 0, BTN_B = 1 };

  // Time/Input
  bool          ButtonPressed(Button b);        // true while held
  unsigned long Millis();                       // ms since start
  void          Delay(unsigned ms);             // non-critical sleep (web = blocking)

  // Random
  int           RandomInt(int min_inclusive, int max_exclusive);

  // Display (monochrome)
  void ClearDisplay();
  void Present();
  void DrawPixel(int x, int y, bool on=true);
  void DrawRect(int x, int y, int w, int h, bool on=true);
  void FillRect(int x, int y, int w, int h, bool on=true);
  void DrawLine(int x0, int y0, int x1, int y1, bool on=true);

  // Text (5x7 bitmap font, integer scale)
  void DrawText(int x, int y, const char* text, int scale=1, bool on=true);
}
