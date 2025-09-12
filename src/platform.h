#pragma once
#include <cstdint>
#include <cstddef>

namespace Platform {
  // Screen
  static constexpr int SCREEN_WIDTH  = 128;
  static constexpr int SCREEN_HEIGHT = 64;
  static constexpr int STATUS_BAR_HEIGHT = 16;
  static constexpr int PLAYFIELD_HEIGHT  = SCREEN_HEIGHT - STATUS_BAR_HEIGHT;

  // Inputs
  enum Button : int { BTN_A = 0, BTN_B = 1 };

  // Time/Input
  bool          ButtonPressed(Button b);
  unsigned long Millis();
  void          Delay(unsigned ms);

  // >>> Speed scaling (web uses >1.0 to slow gameplay to retro vibe)
  float         SpeedScale();   // e.g., 1.8 on web, 1.0 on Arduino

  // Random
  int           RandomInt(int min_inclusive, int max_exclusive);

  // Display (monochrome)
  void ClearDisplay();
  void Present();
  void DrawPixel(int x, int y, bool on=true);
  void DrawRect(int x, int y, int w, int h, bool on=true);
  void FillRect(int x, int y, int w, int h, bool on=true);
  void DrawLine(int x0, int y0, int x1, int y1, bool on=true);

  // Text (5x7), integer scale
  void DrawText(int x, int y, const char* text, int scale=1, bool on=true);

  // Persistent storage (web: localStorage)
  bool StorageGet(const char* key, int& outVal);
  void StorageSet(const char* key, int value);
}
