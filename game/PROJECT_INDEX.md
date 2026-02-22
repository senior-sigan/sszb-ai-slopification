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
│   ├── command_server.h        # STB-style header-only TCP command server
│   ├── game_types.h            # All game constants, enums, structs, function declarations
│   ├── game.c                  # Core game logic: sprite anim, room helpers, difficulty, house layout
│   ├── assets.h                # Asset loading/unloading declarations
│   ├── assets.c                # Asset loading/unloading (textures, sounds, fonts, animations)
│   ├── night.h                 # Night phase declarations (enter/update/render/exit)
│   ├── night.c                 # Night phase implementation (tower defense gameplay)
│   ├── day.h                   # Day phase declarations (enter/update/render/exit)
│   └── day.c                   # Day phase implementation (shopping/upgrade screen)
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

Game engine foundation with types, assets, core logic, night phase, and day phase fully implemented.

### src/main.c (~220 lines)
- Window: 1366x768 ("Save Soul of Zlaya Babka"), 60 FPS
- `srand(time(NULL))` called after InitWindow for random number seeding
- TCP command server on port 9999
- `ManagedIsKeyPressed` / `ManagedIsMouseButtonPressed` — combine physical input + TCP commands (non-static, extern-linked by night.c and day.c)
- Full 9-state game state machine: LOGO -> MENU -> TUTOR1/2/3 -> NIGHT <-> DAY -> WIN/OVER
- State transition handlers: enter/exit callbacks for NIGHT and DAY phases; STATE_OVER enter handler is intentionally empty (reset happens on ENTER press)
- `state_update` — per-state update logic dispatching
- `state_render` — per-state rendering dispatching
- Audio device initialization before asset loading
- Proper cleanup sequence: assets, command server, audio device, window

### src/command_server.h (210 lines)
- STB-style header-only library (`#define COMMAND_SERVER_IMPLEMENTATION`)
- Non-blocking TCP server on localhost
- Commands: `SCREENSHOT <file>`, `KEY_PRESS <code>`, `MOUSE_PRESS <button>`, `MOVE_MOUSE <x> <y>`, `QUIT`
- Line-based protocol with `OK\n` / `ERROR <msg>\n` responses

### src/game_types.h (218 lines)
- All game constants (screen, building grid, entity limits, timing, prices, physics thresholds)
- Coordinate conversion macros: `FLIP_Y(y,h)`, `ROOM_GDX_X(col)`, `ROOM_GDX_Y(row)`
- Enums: GameState, RoomType, CreatureType, WeightType, AnimType
- Structs: Room, Creature, Weight (with hit_l0/hit_l1 one-shot collision flags), Bullet, AnimEffect, CashSprite, SpriteAnim, GameAssets, Game
- Function declarations for sprite animation, room pricing, difficulty scaling, house init

### src/game.c (163 lines)
- `sprite_anim_frame/duration/setup` — sprite sheet animation helpers
- `room_cooldown_time/repair_price/buy_price/weapon_price/grate_price` — room economics
- `difficulty_*` — per-level enemy speed, cooldowns, spawn patterns
- `game_init_house` — 3x6 room layout from legacy Scala RenderFactory
- `game_reset` — full game state reset

### src/assets.h / assets.c (163 lines)
- `assets_load` — loads all textures, sprite sheets, animations, fonts, music, SFX
- `assets_unload` — cleans up all loaded resources
- 51 textures, 9 sprite animations, 1 font, 3 music streams, 8 sound effects

### src/day.h / day.c (~310 lines)
- **day_enter**: play round_end sound, start birds music, reset frame animation
- **day_update**: shopping phase input handling:
  - Arrow keys: navigate rooms (day selection rule: room bought OR adjacent to bought)
  - KEY_ONE: buy room or repair broken room
  - KEY_TWO: install grate (if room bought, not broken, no grate)
  - KEY_THREE: install weapon (if room bought, not broken, not armed)
  - KEY_FOUR: buy club (win condition, costs 400)
  - ENTER: advance to next night round
  - Music stream update for birds BGM
  - State transitions: ENTER → NIGHT, club_bought → WIN
- **day_render**: 6-layer render pipeline:
  1. Background (bg_day)  2. Club building (club_day)
  3. Room loop: in-window weapons, lights (light_on/light_day), window frames (day variants)
  4. Money display  5. Animated selection frame  6. Shop UI (buy/repair, grate, weapon, club buttons with green/red price text)
- **day_exit**: stop birds music, increment level

### src/night.h / night.c (~510 lines)
- **night_enter**: reset hits/timers/entities, start crickets music
- **night_update**: full tower defense game loop:
  - Player input (arrow keys, SPACE fire, cheats L/P/ESC)
  - AI spawning with difficulty-scaled timers
  - Creature AI (hooligan shoots bullets, whore does selfie)
  - Bullet physics (atan2 homing, grate/window damage)
  - Weight physics (falling, AABB collision on two lanes, kill/reward)
  - Room cooldowns, loose control (creatures reaching club = hits)
  - Selfie flash effect, animation/cash cleanup
  - State transitions: night won → DAY, 5 hits → OVER
- **night_render**: 15-layer render pipeline with libGDX→raylib coordinate conversion:
  1. Background  2. Club building  3. Babka in window  4. In-window weapons
  5. Lights  6. Window frames  7. Babka hands up + weapon  8. Cash sprites
  9. Creatures (two lanes)  10. Death/crash anims  11. Falling weights
  12. Bullets  13. Frame selector  14. HUD (hits bar, money, time, level)
  15. Selfie flash overlay
- **night_exit**: stop music, clear all entities

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

## Game Design Document

**[docs/GAME_DESIGN.md](docs/GAME_DESIGN.md)** — Complete game design document (in Russian) covering all gameplay features: night/day phases, difficulty system, economy, room layout, weapons, enemies, win/lose conditions, and cheat codes.

## Architecture Document

**[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** — Complete C/Raylib architecture blueprint:
- All source files (.h/.c) with module responsibilities
- Every struct definition (Room, Creature, Bullet, Weight, CrashAnim, CashPickup, SpriteSheet, Assets, Game)
- State machine (LOGO→MENU→TUTORIAL→NIGHT→DAY→GAMEOVER/WIN)
- Function signatures for night.c, day.c, assets.c, game.c
- Raylib coordinate system conversions from libgdx (Y-up → Y-down)
- Animation frame sequences for all sprite sheets
- Entity management pattern (fixed arrays + alive flag + compact)
- 6 implementation milestones in dependency order

## What Has Been Ported

1. **Game types & constants** — all structs, enums, macros (game_types.h)
2. **Core game logic** — sprite animation, room economics, difficulty scaling, house layout (game.c)
3. **Asset pipeline** — full load/unload of all textures, animations, fonts, audio (assets.c)
4. **Night phase** — complete tower defense gameplay: AI, spawning, combat, rendering, audio (night.c)
5. **Day phase** — room shopping/upgrade: navigation, buy/repair/grate/weapon/club, shop UI rendering (day.c)

## What Still Needs Porting

All major systems have been ported. The game is fully playable with complete state machine integration.
