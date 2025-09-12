#include "bloop_entry.h"
#include "GameManager.h"
#include "platform.h"

void bloop_setup() {
  Platform::ClearDisplay();
  initGameManager();
}

void bloop_loop() {
  runGameLoop();
  // The loop draws/presents as needed each frame
}
