// src/bloop_adapter.h
#pragma once
#include "bridge.h"

// Map Arduino-like functions to bridge

// Button IDs (web: 0=Left, 1=Right)
#define BUTTON_A 0
#define BUTTON_B 1

// millis() substitute
static inline unsigned long millis() {
    return (unsigned long)bridgeNow();
}

// digitalRead substitute (LOW = pressed, HIGH = released)
static inline int digitalRead(int pin) {
    return bridgeButtonPressed(pin) ? 0 : 1;
}

// Display shim
struct DisplayShim {
    void clearDisplay() { bridgeClearDisplay(); }
    void display() { bridgeDisplay(); }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        bridgeDrawPixel(x, y, color != 0);
    }
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
        for (int i = 0; i < w; i++) drawPixel(x + i, y, color);
    }
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
        for (int j = 0; j < h; j++) drawPixel(x, y + j, color);
    }
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        drawFastHLine(x, y, w, color);
        drawFastHLine(x, y + h - 1, w, color);
        drawFastVLine(x, y, h, color);
        drawFastVLine(x + w - 1, y, h, color);
    }
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        for (int j = 0; j < h; j++) drawFastHLine(x, y + j, w, color);
    }
};
