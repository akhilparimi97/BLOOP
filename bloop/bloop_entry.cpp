#include "bloop_entry.h"
#include "GameManager.h"
// Remove "platform.h" include - it's included by GameManager.h

void bloop_setup() {
  Platform::Init();
  Platform::ClearDisplay();
  initGameManager();
}

void bloop_loop() {
  runGameLoop();
}