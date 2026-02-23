# Project Index: Save Soul of Zlaya Babka (SSZB)

Generated: 2026-02-23

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
├── vendor/raylib/              # Vendored raylib submodule (5.5)
├── build/                      # CMake native build output
├── cmake-build-web/            # CMake web/WASM build output (gitignored)
├── CMakeLists.txt              # Build config: C23, raylib, WASM support, ASan/UBSan
├── Makefile                    # Convenience: build, run, test, build_web, run_web, clean
├── tests/                      # Test scripts (.sszb) and runner
│   ├── run_test.sh             # Bash script: launch game, send SCRIPT command, check report
│   └── test_full_cycle.sszb    # Full cycle test: logo→menu→tutorials→night→day→gameover→restart
├── src/index.html              # Emscripten HTML shell for web builds
└── CLAUDE.md                   # Project instructions
```

## Entry Points

- **C Application**: `src/main.c` — Raylib game loop with TCP command server on port 9999
- **Native build**: `make build` then `make run`
- **Run tests**: `make test` (builds, launches game, runs `.sszb` test script, reports results)
- **Web build**: `make build_web` then `make run_web` (opens http://localhost:3000/Game.html)

## C Raylib Application (Current State)

Game engine foundation with types, assets, core logic, night phase, and day phase fully implemented.

### src/main.c (~340 lines)
- Window: 1366x768 ("Save Soul of Zlaya Babka"), 60 FPS
- `srand(time(NULL))` called after InitWindow for random number seeding
- TCP command server on port 9999 (disabled in WASM builds)
- `ManagedIsKeyPressed` / `ManagedIsMouseButtonPressed` — combine physical input + TCP commands (non-static, extern-linked by night.c and day.c); pure raylib input on web
- Full 9-state game state machine: LOGO -> MENU -> TUTOR1/2/3 -> NIGHT <-> DAY -> WIN/OVER
- State transition handlers: enter/exit callbacks for NIGHT and DAY phases; STATE_OVER enter handler is intentionally empty (reset happens on ENTER press)
- `update()` — extracted frame function for emscripten_set_main_loop compatibility
- `state_update` / `state_render` — per-state logic and rendering dispatching
- `#ifdef PLATFORM_WEB` guards for emscripten/WASM builds
- Audio device initialization before asset loading
- Proper cleanup sequence: assets, command server, audio device, window
- Script test support: `game_get_field_int()` for querying game state, `state_name()` for enum→string, ASSERT/GET/WAIT_STATE handling, `script_respond()` for TCP output

### src/command_server.h (~450 lines)
- STB-style header-only library (`#define COMMAND_SERVER_IMPLEMENTATION`)
- Non-blocking TCP server on localhost
- Single-command mode: `SCREENSHOT <file>`, `KEY_PRESS <code>`, `MOUSE_PRESS <button>`, `MOVE_MOUSE <x> <y>`, `QUIT`
- Script runner mode: `SCRIPT <file>` loads `.sszb` test script, executes line-per-frame
- ScriptRunner struct: line buffer, WAIT counter, WAIT_STATE target, pass/fail counters, report buffer
- Script commands: KEY, MOUSE, MOVE, SHOT, WAIT, WAIT_STATE, ASSERT_STATE, ASSERT_EQ/GE/LE, GET, LOG, QUIT
- Persistent TCP connection during script execution; sends report on completion

### src/game_types.h (218 lines)
- All game constants (screen, building grid, entity limits, timing, prices, physics thresholds)
- Coordinate conversion macros: `FLIP_Y(y,h)`, `ROOM_GDX_X(col)`, `ROOM_GDX_Y(row)`
- Enums: GameState, RoomType, CreatureType, WeightType, AnimType
- Structs: Room, Creature, Weight (with hit_l0/hit_l1 one-shot collision flags), Bullet, AnimEffect, CashSprite, SpriteAnim, GameAssets, Game
- Function declarations for sprite animation, room pricing, difficulty scaling, house init

