# Project Index: Save Soul of Zlaya Babka (SSZB)

Generated: 2026-02-22

## Overview

Rewriting a Scala/libgdx pixel-art game "Save Soul of Zlaya Babka" into C/Raylib. The game is about an angry grandma ("Zlaya Babka") who defends her apartment building from rowdy youngsters.

**Do NOT compile the Scala legacy code. Only compile the C Raylib application.**

## Project Structure

```
game/
├── src/                        # C Raylib source (active codebase)
│   ├── main.c                  # Entry point, game loop, TCP command handling
│   └── command_server.h        # STB-style header-only TCP command server
├── legacy/                     # Original Scala/libgdx code (reference only)
│   ├── sszb/                   # Game-specific code
│   │   ├── SaveSoulOfZlayaBabka.scala  # Main game class, state routing
│   │   ├── NightState.scala    # Night phase gameplay (defend from enemies)
│   │   ├── DayState.scala      # Day phase gameplay (buy/repair/upgrade rooms)
│   │   ├── Shared.scala        # Shared game state between phases
│   │   ├── Assets.scala        # All texture/font/sound asset loading
│   │   ├── common/Const.scala  # Game constants, difficulty, UI layout
│   │   ├── entity/             # Game entities (Rooms, Creatures, Bullets, Weights)
│   │   └── units/              # Game systems (AI, Control, View, Rendering)
│   └── lib/                    # Engine library (state machine, animation, utils)
├── assets/                     # Game assets (shared between legacy and new)
│   ├── textures/               # ~60 sprites (gif/png): windows, characters, UI, backgrounds
│   ├── sound/                  # ~10 sound effects + 3 background music tracks (mp3)
│   └── font/                   # 2 TTF fonts (impact, main)
├── build/                      # CMake build output
├── CMakeLists.txt              # Build config: C23, raylib 5.5, strict warnings
├── Makefile                    # Convenience: make build, make run, make clean
└── CLAUDE.md                   # Project instructions
```

## Entry Points

- **C Application**: `src/main.c` — Raylib game loop with TCP command server on port 9999
- **Build**: `make build` (CMake + make) or `cmake -Bbuild && cmake --build build -j 4`
- **Run**: `make run` or `./build/Game`

## C Raylib Application (Current State)

The C version is a **minimal prototype** — a movable green circle with mouse crosshair and TCP command support. The actual game logic from the Scala version has NOT been ported yet.

### src/main.c (110 lines)
- Window: 640x480, 60 FPS
- Player: green circle, arrow-key movement
- Mouse crosshair and click marker
- HUD: FPS, player position, mouse position, last TCP command
- TCP commands integrated via `ManagedIsKeyPressed` / `ManagedIsMouseButtonPressed`

### src/command_server.h (210 lines)
- STB-style header-only library (`#define COMMAND_SERVER_IMPLEMENTATION`)
- Non-blocking TCP server on localhost
- Commands: `SCREENSHOT <file>`, `KEY_PRESS <code>`, `MOUSE_PRESS <button>`, `MOVE_MOUSE <x> <y>`, `QUIT`
- Line-based protocol with `OK\n` / `ERROR <msg>\n` responses

## Game Design (from Legacy Scala Code)

### Game Flow
```
Logo → Menu → Tutorial(1-3) → [Night → Day]* → GameWin/GameOver → restart
```

### Night Phase (defend, 30 seconds per level)
- Babka sits in window of apartment building (3x6 grid of rooms)
- Enemies approach: Hooligans (throw bottles, break windows) and Whores (take selfies, blind babka)
- Babka drops items from windows: Pot (flower), TV, Royal Piano
- Arrow keys to move between rooms, Space to attack
- Club nearby plays loud music — damages babka
- Lose condition: 5 hits

### Day Phase (shop/upgrade)
- Arrow keys to select room in building grid
- `1` — Buy or repair room
- `2` — Buy armor (grate) for room
- `3` — Weaponize room (assign drop item)
- `4` — Buy club (win condition, costs 400)
- Win condition: buy the club

### Key Constants
- Screen: 1366x768 (legacy), 1366x768 (current C prototype)
- Building: 3 rows x 6 columns of rooms
- Room types: PotRoom, TVRoom, RoyalRoom (different weapons/costs)
- Start money: 10, Club price: 400
- Level time: 30 seconds

### Assets Summary

See **[docs/ASSET_INDEX.md](docs/ASSET_INDEX.md)** for the full detailed asset index with descriptions.

- **Textures** (51 files): backgrounds (day/night), window states (normal/broken/grate/disabled × day/night), weapon sprites + crash animations (pot/tv/piano), character sprite sheets (babka, gopstop 7fr, whore 7fr), UI elements (HUD 3-layer, toolbar buttons, selection frame), screens (menu, gameover, gamewin, 3 tutorials)
- **Sounds** (11 files): bgm_cool (53s), crickets (60s), birds (60s) — music/ambient; pot_destroy, tv_destroy, selfie, iron, bye, round_end, window_broken, royal_deploy — SFX
- **Fonts** (2 files): impact.ttf (133K, bold display), main.ttf (238K, primary UI)

## Raylib API Reference

Split reference docs in `docs/raylib/api/` — see `docs/raylib/api/INDEX.md` for full lookup table.

| File | Contents |
|------|----------|
| `01_defines.txt` | Macros, color constants |
| `02_structs.txt` | All data types (Vector2/3/4, Color, Rectangle, Image, Texture, Font, Camera, Mesh, Model, Sound, Music...) |
| `03_enums.txt` | ConfigFlags, KeyboardKey, MouseButton, GamepadButton, PixelFormat, BlendMode, CameraMode... |
| `04_core_window.txt` | Window, Drawing loop, Shaders, Screen-space, Timing, Files, Compression |
| `05_input.txt` | Keyboard, Mouse, Gamepad, Touch, Gestures, Camera update |
| `06_shapes.txt` | 2D primitives, Splines, 2D Collision detection |
| `07_textures.txt` | Image load/gen/manipulate, Texture load/draw, Color utilities |
| `08_text.txt` | Font load, Text drawing/measurement, Codepoints, String utils |
| `09_models.txt` | 3D shapes, Model/Mesh/Material, Animations, 3D Collision |
| `10_audio.txt` | Audio device, Wave/Sound, Music stream, AudioStream, Processors |

## Build Configuration

- **CMake**: C23 standard, raylib 5.5, strict warnings (`-Wall -Werror -Wextra -pedantic`)
- **Platform**: macOS (Apple frameworks: IOKit, Cocoa, OpenGL)
- **Dependency**: raylib (must be installed, found via `find_package`)

## Quick Start

1. Install raylib 5.5 (e.g. `brew install raylib`)
2. `make build` — configure and build
3. `make run` — run the game
4. Optional: send TCP commands to port 9999 (e.g. `echo "KEY_PRESS 262\n" | nc localhost 9999`)

## What Needs Porting

The Scala codebase contains the full game logic that needs to be reimplemented in C/Raylib:

1. **State machine** — game flow routing (logo → tutorial → night → day → win/lose)
2. **Night phase** — enemy AI, spawning, movement, combat, item dropping
3. **Day phase** — room selection, buying, repairing, upgrading, shop UI
4. **Entity system** — rooms (3 types), creatures (2 types), bullets, weights, animations
5. **Rendering** — building grid, window states, character animations, HUD, backgrounds
6. **Audio** — background music per phase, sound effects for actions
7. **Difficulty scaling** — enemy speed/cooldown/spawn rates per level
