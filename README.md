# BLOOP - Dual-Target Arduino Game

![BLOOP Logo](https://img.shields.io/badge/BLOOP-Arduino%20Game-brightgreen?style=for-the-badge)
![Build Status](https://img.shields.io/github/actions/workflow/status/akhilparimi97/BLOOP/build.yml?branch=main&style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-Arduino%20%7C%20Web-blue?style=for-the-badge)

# 🎮 BLOOP — Keychain sized Game Console
BLOOP is a **tiny retro handheld game console** built using an **ESP32-C3**, a **0.96" OLED display**, and **two buttons**.  
It runs the same game both on hardware and in a web browser.
I’m open-sourcing it so others can build their own or help improve it — hardware, code, design, anything.  
I’m not an expert in electronics or programming — most of this was built with the help of AI (including this page) and experimentation.

## 🎮Web Version

**🌐 [Play BLOOP in your browser!](https://akhilparimi97.github.io/BLOOP/web/index.html)**

*The web version is automatically built and deployed from the latest code.*

## 🛠 Hardware Setup

### Required Components
- **ESP32-C3** microcontroller
- **SSD1306** 128×64 OLED display (I2C)
- **2 push buttons** (momentary switches)
- Breadboard and jumper wires

### Wiring Diagram
ESP32-C3        SSD1306 OLED

GPIO4    ──────  SDA
GPIO5    ──────  SCL
3.3V     ──────  VCC
GND      ──────  GND
ESP32-C3        Buttons

GPIO5    ──────  Button 1 (Left)  ──── GND
GPIO6    ──────  Button 2 (Right) ──── GND

## 🚀 Quick Start

### Arduino Hardware
1. **Install Libraries** (Arduino IDE → Library Manager):
   - `Adafruit SSD1306` by Adafruit
   - `Adafruit GFX Library` by Adafruit

2. **Upload Code**:
   - Open `arduino/bloop.ino` in Arduino IDE
   - Select your ESP32-C3 board
   - Choose correct COM port
   - Click Upload 🚀

3. **Play!**
   - Use the physical buttons to navigate/control.
     
## 🎯 Controls

| Platform | Left Move | Right Move | Start/Pause |
|----------|-----------|------------|-------------|
| **Arduino** | GPIO5 Button | GPIO6 Button | Both buttons |
| **Web** | ← Arrow Key | → Arrow Key | Both arrows |


## 🏗 Project Architecture

This project uses a **bridge abstraction pattern** to enable cross-platform compatibility:

```
bloop/
├── 📁 arduino/           # Arduino sketch entry point
│   └── bloop.ino        # Main Arduino file
├── 📁 src/              # Shared game logic
│   ├── bridge.h         # 🔧 Platform abstraction layer
│   ├── config.h         # Game configuration
│   ├── game.h          # Game class definition
│   └── game.cpp        # Core game implementation
├── 📁 web/             # Web/Emscripten build
│   ├── main.cpp        # Web entry point
│   ├── Makefile        # Build configuration
│   └── index.html      # Web interface
└── 📁 .github/workflows/
    └── build.yml       # Auto-deployment
```

## 🛠 Need to work!!
| Area          | Examples                                         |
|---------------|--------------------------------------------------|
| Firmware      | New games, menu system, better input handling    |
| Electronics   | Custom PCB, power management, smaller layout     |
| 3D Design     | Better case, buttons, keychain mounts            |
| Docs          | Build guides, diagrams, beginner-friendly setup  |

### Bridge Function Reference
| Function | Purpose | Arduino | Web |
|----------|---------|---------|-----|
| `bridgeDrawPixel(x,y,color)` | Draw pixel | `display.drawPixel()` | Canvas pixel |
| `bridgeDisplay()` | Update screen | `display.display()` | Canvas refresh |
| `bridgeClearDisplay()` | Clear screen | `display.clearDisplay()` | Clear canvas |
| `bridgeButtonPressed(pin)` | Read input | `digitalRead()` | Keyboard state |
| `bridgeNow()` | Get time | `millis()` | `performance.now()` |


### 🧾 Notes

This project is still evolving.  
Feel free to use it, break it, fix it, or make it your own version.

