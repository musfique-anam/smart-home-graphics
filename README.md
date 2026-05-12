# 🏠 Smart Home Automation System

> A real-time 2D smart-home visualization and simulation built in **C++ / OpenGL (GLUT)** as a Computer Graphics course project.

![Language](https://img.shields.io/badge/language-C%2B%2B-blue)
![Graphics](https://img.shields.io/badge/graphics-OpenGL%20%2F%20GLUT-orange)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-lightgrey)
![Status](https://img.shields.io/badge/status-active-success)

---

## 📖 Overview

This project models a four-room house with fully interactive appliances, dynamic lighting, weather, energy monitoring, and a **keyboard-controlled occupant** that triggers automatic lighting as he moves between rooms [file:20].

The scene is rendered in an orthographic 2D viewport and updated at **~60 FPS** via a GLUT timer callback [file:20]. Every device exposes both **manual control** (mouse + keyboard) and **automatic behavior** driven by an internal simulation loop [file:20].

---

## ✨ Features

### 🏘️ House & Rooms
- Four labeled rooms: **Living Room**, **Bedroom**, **Kitchen**, **Bathroom** [file:20]
- Roof with decorative tiles, chimney with animated smoke particles [file:20]
- Front/garage doors with smooth open/close interpolation [file:20]
- Front-facing windows that glow when their room's light is on [file:20]

### 💡 Lighting
- Independent on/off control per room [file:20]
- Brightness slider per room (range `0.2 – 1.0`) [file:20]
- Soft radial glow, bulb filament, and emitted light rays via alpha blending [file:20]

### 🔌 Appliances
| Device | Behavior |
|--------|----------|
| **Ceiling Fans** (Kitchen, Bathroom) | Variable speed 1–5, continuous rotation [file:20] |
| **Air Conditioner** | Temp slider 16–30 °C, animated airflow, drift toward set point [file:20] |
| **Television** | Animated colour-bar pattern with moving scanline [file:20] |
| **Security Alarm** | Pulsing glow + animated clapper [file:20] |

### 🌦️ Environment
- **Day / Night** toggle with sky-gradient interpolation, sun/moon, flickering stars [file:20]
- **Weather cycle**: Sunny → Cloudy → Rainy [file:20]
- Drifting sinusoidal clouds and per-drop rain velocity [file:20]

### ⚡ Energy Dashboard
- Live wattage computation across all active devices [file:20]
- Colour-graded power bar (🟢 green → 🟡 yellow → 🔴 red) [file:20]
- Active-device counter and simulated real-time clock [file:20]

### 🚶 Keyboard-Controlled Person *(new module)*
- Occupant enters through the front door on demand [file:20]
- Walks to the chosen room and **auto-turns the light on** [file:20]
- Lights he auto-turned-on switch off when he leaves the room or exits [file:20]
- Walk cycle animated with sine-based limb swing: `θ(t) = A·sin(ωt)` [file:20]

### 👥 Auto-Interacting Crowd *(optional)*
- Up to **8 autonomous agents** can be spawned [file:20]
- Each walks to random appliances and toggles them [file:20]
- Pairs of agents pause to wave when they meet [file:20]

### 🖥️ UI
- Right-hand control panel with hover-highlighted buttons and on/off badges [file:20]
- Interactive sliders for brightness, fan speed, and AC temperature [file:20]
- Top status bar (title, clock, device count, energy bar, weather) [file:20]
- Bottom keyboard-hint strip [file:20]

---

## 🎮 Controls

### 🖱️ Mouse
- **Click** any sidebar button → toggle device, cycle weather, or trigger ALL ON / ALL OFF [file:20]
- **Click + Drag** any slider → adjust brightness, fan speed, or AC temperature [file:20]

### ⌨️ Keyboard

| Key | Action |
|:---:|--------|
| `1` – `4` | Toggle Living / Bedroom / Kitchen / Bathroom light [file:20] |
| `5` – `6` | Toggle Kitchen / Bathroom fan [file:20] |
| `7` – `8` | Toggle Front / Garage door [file:20] |
| `9` | Toggle Air Conditioner [file:20] |
| `T` | Toggle Television [file:20] |
| `A` | Toggle Security Alarm [file:20] |
| `D` / `N` | Day / Night mode [file:20] |
| `W` | Cycle weather [file:20] |
| `0` / `-` | Master ALL ON / ALL OFF [file:20] |
| `I` | Person enters through front door [file:20] |
| `L` / `B` / `K` / `H` | Send person to Living / Bedroom / Kitchen / Bathroom [file:20] |
| `X` | Person exits through front door [file:20] |
| `F` | Toggle fullscreen [file:20] |
| `Q` | Quit [file:20] |

---

## 🛠️ Build & Run

### Dependencies
- C++ compiler with **C++11** support (GCC ≥ 5 or Clang ≥ 3.4) [file:20]
- **OpenGL** development headers
- **FreeGLUT** (or original GLUT)

### 🐧 Linux (Debian / Ubuntu / Pop!\_OS)
```bash
sudo apt install build-essential freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev
g++ smart_home_pro.cpp -o smart_home_pro -lGL -lGLU -lglut
./smart_home_pro
```

### 🪟 Windows (MinGW)
```bash
g++ smart_home_pro.cpp -o smart_home_pro.exe -lfreeglut -lopengl32 -lglu32
```

### 🍎 macOS
```bash
g++ smart_home_pro.cpp -o smart_home_pro -framework OpenGL -framework GLUT
```

---

## 📁 Project Structure
├── smart_home_pro.cpp # Single-file source (~1500 LOC)
└── README.md


The source is organized top-to-bottom by responsibility:
globals → primitives → device renderers → house renderer
→ person module → UI panels → display/timer/input callbacks → main

[file:20]

---

## 🏗️ Architecture Notes

- **Rendering loop**: `glutTimerFunc(16, onTimer, 0)` drives a 60 FPS update; `display()` issues all draw calls in immediate mode under `gluOrtho2D` [file:20]
- **State separation**: All device states are plain global variables, allowing both UI and the autonomous person to mutate them without coupling [file:20]
- **Auto-light bookkeeping**: A parallel `autoLight[4]` flag remembers which lights the person turned on, ensuring user-toggled lights remain untouched on exit [file:20]
- **Fullscreen mouse fix**: `glutReshapeFunc` records current window size, and the mouse handler rescales `(mx, my)` back into the logical `1100 × 740` coordinate space [file:20]

---

## 🎨 Computer-Graphics Concepts Demonstrated

- ✅ 2D primitive rasterization (`GL_TRIANGLE_FAN`, `GL_QUADS`, `GL_LINES`, `GL_LINE_LOOP`) [file:20]
- ✅ Alpha blending for glows, shadows, and weather effects [file:20]
- ✅ Hierarchical transforms via `glPushMatrix` / `glPopMatrix` patterns [file:20]
- ✅ Procedural animation with parametric trig curves [file:20]
- ✅ Particle systems (chimney smoke, rain) with per-particle lifetime and velocity [file:20]
- ✅ Orthographic projection and viewport handling for resizable/fullscreen windows [file:20]
- ✅ Event-driven input integrated with continuous animation [file:20]

---

## 🚀 Future Work

- [ ] Wall-aware pathfinding so the occupant uses doorways rather than walking through internal walls
- [ ] Persistent room-occupancy memory to trigger fans/AC alongside lights
- [ ] Multi-person scheduling with collision avoidance using social-force vectors
- [ ] Migration to modern OpenGL (VAOs/VBOs + shaders) for hardware-accelerated batching

---

## 👨‍💻 Author

Developed as a **6th-semester Computer Graphics course project** [file:20].

---

## 📜 License

Released for **academic and educational use**. Attribution appreciated when reused or adapted.