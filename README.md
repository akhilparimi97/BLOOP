# BLOOP - Dual-Target Arduino Game

![BLOOP Logo](https://img.shields.io/badge/BLOOP-Arduino%20Game-brightgreen?style=for-the-badge)
![Build Status](https://img.shields.io/github/actions/workflow/status/akhilparimi97/BLOOP/build.yml?branch=main&style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-Arduino%20%7C%20Web-blue?style=for-the-badge)

A retro-style obstacle-avoidance game that runs on both Arduino hardware and in web browsers using **the exact same game logic**. Experience the nostalgia of classic arcade games with modern cross-platform compatibility!

## 🎮 Live Demo

**🌐 [Play BLOOP in your browser!](https://akhilparimi97.github.io/BLOOP)**

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

*Internal pull-up resistors are enabled in software.*

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
   - Use the physical buttons to navigate
   - Avoid falling obstacles
   - Beat your high score!

### Web Emulator (Local Development)
```bash
# Install Emscripten (one-time setup)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build the game
cd bloop/web
make clean && make

# Serve locally (Python example)
python -m http.server 8000
# Visit http://localhost:8000
```
## 🎯 Game Controls

| Platform | Left Move | Right Move | Start/Pause |
|----------|-----------|------------|-------------|
| **Arduino** | GPIO5 Button | GPIO6 Button | Both buttons |
| **Web** | ← Arrow Key | → Arrow Key | Both arrows |

## 📋 Game Rules

- **Objective**: Survive as long as possible by avoiding falling obstacles
- **Movement**: Dodge left and right to avoid collisions
- **Scoring**: Points increase over time - challenge yourself!
- **Lives**: One hit and it's game over (classic arcade style)
- **Pause**: Press both controls simultaneously to pause
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

### Bridge Abstraction Magic ✨

The `bridge.h` file translates platform-specific calls:

```cpp
// Your game code uses universal functions:
bridgeDrawPixel(x, y, color);    // Works on both platforms
bridgeButtonPressed(BUTTON_LEFT); // Unified input handling
bridgeNow();                     // Cross-platform timing

// Arduino: Maps to Adafruit display & digitalRead
// Web: Maps to HTML5 canvas & keyboard events
```

## 🔄 Automated Deployment

Every push to the main branch triggers:
1. **🔨 Emscripten compilation** of C++ to WebAssembly  
2. **📦 Asset bundling** (HTML + JS + WASM)
3. **🚀 GitHub Pages deployment** for instant web access

No manual building required for the web version!

## 🛠 Development

### Adding Features
1. **Modify game logic** in `src/game.cpp`
2. **Use bridge functions** instead of platform-specific calls
3. **Test on Arduino** by uploading the sketch
4. **Test on web** by running `make` in `/web`
5. **Deploy** by pushing to GitHub (auto-builds)

### Bridge Function Reference
| Function | Purpose | Arduino | Web |
|----------|---------|---------|-----|
| `bridgeDrawPixel(x,y,color)` | Draw pixel | `display.drawPixel()` | Canvas pixel |
| `bridgeDisplay()` | Update screen | `display.display()` | Canvas refresh |
| `bridgeClearDisplay()` | Clear screen | `display.clearDisplay()` | Clear canvas |
| `bridgeButtonPressed(pin)` | Read input | `digitalRead()` | Keyboard state |
| `bridgeNow()` | Get time | `millis()` | `performance.now()` |

## 🎨 Customization

### Game Parameters (`src/config.h`)
```cpp
#define MAX_OBSTACLES 8        // Number of obstacles
#define PLAYER_WIDTH 4         // Player size
#define OBSTACLE_WIDTH 3       // Obstacle size  
#define GAME_SPEED 100        // Update interval (ms)
```

### Display Settings
- **Resolution**: 128×64 pixels (classic OLED size)
- **Style**: Monochrome retro graphics
- **Rendering**: Pixel-perfect scaling for web

