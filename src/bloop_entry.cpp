// src/bloop_entry.cpp
#include "bloop_entry.h"
#include "bloop_adapter.h"
#include "GameManager.h"

void bloop_setup() {
    initGameManager();
}

void bloop_loop() {
    runGameLoop();
    display.display(); // flush screen
}
