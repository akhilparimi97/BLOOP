#include "bloop_entry.h"
#include "GameManager.h"
#include "platform.h"

void bloop_setup() {
  Platform::Init();        // <— HW: init display & pins; Web: no-op
  Platform::ClearDisplay();
  initGameManager();
}

void bloop_loop() {
  runGameLoop();
}