### src/game.c (~276 lines)
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

### src/night.h / night.c (~600 lines)
- Functions use CamelCase naming (NightEnter, NightUpdate, NightRender, NightExit)
- Logic decomposed into ~20 static helper functions to keep cognitive complexity low
- **NightEnter**: reset hits/timers/entities, start crickets music
- **NightUpdate**: orchestrates subsystem updates:
  - UpdateInputHandling — arrow keys, SPACE fire, cheats L/P/ESC
  - UpdateFireWeapon — spawn falling weights from armed rooms
  - SpawnCreatures — AI spawning with difficulty-scaled timers
  - UpdateCreatures / UpdateCreatureAttack — creature AI (hooligan shoots bullets, whore does selfie)
  - UpdateBullets — atan2 homing, grate/window damage
  - UpdateWeights / CheckWeightCollision* / WeightHitGround — falling, AABB collision on two lanes, kill/reward
  - UpdateRoomCooldowns, UpdateLooseControl, UpdateSelfieFlash, UpdateAnimations
  - State transitions: night won -> DAY, 5 hits -> OVER
- **NightRender**: delegates to 13 render helpers for 15-layer pipeline:
  RenderClubBuilding, RenderBabkaInWindow, RenderInWindowWeapons, RenderLights,
  RenderWindowFrames, RenderBabkaHandsUp, RenderCashSprites, RenderCreatures,
  RenderDeathAnimations, RenderFallingWeights, RenderBullets, RenderFrameSelector,
  RenderHud, RenderSelfieFlash
- **NightExit**: stop music, clear all entities

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

- **CMake**: C23 standard, vendored raylib, strict warnings (`-Wall -Werror -Wextra -pedantic`)
- **Native**: macOS (Apple frameworks: IOKit, Cocoa, OpenGL), ASan/UBSan enabled
- **Web/WASM**: Emscripten with `--embed-file assets/`, ASYNCIFY, GLFW3
- **Dependency**: raylib vendored as git submodule in `vendor/raylib/`

## Quick Start

### Native
1. `make build` — configure and build
2. `make run` — run the game
3. Optional: send TCP commands to port 9999 (e.g. `echo "KEY_PRESS 262" | nc localhost 9999`)

### Web (WASM)
1. Install Emscripten (`brew install emscripten`)
2. `make build_web` — configure and build for WASM
3. `make run_web` — start HTTP server on port 3000
4. Open http://localhost:3000/Game.html in browser

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

## Balance Verification

All game balance parameters have been audited against the original Scala code. Key fixes applied:
- Spawn probability: ~50% of spawns now correctly produce dual-lane enemies (matching Scala's signed-int RNG behavior)
- Selfie flash: 0.5s delay before activation (matching Scala's deferred callback)
- Creature movement: enemies keep walking during attack animations (matching Scala's unconditional movement)
- BGM Cool: pause/resume instead of stop/restart (preserving playback position)
- Club purchase: C correctly subtracts money (`-= CLUB_PRICE`), fixing an original Scala bug (`= CLUB_PRICE`)

## Automated Testing

Frame-based test scripting system using `.sszb` files. See `docs/TEST_SYSTEM_DESIGN.md` for full design.

- `SCRIPT <file>` TCP command loads a script and executes it line-per-frame inside the game loop
- Commands: `KEY`, `MOUSE`, `MOVE`, `SHOT`, `WAIT <frames>`, `WAIT_STATE <state> [timeout]`
- Assertions: `ASSERT_STATE`, `ASSERT_EQ/GE/LE <field> <value>`, `GET <field>`
- Reports: pass/fail counts + per-line details sent via TCP on completion
- Run: `make test` or `./tests/run_test.sh [script.sszb]`

## Status

All major systems have been ported. The game is fully playable with complete state machine integration. Both native (macOS) and web (WASM) builds are functional. Automated frame-based testing is operational.
