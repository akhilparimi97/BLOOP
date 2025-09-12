#pragma once
#include "bridge.h"
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>

// Arduino-like constants
#define LOW 0
#define HIGH 1

inline void delay(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

using String = std::string;

#define SSD1306_WHITE 1
#define SSD1306_BLACK 0

// Stub Adafruit_SSD1306 replacement for web builds
class Adafruit_SSD1306 {
public:
    Adafruit_SSD1306(int w, int h, void* wire, int rst) {
        (void)w; (void)h; (void)wire; (void)rst;
    }

    bool begin(int vccstate, int addr) {
        (void)vccstate; (void)addr;
        return true;
    }

    void clearDisplay() { bridgeClearDisplay(); }
    void display() { bridgeDisplay(); }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        bridgeDrawPixel(x, y, color != 0);
    }

    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        for (int i = 0; i < w; i++) {
            bridgeDrawPixel(x + i, y, color != 0);
            bridgeDrawPixel(x + i, y + h - 1, color != 0);
        }
        for (int j = 0; j < h; j++) {
            bridgeDrawPixel(x, y + j, color != 0);
            bridgeDrawPixel(x + w - 1, y + j, color != 0);
        }
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++)
                bridgeDrawPixel(x + i, y + j, color != 0);
    }

    void drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        while (true) {
            bridgeDrawPixel(x0, y0, color != 0);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    // Text API stubs (can later be mapped to canvas text)
    void setTextSize(int s) { (void)s; }
    void setTextColor(uint16_t c) { (void)c; }
    void setCursor(int16_t x, int16_t y) { cursorX = x; cursorY = y; }
    void print(const String& s) { std::cout << s; }
    void print(int n) { std::cout << n; }
    void println(const String& s) { std::cout << s << "\n"; }
    void println(int n) { std::cout << n << "\n"; }

private:
    int cursorX = 0;
    int cursorY = 0;
};
