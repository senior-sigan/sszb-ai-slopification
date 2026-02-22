# Architecture: Save Soul of Zlaya Babka — C/Raylib Port

Generated: 2026-02-22
Source analysis: legacy Scala/libgdx codebase + Raylib 5.5 API

---

## 1. Coordinate System

LibGDX uses Y-up (Y=0 at bottom); Raylib uses Y-down (Y=0 at top). All positions in this document use **Raylib coordinates** unless noted.

**Conversion formula** for a libgdx position (gx, gy) for a sprite of height `h`:
```
raylib_x = gx
raylib_y = 768 - gy - h
```

**Key reference positions (Raylib)**:
| Element | Raylib X | Raylib Y | Notes |
|---------|----------|----------|-------|
| Room row 0 top-left | `col*128+128` | 384 | visual bottom floor |
| Room row 1 top-left | `col*128+128` | 256 | visual middle floor |
| Room row 2 top-left | `col*128+128` | 128 | visual top floor |
| Room center X | `col*128+192` | — | col*128+128+64 |
| Room center Y | — | `384 - row*128 + 64` | |
| Road 0 creature Y | — | 624 | bottom lane (streetlevel) |
| Road 1 creature Y | — | 518 | upper lane |
| Ground Y (weapon landing) | — | 704 | libgdx y=64 |
| Weight hitL0 zone boundary | — | 576 | libgdx y=192, hits road 1 |
| Weight hitL1 zone boundary | — | 640 | libgdx y=128, hits all roads |
| Club position | 1010 | 284 | libgdx (1010, 100), club h≈384 |
| HUD portrait | 961 | 5 | libgdx (961, 699), hud h=64 |
| Money text | 1150 | 44 | libgdx (1150, 685) |
| Time text | 1150 | 79 | libgdx (1150, 650) |
| Level text | 1150 | 114 | libgdx (1150, 615) |

---

## 2. File Structure

```
src/
├── main.c           # Entry point, game loop, state machine dispatcher, TCP integration
├── command_server.h # UNCHANGED — STB-style TCP server
├── const.h          # All compile-time constants (macros only, no .c file)
├── entity.h         # All entity struct definitions (no .c file)
├── anim.h           # SpriteSheet + animation utilities (header-only implementation)
├── assets.h         # Assets struct declaration + load/unload signatures
├── assets.c         # assets_load() / assets_unload() implementations
├── game.h           # Game struct + GameState enum declarations
├── game.c           # game_init(), game_reset(), room init helpers
├── night.h          # Night phase function declarations
├── night.c          # night_enter/exit/update/render
└── day.h            # Day phase function declarations
    day.c            # day_enter/exit/update/render
```

**CMakeLists.txt**: No changes needed. `file(GLOB_RECURSE SOURCES src/*.c)` picks up all new .c files automatically.

---

## 3. const.h

```c
#pragma once

// Screen
#define SCREEN_WIDTH    1366
#define SCREEN_HEIGHT   768
#define TARGET_FPS      60
#define CMD_PORT        9999

// Building grid
#define ROOMS_ROWS      3
#define ROOMS_COLS      6

// Room cell size (pixels)
#define ROOM_CELL_SIZE  128

// Room position (Raylib top-left of 128×128 cell)
#define ROOM_X(col)     ((col) * ROOM_CELL_SIZE + ROOM_CELL_SIZE)
#define ROOM_Y(row)     (384 - (row) * ROOM_CELL_SIZE)
#define ROOM_CENTER_X(col) (ROOM_X(col) + 64)
#define ROOM_CENTER_Y(row) (ROOM_Y(row) + 64)

// Creature lane Y positions (Raylib, top-left of sprite)
#define LANE_Y(road)    ((road) == 0 ? 624 : 518)

// Creature dimensions (from Assets.scala TextureRegion.split)
#define HOOLIGAN_FRAME_W   120
#define HOOLIGAN_FRAME_H   128
#define WHORE_FRAME_W      88
#define WHORE_FRAME_H      128

// Weight physics (Raylib, y increases as weapon falls)
#define WEIGHT_SPEED        400.0f
#define BOTTLE_SPEED        250.0f
#define WEIGHT_GROUND_Y     704       // weapon removed when bottom edge > this
#define WEIGHT_HIT_L0_Y     576       // hits road 1 when weapon bottom > this
#define WEIGHT_HIT_L1_Y     640       // hits all roads when weapon bottom > this

// Weapon dimensions
#define POT_WIDTH   48
#define POT_HEIGHT  64
#define TV_WIDTH    92
#define TV_HEIGHT   96
#define ROYAL_WIDTH 128
#define ROYAL_HEIGHT 108

// Club (Raylib coords, approx — adjust from actual texture height)
#define CLUB_X          1010
#define CLUB_Y          284
#define CLUB_THRESHOLD  1010   // creatures past this x cause a hit

// HUD (Raylib)
#define HUD_X           961
#define HUD_Y           5
#define HUD_HIT_WIDTH   80     // width per hit in hud.png
#define HUD_HEIGHT      64
#define MONEY_TEXT_X    1150
#define MONEY_TEXT_Y    44
#define TIME_TEXT_X     1150
#define TIME_TEXT_Y     79
#define LVL_TEXT_X      1150
#define LVL_TEXT_Y      114
#define HUD_FONT_SIZE   44

// Day shop button positions (Raylib, buttons at bottom of screen, height ~120)
#define BTN_HEIGHT      120
#define BTN_BUY_X       0
#define BTN_BUY_Y       (SCREEN_HEIGHT - BTN_HEIGHT)
#define BTN_GRATE_X     343
#define BTN_GRATE_Y     (SCREEN_HEIGHT - BTN_HEIGHT)
#define BTN_WEAPON_X    686
#define BTN_WEAPON_Y    (SCREEN_HEIGHT - BTN_HEIGHT)
#define BTN_CLUB_X      1029
#define BTN_CLUB_Y      (SCREEN_HEIGHT - BTN_HEIGHT)
#define PRICE_TEXT_Y    (SCREEN_HEIGHT - 20)

// Game limits for fixed arrays
#define MAX_CREATURES   32
#define MAX_BULLETS     64
#define MAX_WEIGHTS     16
#define MAX_CRASHES     32
#define MAX_CASH        32

// Gameplay constants
#define MAX_HITS        5
#define LEVEL_TIME      30.0f
#define LOGO_DURATION   1.5f
#define TUTORIAL_PAGES  3
#define START_MONEY     10
#define CLUB_PRICE      400

// Room price multipliers
#define PRICE_MUL_BUY       10
#define PRICE_MUL_REPAIR    5
#define PRICE_MUL_GRATE     15
#define PRICE_MUL_POT       5
#define PRICE_MUL_TV        10
#define PRICE_MUL_ROYAL     20

// Kill rewards
#define REWARD_HOOLIGAN     7
#define REWARD_WHORE        6

// Grate lives
#define GRATE_LIVES         2

// Difficulty: room weapon cooldowns (seconds)
#define COOLDOWN_POT    1.0f
#define COOLDOWN_TV     2.0f
#define COOLDOWN_ROYAL  3.0f

// Animation speeds (seconds per frame)
#define ANIM_SPEED_NORMAL   0.2f
#define ANIM_SPEED_FAST     0.1f
#define ANIM_SPEED_CLUB     0.6f
#define ANIM_SPEED_FRAME    0.2f

// Selfie flash effect
#define SELFIE_DELAY        0.5f
#define SELFIE_DURATION     2.0f
#define SELFIE_ROAD_MAX     1  // only affects babka on rows 0 or 1 (not row 2)

// Bullet hit distance threshold
#define BULLET_HIT_DIST     5.0f

// Weapon draw offsets in room (Raylib, from ROOM_X/ROOM_Y)
#define WND_WEAPON_POT_DX   55
#define WND_WEAPON_POT_DY   30   // added to ROOM_Y (downward in Raylib)
#define WND_WEAPON_TV_DX    50
#define WND_WEAPON_TV_DY    30
#define WND_WEAPON_ROYAL_DX 60
#define WND_WEAPON_ROYAL_DY 30

// Babka hands-up offset in room
#define BABKA_DX    23
#define BABKA_DY    30
```

---

## 4. entity.h

```c
#pragma once
#include <stdbool.h>

// ─── Room ──────────────────────────────────────────────────────────────────

typedef enum {
    ROOM_TYPE_POT   = 1,
    ROOM_TYPE_TV    = 2,
    ROOM_TYPE_ROYAL = 3,
} RoomType;

typedef struct {
    RoomType type;
    int col, row;           // grid position (col: 0-5, row: 0-2)
    int base_price;         // 1=cheap,2=normal,3=rich,4=enormous

    bool bought;
    bool broken;
    bool grate;
    bool armed;

    // Cooldown system: when firing, ready_to_fire=false, timer counts down
    bool ready_to_fire;     // true = room can fire weapon
    float cooldown_timer;   // counts down to 0 when weapon fired
    int grate_lives;        // starts at GRATE_LIVES (2)
} Room;

// Derived pricing (inline helpers — no division, all multiply)
static inline int room_buy_price(const Room *r)    { return r->base_price * PRICE_MUL_BUY;    }
static inline int room_repair_price(const Room *r) { return r->base_price * PRICE_MUL_REPAIR; }
static inline int room_grate_price(const Room *r)  { return r->base_price * PRICE_MUL_GRATE;  }
static inline int room_weapon_price(const Room *r) {
    switch (r->type) {
        case ROOM_TYPE_POT:   return r->base_price * PRICE_MUL_POT;
        case ROOM_TYPE_TV:    return r->base_price * PRICE_MUL_TV;
        case ROOM_TYPE_ROYAL: return r->base_price * PRICE_MUL_ROYAL;
    }
    return 0;
}
static inline float room_cooldown_time(const Room *r) {
    switch (r->type) {
        case ROOM_TYPE_POT:   return COOLDOWN_POT;
        case ROOM_TYPE_TV:    return COOLDOWN_TV;
        case ROOM_TYPE_ROYAL: return COOLDOWN_ROYAL;
    }
    return 1.0f;
}

// ─── Creature ──────────────────────────────────────────────────────────────

typedef enum {
    CREATURE_HOOLIGAN,
    CREATURE_WHORE,
} CreatureType;

typedef struct {
    CreatureType type;
    bool alive;

    float x;            // Raylib X (moves left to right; 0 = left edge)
    int road;           // 0 = bottom lane, 1 = top lane

    float speed;        // px/second
    float width;        // sprite frame width (for collision)

    // Attack state
    bool can_attack;        // true = trigger attack this frame
    float attack_timer;     // counts down to 0 → can_attack=true
    bool attacking;         // attack animation is playing
    float attack_anim_timer;// counts down to 0 → attacking=false

    // Rendering animation time
    float state_time;       // elapsed seconds for walk animation
    float attack_state_time;// elapsed seconds for attack animation
} Creature;

// ─── Bullet ────────────────────────────────────────────────────────────────

typedef struct {
    bool alive;
    float x, y;             // current position (Raylib)
    float target_x, target_y;
    float speed;
    int target_row, target_col; // which room this is aimed at (for damage)
} Bullet;

// ─── Weight (falling weapon) ───────────────────────────────────────────────

typedef enum {
    WEIGHT_POT,
    WEIGHT_TV,
    WEIGHT_ROYAL,
} WeightType;

typedef struct {
    WeightType type;
    bool alive;
    float x, y;     // Raylib top-left; y increases as weapon falls
    float speed;    // px/second downward
} Weight;

// ─── Crash animation ───────────────────────────────────────────────────────

typedef enum {
    CRASH_POT,
    CRASH_TV,
    CRASH_ROYAL,
    CRASH_HOOLIGAN_DIE,
    CRASH_WHORE_DIE,
} CrashType;

typedef struct {
    CrashType type;
    bool alive;
    float x, y;
    float elapsed;      // time since spawn
} CrashAnim;

// ─── Cash pickup ───────────────────────────────────────────────────────────

typedef struct {
    bool alive;
    float x, y;
    float lifetime;     // counts down; removed at 0
} CashPickup;
```

---

## 5. anim.h  (header-only)

```c
#pragma once
#include <raylib.h>

// Sprite sheet: all frames in a single row (horizontal layout)
// Loaded via LoadTexture(); frames accessed via DrawTextureRec
typedef struct {
    Texture2D texture;
    int frame_width;    // per-frame pixel width
    int frame_height;   // = texture.height
} SpriteSheet;

static inline SpriteSheet sprite_sheet_load(const char *path, int frame_width) {
    SpriteSheet s = {0};
    s.texture     = LoadTexture(path);
    s.frame_width = frame_width;
    s.frame_height = s.texture.height;
    return s;
}

static inline void sprite_sheet_unload(SpriteSheet *s) {
    UnloadTexture(s->texture);
    *s = (SpriteSheet){0};
}

static inline int sprite_sheet_frame_count(const SpriteSheet *s) {
    return s->texture.width / s->frame_width;
}

// Draw a specific frame at Raylib position
static inline void sprite_draw_frame(const SpriteSheet *s, int frame,
                                     Vector2 pos, Color tint) {
    Rectangle src = {
        .x      = (float)(frame * s->frame_width),
        .y      = 0,
        .width  = (float)s->frame_width,
        .height = (float)s->frame_height,
    };
    DrawTextureRec(s->texture, src, pos, tint);
}

// Select looping frame from a frame index list (elapsed: seconds, fps: frames/sec)
static inline int anim_frame_loop(const int *frames, int count,
                                  float elapsed, float fps) {
    int idx = (int)(elapsed * fps) % count;
    if (idx < 0) idx = 0;
    return frames[idx];
}

// Select one-shot frame (clamps at last frame)
static inline int anim_frame_once(const int *frames, int count,
                                  float elapsed, float fps) {
    int idx = (int)(elapsed * fps);
    if (idx >= count) idx = count - 1;
    return frames[idx];
}

// One-shot animation total duration
static inline float anim_duration(int count, float fps) {
    return (float)count / fps;
}
```

**Frame sequences** (derived from Assets.scala, all from a single horizontal sprite sheet):

```c
// gopstop.gif — 7 frames, 120×128 each
// Sequences reference into the sheet
static const int ANIM_HOOLIGAN_IDLE[]   = {0, 1};          // looping, normal speed
static const int ANIM_HOOLIGAN_ATTACK[] = {2, 3, 4, 5, 6}; // one-shot, normal speed
static const int ANIM_HOOLIGAN_DIE[]    = {5, 6, 5, 6};    // one-shot, normal speed

// whore.gif — 8 frames, 88×128 each
static const int ANIM_WHORE_WALK[]      = {0, 1, 2, 3};    // looping, normal speed
static const int ANIM_WHORE_ATTACK[]    = {4, 5, 6, 7};    // looping, normal speed
static const int ANIM_WHORE_DIE[]       = {1, 3};           // one-shot, normal speed

// pot_crash_animation.gif — 4 frames, 80×104 each, fast speed
static const int ANIM_POT_CRASH[]       = {0, 1, 2, 3};

// tv_crash_animation.gif — 6 frames, 160×96 each, fast speed
static const int ANIM_TV_CRASH[]        = {0, 1, 2, 3, 4, 5};

// piano_crash_animation.gif — 8 frames, 200×128 each, fast speed
static const int ANIM_ROYAL_CRASH[]     = {0, 1, 2, 3, 4, 5, 6, 7};

// club_night_1.gif — 2 frames, 382×484 each, club speed
static const int ANIM_CLUB[]            = {0, 1};

// frame.png — 2 frames, 138×138 each, frame speed
static const int ANIM_FRAME_SEL[]       = {0, 1};
```

---

## 6. assets.h / assets.c

### assets.h

```c
#pragma once
#include <raylib.h>
#include "anim.h"

typedef struct {
    // ── Backgrounds ──────────────────────────────────────────────────────
    Texture2D bg_night;         // bg_night.gif
    Texture2D bg_day;           // bg_day.gif
    Texture2D club_day;         // club_day.gif
    SpriteSheet club_night;     // club_night_1.gif (2 fr, 382×484)

    Texture2D light_day;        // light_day.png (daytime window fill)
    Texture2D light_off;        // light_off.png (unlit night)
    Texture2D light_on;         // light_on.png (lit night)

    // ── Windows ──────────────────────────────────────────────────────────
    Texture2D window_normal_day;
    Texture2D window_normal_night;
    Texture2D window_broken_day;
    Texture2D window_broken_night;
    Texture2D window_disabled_day;
    Texture2D window_disabled_night;
    Texture2D window_grid_day;
    Texture2D window_grid_night;

    // ── Characters ───────────────────────────────────────────────────────
    Texture2D babka_in_window;  // babka_in_window.gif (single frame)
    Texture2D babka_hands_up;   // babka_hands_up.gif (single frame)
    SpriteSheet hooligan;       // gopstop.gif (7 fr, 120×128)
    SpriteSheet whore;          // whore.gif (8 fr, 88×128)
    Texture2D cash;             // cash.gif

    // ── Weapons in-flight ────────────────────────────────────────────────
    Texture2D pot;              // pot.gif
    Texture2D tv;               // tv.gif
    Texture2D royal;            // royal.gif
    Texture2D bottle;           // bottle.png

    // ── Weapon crash animations ───────────────────────────────────────────
    SpriteSheet pot_crash;      // pot_crash_animation.gif (4 fr, 80×104)
    SpriteSheet tv_crash;       // tv_crash_animation.gif (6 fr, 160×96)
    SpriteSheet royal_crash;    // piano_crash_animation.gif (8 fr, 200×128)

    // ── Weapon-in-window previews ─────────────────────────────────────────
    Texture2D flower_in_window; // flower_in_window.gif
    Texture2D tv_in_window;     // tv_in_window.gif
    Texture2D piano_in_window;  // piano_in_window.gif

    // ── UI / HUD ─────────────────────────────────────────────────────────
    Texture2D hud_back;         // hud_back.png
    Texture2D hud;              // hud.png (clipped by hits count)
    Texture2D hud_front;        // hud_front.png
    SpriteSheet frame_sel;      // frame.png (2 fr, 138×138, selection frame)
    Texture2D logo;             // logo.png
    Texture2D t2;               // t2.png (key binding tutorial overlay)

    // ── Shop buttons ─────────────────────────────────────────────────────
    Texture2D btn_buy;          // inrerface_buy.gif
    Texture2D btn_repair;       // inrerface_repair.gif
    Texture2D btn_grate;        // inrerface_grid.gif
    Texture2D btn_weapon;       // inrerface_weapon.gif (no weapon)
    Texture2D btn_weapon_pot;   // inrerface_weapon_flower.gif
    Texture2D btn_weapon_tv;    // inrerface_weapon_tv.gif
    Texture2D btn_weapon_royal; // inrerface_weapon_piano.gif
    Texture2D btn_club;         // interface_club.gif

    // ── Full screens ─────────────────────────────────────────────────────
    Texture2D screen_menu;      // menu.gif
    Texture2D screen_gameover;  // gameover.gif
    Texture2D screen_gamewin;   // gamewin.gif
    Texture2D tutor[3];         // tutor_1.gif, tutor_2.gif, tutor_3.gif

    // ── Fonts ─────────────────────────────────────────────────────────────
    Font font_main;             // main.ttf, size 44

    // ── Audio ─────────────────────────────────────────────────────────────
    Music bgm_crickets;         // crickets.mp3 — night ambient (vol 0.2, loop)
    Music bgm_birds;            // birds.mp3 — day ambient (vol 0.2, loop)
    Music bgm_cool;             // bgm_cool.mp3 — menu/lose music (loop)

    Sound sfx_pot_destroy;      // pot_destroy.mp3
    Sound sfx_tv_destroy;       // tv_destroy.mp3
    Sound sfx_selfie;           // selfie.mp3
    Sound sfx_iron;             // iron.mp3
    Sound sfx_bye;              // bye.mp3
    Sound sfx_round_end;        // round_end.mp3
    Sound sfx_window_broken;    // window_broken.mp3
    Sound sfx_royal_deploy;     // royal_deploy.mp3
} Assets;

void assets_load(Assets *a);
void assets_unload(Assets *a);
```

### assets.c (key excerpts)

```c
#include "assets.h"
#include <raylib.h>

void assets_load(Assets *a) {
    *a = (Assets){0};

    // Backgrounds
    a->bg_night = LoadTexture("assets/textures/bg_night.gif");
    a->bg_day   = LoadTexture("assets/textures/bg_day.gif");
    a->club_day = LoadTexture("assets/textures/club_day.gif");
    a->club_night = sprite_sheet_load("assets/textures/club_night_1.gif", 382);

    a->light_day = LoadTexture("assets/textures/light_day.png");
    a->light_off  = LoadTexture("assets/textures/light_off.png");
    a->light_on   = LoadTexture("assets/textures/light_on.png");

    // Windows
    a->window_normal_day     = LoadTexture("assets/textures/window_normal_day.gif");
    a->window_normal_night   = LoadTexture("assets/textures/window_normal_night.gif");
    a->window_broken_day     = LoadTexture("assets/textures/window_broken_day.gif");
    a->window_broken_night   = LoadTexture("assets/textures/window_broken_night.gif");
    a->window_disabled_day   = LoadTexture("assets/textures/window_day_disabled.gif");
    a->window_disabled_night = LoadTexture("assets/textures/window_night_disabled.gif");
    a->window_grid_day       = LoadTexture("assets/textures/window_grid_day.gif");
    a->window_grid_night     = LoadTexture("assets/textures/window_grid_night.gif");

    // Characters
    a->babka_in_window = LoadTexture("assets/textures/babka_in_window.gif");
    a->babka_hands_up  = LoadTexture("assets/textures/babka_hands_up.gif");
    a->hooligan = sprite_sheet_load("assets/textures/gopstop.gif", 120);
    a->whore    = sprite_sheet_load("assets/textures/whore.gif",   88);
    a->cash     = LoadTexture("assets/textures/cash.gif");

    // Weapons
    a->pot    = LoadTexture("assets/textures/pot.gif");
    a->tv     = LoadTexture("assets/textures/tv.gif");
    a->royal  = LoadTexture("assets/textures/royal.gif");
    a->bottle = LoadTexture("assets/textures/bottle.png");

    // Crash animations
    a->pot_crash   = sprite_sheet_load("assets/textures/pot_crash_animation.gif",   80);
    a->tv_crash    = sprite_sheet_load("assets/textures/tv_crash_animation.gif",    160);
    a->royal_crash = sprite_sheet_load("assets/textures/piano_crash_animation.gif", 200);

    // Window weapon previews
    a->flower_in_window = LoadTexture("assets/textures/flower_in_window.gif");
    a->tv_in_window     = LoadTexture("assets/textures/tv_in_window.gif");
    a->piano_in_window  = LoadTexture("assets/textures/piano_in_window.gif");

    // UI/HUD
    a->hud_back  = LoadTexture("assets/textures/hud_back.png");
    a->hud       = LoadTexture("assets/textures/hud.png");
    a->hud_front = LoadTexture("assets/textures/hud_front.png");
    a->frame_sel = sprite_sheet_load("assets/textures/frame.png", 138);
    a->logo      = LoadTexture("assets/textures/logo.png");
    a->t2        = LoadTexture("assets/textures/t2.png");

    // Shop buttons
    a->btn_buy         = LoadTexture("assets/textures/inrerface_buy.gif");
    a->btn_repair      = LoadTexture("assets/textures/inrerface_repair.gif");
    a->btn_grate       = LoadTexture("assets/textures/inrerface_grid.gif");
    a->btn_weapon      = LoadTexture("assets/textures/inrerface_weapon.gif");
    a->btn_weapon_pot  = LoadTexture("assets/textures/inrerface_weapon_flower.gif");
    a->btn_weapon_tv   = LoadTexture("assets/textures/inrerface_weapon_tv.gif");
    a->btn_weapon_royal= LoadTexture("assets/textures/inrerface_weapon_piano.gif");
    a->btn_club        = LoadTexture("assets/textures/interface_club.gif");

    // Screens
    a->screen_menu     = LoadTexture("assets/textures/menu.gif");
    a->screen_gameover = LoadTexture("assets/textures/gameover.gif");
    a->screen_gamewin  = LoadTexture("assets/textures/gamewin.gif");
    a->tutor[0]        = LoadTexture("assets/textures/tutor_1.gif");
    a->tutor[1]        = LoadTexture("assets/textures/tutor_2.gif");
    a->tutor[2]        = LoadTexture("assets/textures/tutor_3.gif");

    // Fonts
    a->font_main = LoadFontEx("assets/font/main.ttf", HUD_FONT_SIZE, NULL, 0);

    // Audio — music streams
    a->bgm_crickets = LoadMusicStream("assets/sound/crickets.mp3");
    a->bgm_birds    = LoadMusicStream("assets/sound/birds.mp3");
    a->bgm_cool     = LoadMusicStream("assets/sound/bgm_cool.mp3");
    SetMusicVolume(a->bgm_crickets, 0.2f);
    SetMusicVolume(a->bgm_birds,    0.2f);
    SetMusicVolume(a->bgm_cool,     1.0f);

    // Audio — sound effects
    a->sfx_pot_destroy   = LoadSound("assets/sound/pot_destroy.mp3");
    a->sfx_tv_destroy    = LoadSound("assets/sound/tv_destroy.mp3");
    a->sfx_selfie        = LoadSound("assets/sound/selfie.mp3");
    a->sfx_iron          = LoadSound("assets/sound/iron.mp3");
    a->sfx_bye           = LoadSound("assets/sound/bye.mp3");
    a->sfx_round_end     = LoadSound("assets/sound/round_end.mp3");
    a->sfx_window_broken = LoadSound("assets/sound/window_broken.mp3");
    a->sfx_royal_deploy  = LoadSound("assets/sound/royal_deploy.mp3");
}

void assets_unload(Assets *a) {
    UnloadTexture(a->bg_night);
    // ... unload every Texture2D, SpriteSheet, Font, Music, Sound field
    sprite_sheet_unload(&a->club_night);
    // etc.
    CloseAudioDevice(); // called AFTER unloading
}
```

---

## 7. game.h / game.c

### game.h

```c
#pragma once
#include <stdbool.h>
#include "entity.h"
#include "assets.h"

typedef enum {
    STATE_LOGO,
    STATE_MENU,
    STATE_TUTORIAL,
    STATE_NIGHT,
    STATE_DAY,
    STATE_GAMEOVER,
    STATE_GAMEWIN,
} GameState;

typedef struct {
    GameState state;

    // ── Shared persistent state ───────────────────────────────────────────
    int money;
    int level;
    bool club_bought;

    // ── Night phase state ─────────────────────────────────────────────────
    int hits;               // 0–5; game over at 5
    float level_time;       // elapsed seconds this night
    int babka_row, babka_col; // current position in building grid

    // Entity pools
    Room rooms[ROOMS_ROWS][ROOMS_COLS];

    Creature creatures[MAX_CREATURES];
    int creature_count;

    Bullet bullets[MAX_BULLETS];
    int bullet_count;

    Weight weights[MAX_WEIGHTS];
    int weight_count;

    CrashAnim crashes[MAX_CRASHES];
    int crash_count;

    CashPickup cash_pickups[MAX_CASH];
    int cash_count;

    // AI timer
    float generator_timer;  // countdown; spawn when <= 0

    // Night visual effects
    float club_anim_time;   // elapsed for club sprite animation
    bool  selfie_active;
    float selfie_timer;     // countdown for selfie flash duration

    // ── Day phase state ───────────────────────────────────────────────────
    int day_cursor_row, day_cursor_col;
    float frame_sel_time;   // elapsed for selection frame animation

    // ── Menu/logo state ───────────────────────────────────────────────────
    float logo_timer;
    int tutorial_page;      // 0, 1, or 2

    // ── Audio control ─────────────────────────────────────────────────────
    // Which music is currently playing (to avoid double-play)
    bool music_crickets_playing;
    bool music_birds_playing;
    bool music_cool_playing;

    // ── Assets (owned by Game, loaded once) ──────────────────────────────
    Assets assets;
} Game;

void game_init(Game *g);    // zero-init + mark rooms for later setup
void game_reset(Game *g);   // full reset: money=START_MONEY, level=1, rooms, hit=0
void game_init_rooms(Game *g);  // set up 3×6 room layout per RenderFactory spec
```

### game.c — room layout (from RenderFactory.scala)

```c
// Room type layout: [row][col] → (RoomType, base_price)
// Row 0 = visual bottom floor, Row 2 = visual top floor
static const struct { RoomType type; int price; } ROOM_LAYOUT[ROOMS_ROWS][ROOMS_COLS] = {
    // Row 0 (bottom visual floor)
    {{ ROOM_TYPE_TV,    4 }, { ROOM_TYPE_POT,   4 }, { ROOM_TYPE_POT,   3 },
     { ROOM_TYPE_POT,   3 }, { ROOM_TYPE_TV,    3 }, { ROOM_TYPE_ROYAL, 4 }},
    // Row 1 (middle floor)
    {{ ROOM_TYPE_POT,  4 }, { ROOM_TYPE_POT,   3 }, { ROOM_TYPE_TV,    3 },
     { ROOM_TYPE_TV,   2 }, { ROOM_TYPE_POT,   2 }, { ROOM_TYPE_ROYAL, 3 }},
    // Row 2 (top floor)
    {{ ROOM_TYPE_ROYAL, 3 }, { ROOM_TYPE_ROYAL, 3 }, { ROOM_TYPE_POT,   1 },
     { ROOM_TYPE_ROYAL, 2 }, { ROOM_TYPE_TV,    2 }, { ROOM_TYPE_POT,   1 }},
};

void game_init_rooms(Game *g) {
    for (int row = 0; row < ROOMS_ROWS; row++) {
        for (int col = 0; col < ROOMS_COLS; col++) {
            Room *r = &g->rooms[row][col];
            r->type        = ROOM_LAYOUT[row][col].type;
            r->base_price  = ROOM_LAYOUT[row][col].price;
            r->row         = row;
            r->col         = col;
            r->bought      = false;
            r->broken      = false;
            r->grate       = false;
            r->armed       = false;
            r->ready_to_fire = true;
            r->cooldown_timer = 0;
            r->grate_lives = GRATE_LIVES;
        }
    }
    // firstRoom = (row=1, col=2) and (row=2, col=2) start bought+armed
    g->rooms[1][2].bought = true;
    g->rooms[1][2].armed  = true;
    g->rooms[2][2].bought = true;
    g->rooms[2][2].armed  = true;
    g->babka_row = 1;
    g->babka_col = 2;
}

void game_reset(Game *g) {
    g->money        = START_MONEY;
    g->level        = 1;
    g->club_bought  = false;
    g->hits         = 0;
    g->level_time   = 0;
    g->creature_count = 0;
    g->bullet_count   = 0;
    g->weight_count   = 0;
    g->crash_count    = 0;
    g->cash_count     = 0;
    g->generator_timer = 0;
    g->selfie_active   = false;
    game_init_rooms(g);
}
```

---

## 8. Difficulty Scaling

```c
// In const.h or a static inline function in game.h:

#include <stdlib.h>  // rand()

static inline float difficulty_generator_timer(int lvl) {
    switch (lvl) {
        case 1: return 2.2f;
        case 2: return 2.0f;
        case 3: return 1.8f;
        case 4: return 1.8f;
        default: return 1.0f;
    }
}

static inline float difficulty_hooligan_speed(int lvl) {
    return (float)(65 + lvl * 10 + (rand() % (lvl * 10 + 1)));
}

static inline float difficulty_hooligan_cooldown(int lvl) {
    switch (lvl) {
        case 1: return 10.0f;
        case 2: return  8.0f;
        case 3: return  3.0f + (float)(rand() % 4);
        default: return 2.0f + (float)(rand() % 3);
    }
}

static inline float difficulty_whore_speed(int lvl) {
    switch (lvl) {
        case 1: return 120.0f;
        case 2: return 125.0f;
        case 3: return 150.0f;
        default: return (float)(100 + lvl * 10 + rand() % (lvl * 10 + 1));
    }
}

static inline float difficulty_whore_cooldown(int lvl) {
    switch (lvl) {
        case 1: return 4.0f + (float)(rand() % 4);
        case 2: return 3.0f + (float)(rand() % 4);
        default: return 2.0f + (float)(rand() % 6);
    }
}

// Spawn pattern: returns (spawn_road0, spawn_road1)
// seed in [0,9]
static inline void difficulty_spawn_pattern(int lvl, int seed,
                                             bool *out_road0, bool *out_road1) {
    switch (lvl) {
        case 1: *out_road0 = (seed < 5); *out_road1 = (seed >= 5); break;
        case 2: *out_road0 = (seed >= 5); *out_road1 = (seed < 5 || seed >= 5); break;
        // ... (expand per original Const.scala spawnRandom)
        default: *out_road0 = true; *out_road1 = true; break;
    }
}
```

---

## 9. night.h / night.c

### night.h

```c
#pragma once
#include "game.h"
#include "command_server.h"

void night_enter(Game *g);
void night_exit(Game *g);
// Returns next GameState (STATE_NIGHT if no transition)
GameState night_update(Game *g, float dt, Command cmd);
void night_render(Game *g);
```

### night.c — update logic (pseudocode + key algorithms)

```
night_enter(g):
    g->hits = 0
    g->level_time = 0
    g->creature_count = 0
    g->bullet_count = 0
    g->weight_count = 0
    g->crash_count = 0
    g->cash_count = 0
    g->selfie_active = false
    g->generator_timer = difficulty_generator_timer(g->level)
    g->club_anim_time = 0
    // Reset all room cooldowns to ready
    for each room: room->ready_to_fire = true; room->cooldown_timer = 0
    // Start crickets music
    PlayMusicStream(g->assets.bgm_crickets)

night_exit(g):
    StopMusicStream(g->assets.bgm_crickets)
    // Clear all entities
    g->creature_count = 0
    g->bullet_count = 0
    g->weight_count = 0
    // Note: crashes and cash_pickups can persist briefly; clear them too
    g->crash_count = 0
    g->cash_count = 0

GameState night_update(g, dt, cmd):
    UpdateMusicStream(g->assets.bgm_crickets)
    g->level_time += dt
    g->club_anim_time += dt

    // 1. Player input
    night_handle_input(g, dt, cmd)

    // 2. AI: spawn creatures
    g->generator_timer -= dt
    if g->generator_timer <= 0:
        night_spawn_creatures(g)
        g->generator_timer = difficulty_generator_timer(g->level)

    // 3. Creature update
    night_update_creatures(g, dt)

    // 4. Bullet update
    night_update_bullets(g, dt)

    // 5. Weight update
    night_update_weights(g, dt)

    // 6. Crash animations update
    for each alive crash: crash->elapsed += dt
    compact(g->crashes, &g->crash_count, crash_is_done)

    // 7. Cash pickups update
    for each alive cash: cash->lifetime -= dt
    compact(g->cash_pickups, &g->cash_count, cash_is_expired)

    // 8. Room cooldown update
    for each room in 3×6 grid:
        if !room->ready_to_fire && room->cooldown_timer > 0:
            room->cooldown_timer -= dt
            if room->cooldown_timer <= 0:
                room->ready_to_fire = true

    // 9. Selfie flash update
    if g->selfie_active:
        g->selfie_timer -= dt
        if g->selfie_timer <= 0: g->selfie_active = false

    // 10. Club hit check (LooseControl)
    night_check_club_hits(g)

    // 11. Win/lose conditions
    if g->hits >= MAX_HITS: return STATE_GAMEOVER
    if g->level_time >= LEVEL_TIME: return STATE_DAY
    return STATE_NIGHT
```

**night_handle_input**:
```
cmd from TCP or keyboard (ManagedIsKeyPressed):

Move babka:
  KEY_UP:    if row+1 < ROWS && rooms[row+1][col].bought && !rooms[row+1][col].broken → babka_row++
  KEY_DOWN:  if row-1 >= 0 && rooms[row-1][col].bought && !rooms[row-1][col].broken → babka_row--
  KEY_LEFT:  if col-1 >= 0 && rooms[row][col-1].bought && !rooms[row][col-1].broken → babka_col--
  KEY_RIGHT: if col+1 < COLS && rooms[row][col+1].bought && !rooms[row][col+1].broken → babka_col++

  Note: libgdx UP key = Y increases = move up on screen = higher row in Raylib
        KEY_UP in game = move to higher floor (row+1 in libgdx convention)

Fire weapon (KEY_SPACE):
  room = &rooms[babka_row][babka_col]
  if room->armed && room->ready_to_fire:
      spawn_weight(g, room)
      room->ready_to_fire = false
      room->cooldown_timer = room_cooldown_time(room)
      (play launch sound if desired)
```

**spawn_weight**:
```c
void night_spawn_weight(Game *g, Room *r) {
    if (g->weight_count >= MAX_WEIGHTS) return;
    Weight *w = &g->weights[g->weight_count++];
    w->alive = true;
    w->type  = (WeightType)(r->type - 1);  // ROOM_TYPE_POT=1 → WEIGHT_POT=0
    w->speed = WEIGHT_SPEED;
    // Raylib position: weapon top-left starts at room position
    w->x = (float)ROOM_X(r->col);
    w->y = (float)ROOM_Y(r->row);  // starts at room top in Raylib
}
```

**night_spawn_creatures**:
```c
void night_spawn_creatures(Game *g) {
    int seed = rand() % 10;
    bool road0, road1;
    difficulty_spawn_pattern(g->level, seed, &road0, &road1);

    if (road0) night_spawn_one_creature(g, 0);
    if (road1) night_spawn_one_creature(g, 1);
}

void night_spawn_one_creature(Game *g, int road) {
    if (g->creature_count >= MAX_CREATURES) return;
    Creature *c = &g->creatures[g->creature_count++];
    *c = (Creature){0};
    c->alive = true;
    c->road  = road;
    c->x     = 0.0f;

    if (rand() % 2 == 0) {
        c->type  = CREATURE_HOOLIGAN;
        c->speed = difficulty_hooligan_speed(g->level);
        c->width = HOOLIGAN_FRAME_W;
        c->attack_timer = difficulty_hooligan_cooldown(g->level) / 2.0f;
    } else {
        c->type  = CREATURE_WHORE;
        c->speed = difficulty_whore_speed(g->level);
        c->width = WHORE_FRAME_W;
        c->attack_timer = difficulty_whore_cooldown(g->level);
    }
    c->can_attack = false;
    c->attacking  = false;
}
```

**night_update_creatures** (combining AIControl + movement):
```c
void night_update_creatures(Game *g, float dt) {
    for (int i = 0; i < g->creature_count; i++) {
        Creature *c = &g->creatures[i];
        if (!c->alive) continue;

        // Cooldown timer → enable attack
        if (!c->can_attack) {
            c->attack_timer -= dt;
            if (c->attack_timer <= 0) c->can_attack = true;
        }

        // Trigger attack when ready
        if (c->can_attack) {
            c->can_attack = false;
            c->attacking = true;
            c->attack_state_time = 0;
            c->state_time = 0;

            if (c->type == CREATURE_HOOLIGAN) {
                night_hooligan_shoot(g, c);
                c->attack_timer = difficulty_hooligan_cooldown(g->level);
                // attack anim duration: ANIM_HOOLIGAN_ATTACK count=5 at normal speed
                c->attack_anim_timer = 5 * ANIM_SPEED_NORMAL;
            } else {
                night_whore_selfie(g, c);
                c->attack_timer = difficulty_whore_cooldown(g->level);
                // whore attack loops for its duration (same as timer)
                c->attack_anim_timer = c->attack_timer;
            }
        }

        // Attack animation countdown
        if (c->attacking) {
            c->attack_anim_timer -= dt;
            c->attack_state_time += dt;
            if (c->attack_anim_timer <= 0) {
                c->attacking = false;
                c->attack_state_time = 0;
            }
        }

        // Movement: always move (in libgdx, creatures move unless can_attack is true,
        // but can_attack triggers immediately each frame so movement always applies here)
        c->x += c->speed * dt;
        c->state_time += dt;  // walk animation clock
    }

    // Compact dead creatures
    night_compact_creatures(g);
}
```

**night_hooligan_shoot**:
```c
void night_hooligan_shoot(Game *g, Creature *h) {
    // Pick a random bought+non-broken room as target
    Room *candidates[ROOMS_ROWS * ROOMS_COLS];
    int count = 0;
    for (int row = 0; row < ROOMS_ROWS; row++)
        for (int col = 0; col < ROOMS_COLS; col++) {
            Room *r = &g->rooms[row][col];
            if (r->bought && !r->broken) candidates[count++] = r;
        }
    if (count == 0) return;
    Room *target = candidates[rand() % count];

    if (g->bullet_count >= MAX_BULLETS) return;
    Bullet *b = &g->bullets[g->bullet_count++];
    b->alive = true;
    b->speed = BOTTLE_SPEED;
    // Spawn at hooligan center position (Raylib)
    b->x = h->x + h->width / 2.0f;
    b->y = (float)(LANE_Y(h->road)) + HOOLIGAN_FRAME_H / 2.0f;
    // Target = room center (Raylib)
    b->target_x = (float)ROOM_CENTER_X(target->col);
    b->target_y = (float)ROOM_CENTER_Y(target->row);
    b->target_row = target->row;
    b->target_col = target->col;
}
```

**night_whore_selfie**:
```c
void night_whore_selfie(Game *g, Creature *w) {
    PlaySound(g->assets.sfx_selfie);
    // Only affects babka on rows 0 or 1 (not row 2 = top = SELFIE_ROAD_MAX=1)
    if (g->babka_row <= SELFIE_ROAD_MAX) {
        // Delay: start flash after SELFIE_DELAY seconds
        // Simple approach: use selfie_timer as a combined delay+flash timer
        g->selfie_active = true;
        g->selfie_timer  = SELFIE_DURATION;
    }
}
```

**night_update_bullets**:
```c
void night_update_bullets(Game *g, float dt) {
    for (int i = 0; i < g->bullet_count; i++) {
        Bullet *b = &g->bullets[i];
        if (!b->alive) continue;

        // Move toward target
        float dx = b->target_x - b->x;
        float dy = b->target_y - b->y;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist > 0) {
            b->x += b->speed * dt * (dx / dist);
            b->y += b->speed * dt * (dy / dist);
        }

        // Check arrival
        if (fabsf(b->x - b->target_x) < BULLET_HIT_DIST &&
            fabsf(b->y - b->target_y) < BULLET_HIT_DIST) {

            Room *r = &g->rooms[b->target_row][b->target_col];

            if (b->target_row == g->babka_row && b->target_col == g->babka_col) {
                // Babka intercepts: bullet vanishes (no damage)
            } else if (r->grate) {
                r->grate_lives--;
                PlaySound(g->assets.sfx_iron);
                if (r->grate_lives <= 0) {
                    r->grate = false;
                    r->grate_lives = GRATE_LIVES;
                }
            } else {
                r->broken = true;
                PlaySound(g->assets.sfx_window_broken);
            }
            b->alive = false;
        }
    }
    // Compact
    night_compact_bullets(g);
}
```

**night_update_weights**:
```c
void night_update_weights(Game *g, float dt) {
    for (int i = 0; i < g->weight_count; i++) {
        Weight *w = &g->weights[i];
        if (!w->alive) continue;

        w->y += w->speed * dt;  // fall downward

        int weapon_h = weight_height(w->type);  // POT=64, TV=96, ROYAL=108
        float bottom = w->y + weapon_h;
        int weapon_w = weight_width(w->type);   // POT=48, TV=92, ROYAL=128

        // Creature hit zones
        if (bottom > WEIGHT_HIT_L0_Y) {
            night_weight_hit_creatures(g, w, weapon_w, weapon_h, 1);  // road 1 only
        }
        if (bottom > WEIGHT_HIT_L1_Y) {
            night_weight_hit_creatures(g, w, weapon_w, weapon_h, -1); // all roads
        }

        // Ground impact
        if (bottom > WEIGHT_GROUND_Y) {
            night_spawn_crash(g, w);
            night_play_crash_sound(g, w->type);
            w->alive = false;
        }
    }
    night_compact_weights(g);
}

void night_weight_hit_creatures(Game *g, Weight *w, int ww, int wh, int road_filter) {
    float wx1 = w->x;
    float wx2 = w->x + ww;

    for (int i = 0; i < g->creature_count; i++) {
        Creature *c = &g->creatures[i];
        if (!c->alive) continue;
        if (road_filter >= 0 && c->road != road_filter) continue;

        float cx1 = c->x;
        float cx2 = c->x + c->width;

        // AABB overlap on X axis
        bool overlap = (wx1 < cx2) && (wx2 > cx1);
        if (overlap) {
            // Kill creature
            float cy = (float)LANE_Y(c->road);
            night_kill_creature(g, c, cy);
        }
    }
}

void night_kill_creature(Game *g, Creature *c, float y) {
    // Spawn death animation
    if (g->crash_count < MAX_CRASHES) {
        CrashAnim *ca = &g->crashes[g->crash_count++];
        ca->alive   = true;
        ca->type    = (c->type == CREATURE_HOOLIGAN) ? CRASH_HOOLIGAN_DIE : CRASH_WHORE_DIE;
        ca->x       = c->x;
        ca->y       = y;
        ca->elapsed = 0;
    }
    // Spawn cash pickup
    if (g->cash_count < MAX_CASH) {
        CashPickup *cp = &g->cash_pickups[g->cash_count++];
        cp->alive    = true;
        cp->x        = c->x;
        cp->y        = y - 10.0f + (float)(rand() % 20);
        cp->lifetime = 2.0f;  // show for 2 seconds
    }
    // Award money
    g->money += (c->type == CREATURE_HOOLIGAN) ? REWARD_HOOLIGAN : REWARD_WHORE;
    c->alive = false;
}
```

**night_check_club_hits** (LooseControl):
```c
void night_check_club_hits(Game *g) {
    int missed = 0;
    for (int i = 0; i < g->creature_count; i++) {
        if (g->creatures[i].alive && g->creatures[i].x > CLUB_THRESHOLD) {
            g->creatures[i].alive = false;
            missed++;
        }
    }
    if (missed > 0) {
        g->hits += missed;
        if (g->hits > MAX_HITS) g->hits = MAX_HITS;
        // Play club music at volume proportional to hits
        if (IsMusicStreamPlaying(g->assets.bgm_cool))
            PauseMusicStream(g->assets.bgm_cool);
        SetMusicVolume(g->assets.bgm_cool, 0.2f * g->hits);
        PlayMusicStream(g->assets.bgm_cool);
        // Update bgm_cool each frame too
    }
    night_compact_creatures(g);
}
```

### night.c — render

**night_render**:
```
night_render(g):
  BeginDrawing is called by main.c BEFORE night_render

  1. Background
     DrawTextureV(assets.bg_night, (0,0), WHITE)

  2. Club (animated)
     int club_frame = anim_frame_loop(ANIM_CLUB, 2, g->club_anim_time, 1.0f/ANIM_SPEED_CLUB)
     sprite_draw_frame(&assets.club_night, club_frame, (CLUB_X, CLUB_Y), WHITE)

  3. Babka in window (if room not on cooldown / i.e. ready_to_fire=true)
     Room *cur = &rooms[babka_row][babka_col]
     if cur->ready_to_fire:
         DrawTextureV(assets.babka_in_window, (ROOM_X(col)+0, ROOM_Y(row)+0), WHITE)

  4. Window grid (6 cols × 3 rows)
     For each room:
       a. If room not broken and armed and NOT ready (cooling down):
          draw weapon-in-window preview behind window:
            POT   → DrawTextureV(assets.flower_in_window, (ROOM_X+55, ROOM_Y+30), WHITE)
            TV    → DrawTextureV(assets.tv_in_window,     (ROOM_X+50, ROOM_Y+30), WHITE)
            ROYAL → DrawTextureV(assets.piano_in_window,  (ROOM_X+60, ROOM_Y+30), WHITE)

       b. Window background (light):
            If bought && !broken: draw assets.light_on at (ROOM_X, ROOM_Y)
            If !bought && !broken: draw assets.light_off at (ROOM_X, ROOM_Y)
            If broken: skip (no light)

       c. Window overlay:
            current room (babka) + cooldown=true (in Scala) = NOT ready_to_fire (in C)
            but just: if broken → window_broken_night
                      if grate  → window_grid_night
                      if bought → window_normal_night
                      else      → window_disabled_night

  5. Babka "hands up" with weapon (current room, if cooling down = NOT ready_to_fire)
     cur = &rooms[babka_row][babka_col]
     if !cur->ready_to_fire:
         // draw weapon sprite in room
         if armed: draw weapon texture at offset above room
         // draw babka hands up
         DrawTextureV(assets.babka_hands_up, (ROOM_X(col)+23, ROOM_Y(row)+30), WHITE)

  6. Cash pickups
     for each alive cash: DrawTextureV(assets.cash, (x, y), WHITE)

  7. Creatures (road 1 first, then road 0 — render bottom lane over top lane)
     for road in {1, 0}:
         for each alive creature on this road:
             int y = LANE_Y(creature->road)
             Vector2 pos = {creature->x, y}
             if creature->attacking:
                 if HOOLIGAN: frame = anim_frame_once(ANIM_HOOLIGAN_ATTACK, 5, elapsed, 1/ANIM_SPEED_NORMAL)
                 if WHORE:    frame = anim_frame_loop(ANIM_WHORE_ATTACK, 4, elapsed, 1/ANIM_SPEED_NORMAL)
             else:
                 if HOOLIGAN: frame = anim_frame_loop(ANIM_HOOLIGAN_IDLE, 2, state_time, 1/ANIM_SPEED_NORMAL)
                 if WHORE:    frame = anim_frame_loop(ANIM_WHORE_WALK, 4, state_time, 1/ANIM_SPEED_NORMAL)
             sprite_draw_frame(&sheet, frame, pos, WHITE)

  8. Falling weapons
     for each alive weight:
         switch type:
             POT:   DrawTextureV(assets.pot,   (x, y), WHITE)
             TV:    DrawTextureV(assets.tv,    (x, y), WHITE)
             ROYAL: DrawTextureV(assets.royal, (x, y), WHITE)

  9. Crash animations
     for each alive crash:
         int frame = anim_frame_once(seq, count, crash->elapsed, 1/ANIM_SPEED_FAST)
         sprite_draw_frame(&sheet, frame, (crash->x, crash->y), WHITE)

  10. Bullets
      for each alive bullet:
          DrawTextureV(assets.bottle, (bullet->x, bullet->y), WHITE)

  11. HUD overlay
      night_render_hud(g)

  12. Selfie flash overlay (white screen flash)
      if g->selfie_active:
          float alpha = cosf(g->selfie_timer * (PI/2.0f) / SELFIE_DURATION)
          alpha = Clamp(alpha, 0, 1)
          DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(WHITE, alpha))
```

**night_render_hud**:
```c
void night_render_hud(Game *g) {
    Assets *a = &g->assets;

    // 3-layer portrait
    DrawTextureV(a->hud_back,  (Vector2){HUD_X, HUD_Y}, WHITE);

    // Clip hud by hits: draw leftmost (80 * hits) pixels of hud.png
    if (g->hits > 0) {
        int clip = HUD_HIT_WIDTH * g->hits;
        Rectangle src = { 0, 0, (float)clip, (float)HUD_HEIGHT };
        DrawTextureRec(a->hud, src, (Vector2){HUD_X, HUD_Y}, WHITE);
    }

    DrawTextureV(a->hud_front, (Vector2){HUD_X, HUD_Y}, WHITE);

    // Text
    int time_left = (int)(LEVEL_TIME - g->level_time);
    if (time_left < 0) time_left = 0;
    DrawTextEx(a->font_main, TextFormat("~: %d",  g->money),
               (Vector2){MONEY_TEXT_X, MONEY_TEXT_Y}, HUD_FONT_SIZE, 1, WHITE);
    DrawTextEx(a->font_main, TextFormat("time:%d", time_left),
               (Vector2){TIME_TEXT_X,  TIME_TEXT_Y},  HUD_FONT_SIZE, 1, WHITE);
    DrawTextEx(a->font_main, TextFormat("lvl:%d",  g->level),
               (Vector2){LVL_TEXT_X,   LVL_TEXT_Y},   HUD_FONT_SIZE, 1, WHITE);
}
```

---

## 10. day.h / day.c

### day.h

```c
#pragma once
#include "game.h"
#include "command_server.h"

void day_enter(Game *g);
void day_exit(Game *g);
GameState day_update(Game *g, float dt, Command cmd);
void day_render(Game *g);
```

### day.c

```
day_enter(g):
    PlaySound(g->assets.sfx_round_end)
    PlayMusicStream(g->assets.bgm_birds)
    // Cursor starts at first room
    g->day_cursor_row = 1
    g->day_cursor_col = 2
    g->frame_sel_time = 0
    // Reset all room cooldowns (all rooms are on cooldown between night rounds)
    for each room: room->ready_to_fire = false; room->cooldown_timer = 0

day_exit(g):
    StopMusicStream(g->assets.bgm_birds)
    g->level += 1

day_update(g, dt, cmd):
    UpdateMusicStream(g->assets.bgm_birds)
    g->frame_sel_time += dt

    // Cursor navigation (arrow keys)
    // canUseRoom(r,c): rooms[r][c].bought || any adjacent room is bought
    row = g->day_cursor_row
    col = g->day_cursor_col

    if KEY_UP    && row+1 < ROOMS_ROWS && day_can_use_room(g, row+1, col): row++
    if KEY_DOWN  && row-1 >= 0         && day_can_use_room(g, row-1, col): row--
    if KEY_RIGHT && col+1 < ROOMS_COLS && day_can_use_room(g, row, col+1): col++
    if KEY_LEFT  && col  > 0           && day_can_use_room(g, row, col-1): col--

    g->day_cursor_row = row
    g->day_cursor_col = col

    Room *room = &g->rooms[row][col]

    // Shop actions
    if KEY_1:
        if room->bought && room->broken: day_repair(g, room)
        else if !room->bought: day_buy(g, room)

    if KEY_2 && room->bought && !room->broken && !room->grate:
        day_buy_grate(g, room)

    if KEY_3 && room->bought && !room->broken && !room->armed:
        day_buy_weapon(g, room)

    if KEY_4:
        day_buy_club(g)

    // Win: club bought or ENTER to proceed to next night
    if g->club_bought: return STATE_GAMEWIN
    if KEY_ENTER: return STATE_NIGHT

    return STATE_DAY

// day_can_use_room: room at (r,c) is accessible if it or any neighbor is bought
bool day_can_use_room(g, r, c):
    if rooms[r][c].bought: return true
    // check 4 neighbors
    if r+1 < ROWS && rooms[r+1][c].bought: return true
    if r-1 >= 0   && rooms[r-1][c].bought: return true
    if c+1 < COLS && rooms[r][c+1].bought: return true
    if c-1 >= 0   && rooms[r][c-1].bought: return true
    return false

day_buy(g, room):
    if g->money >= room_buy_price(room):
        room->bought = true
        g->money -= room_buy_price(room)
        PlaySound(g->assets.sfx_bye)

day_repair(g, room):
    if g->money >= room_repair_price(room):
        room->broken = false
        g->money -= room_repair_price(room)
        PlaySound(g->assets.sfx_bye)

day_buy_grate(g, room):
    if g->money >= room_grate_price(room):
        room->grate = true
        g->money -= room_grate_price(room)
        PlaySound(g->assets.sfx_bye)

day_buy_weapon(g, room):
    if g->money >= room_weapon_price(room):
        room->armed = true
        g->money -= room_weapon_price(room)
        PlaySound(g->assets.sfx_bye)

day_buy_club(g):
    if g->money >= CLUB_PRICE:
        g->club_bought = true
        g->money -= CLUB_PRICE  // note: Scala deducts from money here
```

**day_render**:
```
day_render(g):
  1. Background
     DrawTextureV(assets.bg_day, (0,0), WHITE)

  2. Club (day, static)
     DrawTextureV(assets.club_day, (CLUB_X, CLUB_Y), WHITE)

  3. Room grid (6×3)
     for each room (row, col):
       a. Weapon-in-window preview (if bought, not broken, armed):
          POT   → DrawTextureV(flower_in_window, (ROOM_X+55, ROOM_Y+30), WHITE)
          TV    → DrawTextureV(tv_in_window,     (ROOM_X+50, ROOM_Y+30), WHITE)
          ROYAL → DrawTextureV(piano_in_window,  (ROOM_X+60, ROOM_Y+30), WHITE)

       b. Window background (light):
          bought && !broken → light_on; !bought && !broken → light_day; broken → skip

       c. Window overlay:
          broken    → window_broken_day
          grate     → window_grid_day
          bought    → window_normal_day
          else      → window_disabled_day

  4. Selection frame (animated, at cursor room)
     int sel_frame = anim_frame_loop(ANIM_FRAME_SEL, 2, g->frame_sel_time, 1/ANIM_SPEED_FRAME)
     sprite_draw_frame(&assets.frame_sel, sel_frame,
                       (ROOM_X(cursor_col), ROOM_Y(cursor_row)), WHITE)

  5. Shop toolbar (4 buttons at bottom of screen)
     Room *room = &rooms[cursor_row][cursor_col]

     // Button 1: Buy or Repair
     if !room->bought:
         DrawTextureV(btn_buy,    (BTN_BUY_X, BTN_BUY_Y), WHITE)
         draw price text in green if affordable, red if not
     else:
         DrawTextureV(btn_repair, (BTN_BUY_X, BTN_BUY_Y), WHITE)
         if room->broken: draw repair price

     // Button 2: Grate
     DrawTextureV(btn_grate, (BTN_GRATE_X, BTN_GRATE_Y), WHITE)
     if room->bought && !room->broken && !room->grate: draw grate price

     // Button 3: Weapon (shows current weapon type)
     Texture2D *weapon_btn = !room->bought ? btn_weapon :
                              room->type == POT ? btn_weapon_pot :
                              room->type == TV  ? btn_weapon_tv  : btn_weapon_royal
     DrawTextureV(*weapon_btn, (BTN_WEAPON_X, BTN_WEAPON_Y), WHITE)
     if room->bought && !room->broken && !room->armed: draw weapon price

     // Button 4: Club
     DrawTextureV(btn_club, (BTN_CLUB_X, BTN_CLUB_Y), WHITE)
     draw club price (green/red)

  6. Money text
     DrawTextEx(font_main, TextFormat("~: %d", g->money), (MONEY_TEXT_X, MONEY_TEXT_Y), ...)
```

---

## 11. main.c — full structure

```c
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define COMMAND_SERVER_IMPLEMENTATION
#include "command_server.h"

#include "const.h"
#include "entity.h"
#include "anim.h"
#include "assets.h"
#include "game.h"
#include "night.h"
#include "day.h"

static Command frame_cmd;

static bool ManagedIsKeyPressed(int key) {
    return IsKeyPressed(key) ||
           (frame_cmd.type == CMD_KEY_PRESS && frame_cmd.key_code == key);
}

static bool ManagedIsMouseButtonPressed(int button) {
    return IsMouseButtonPressed(button) ||
           (frame_cmd.type == CMD_MOUSE_PRESS && frame_cmd.mouse_button == button);
}

int main(void) {
    srand((unsigned)time(NULL));

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Save Soul of Zlaya Babka");
    SetTargetFPS(TARGET_FPS);

    InitAudioDevice();
    command_server_init(CMD_PORT);

    Game game = {0};
    assets_load(&game.assets);
    game.state = STATE_LOGO;
    game.logo_timer = 0;
    game_reset(&game);  // sets money=START_MONEY, level=1, init rooms

    bool running = true;

    while (running && !WindowShouldClose()) {
        // ── Poll TCP command ───────────────────────────────────────────────
        frame_cmd = command_server_poll();
        switch (frame_cmd.type) {
            case CMD_SCREENSHOT:
                TakeScreenshot(frame_cmd.filename);
                command_server_respond(true, "OK");
                break;
            case CMD_MOVE_MOUSE:
                SetMousePosition(frame_cmd.pos.x, frame_cmd.pos.y);
                command_server_respond(true, "OK");
                break;
            case CMD_KEY_PRESS:
            case CMD_MOUSE_PRESS:
                command_server_respond(true, "OK");
                break;
            case CMD_QUIT:
                command_server_respond(true, "OK");
                running = false;
                break;
            case CMD_NONE:
                break;
        }

        float dt = GetFrameTime();

        // ── State update ───────────────────────────────────────────────────
        switch (game.state) {
            case STATE_LOGO:
                game.logo_timer += dt;
                if (game.logo_timer >= LOGO_DURATION ||
                    ManagedIsKeyPressed(KEY_ENTER)) {
                    game.state = STATE_MENU;
                    PlayMusicStream(game.assets.bgm_cool);
                }
                break;

            case STATE_MENU:
                UpdateMusicStream(game.assets.bgm_cool);
                if (ManagedIsKeyPressed(KEY_ENTER)) {
                    StopMusicStream(game.assets.bgm_cool);
                    game.tutorial_page = 0;
                    game.state = STATE_TUTORIAL;
                }
                break;

            case STATE_TUTORIAL:
                if (ManagedIsKeyPressed(KEY_ENTER) ||
                    ManagedIsKeyPressed(KEY_SPACE)) {
                    game.tutorial_page++;
                    if (game.tutorial_page >= TUTORIAL_PAGES) {
                        game.state = STATE_NIGHT;
                        night_enter(&game);
                    }
                }
                break;

            case STATE_NIGHT: {
                GameState next = night_update(&game, dt, frame_cmd);
                if (next != STATE_NIGHT) {
                    night_exit(&game);
                    game.state = next;
                    if (next == STATE_DAY) day_enter(&game);
                }
                break;
            }

            case STATE_DAY: {
                GameState next = day_update(&game, dt, frame_cmd);
                if (next != STATE_DAY) {
                    day_exit(&game);
                    game.state = next;
                    if (next == STATE_NIGHT) night_enter(&game);
                }
                break;
            }

            case STATE_GAMEOVER:
                if (ManagedIsKeyPressed(KEY_ENTER)) {
                    game_reset(&game);
                    game.state = STATE_MENU;
                    PlayMusicStream(game.assets.bgm_cool);
                }
                break;

            case STATE_GAMEWIN:
                if (ManagedIsKeyPressed(KEY_ENTER)) {
                    game_reset(&game);
                    game.state = STATE_MENU;
                    PlayMusicStream(game.assets.bgm_cool);
                }
                break;
        }

        // ── Render ─────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground(BLACK);

        switch (game.state) {
            case STATE_LOGO:
                DrawTextureV(game.assets.logo, (Vector2){0,0}, WHITE);
                break;
            case STATE_MENU:
                DrawTextureV(game.assets.screen_menu, (Vector2){0,0}, WHITE);
                break;
            case STATE_TUTORIAL:
                DrawTextureV(game.assets.tutor[game.tutorial_page],
                             (Vector2){0,0}, WHITE);
                break;
            case STATE_NIGHT:
                night_render(&game);
                break;
            case STATE_DAY:
                day_render(&game);
                break;
            case STATE_GAMEOVER:
                DrawTextureV(game.assets.screen_gameover, (Vector2){0,0}, WHITE);
                break;
            case STATE_GAMEWIN:
                DrawTextureV(game.assets.screen_gamewin, (Vector2){0,0}, WHITE);
                break;
        }

        EndDrawing();
    }

    // ── Cleanup ────────────────────────────────────────────────────────────
    assets_unload(&game.assets);
    CloseAudioDevice();
    command_server_cleanup();
    CloseWindow();
    return 0;
}
```

---

## 12. Memory Management Strategy

**Pattern**: Fixed-size arrays with `alive` flag + compact-after-update.

```c
// Generic compact: removes !alive entries by swapping with tail
// Works for any struct with a bool `alive` field as first member
// In practice, implement per-type for type safety:

static void compact_creatures(Game *g) {
    int j = 0;
    for (int i = 0; i < g->creature_count; i++)
        if (g->creatures[i].alive) g->creatures[j++] = g->creatures[i];
    g->creature_count = j;
}
// Same pattern for bullets, weights, crashes, cash_pickups
```

**Array sizes** (all fixed at compile time, on the stack inside `Game`):
| Array | Max size | Typical live count |
|-------|----------|--------------------|
| rooms | 18 (3×6) | always 18 |
| creatures | 32 | 2–8 |
| bullets | 64 | 0–16 |
| weights | 16 | 0–3 |
| crashes | 32 | 0–8 |
| cash_pickups | 32 | 0–8 |

Total `Game` struct size: ~few KB — fits comfortably on stack.

---

## 13. Build System

**No changes to CMakeLists.txt required.** The existing `file(GLOB_RECURSE SOURCES src/*.c)` automatically includes all new `.c` files. All `.h` files in `src/` are also included.

Verify the build picks up new files after creating them:
```bash
make build   # re-runs cmake configure + build
```

If assets path is incorrect at runtime, the working directory must be `game/` (where `assets/` folder lives). The Makefile should run `./build/Game` from `game/` directory.

---

## 14. Implementation Milestones

### Milestone 1 — Foundation (no gameplay)
**Deliverable**: App runs, assets load, screens display, state machine navigates.

- [ ] Create `const.h` with all macros
- [ ] Create `entity.h` with all struct definitions
- [ ] Create `anim.h` as header-only animation module
- [ ] Create `assets.h` + `assets.c` — load/unload all assets
- [ ] Create `game.h` + `game.c` — `game_init`, `game_reset`, `game_init_rooms`
- [ ] Update `main.c`:
  - Window: 1366×768
  - InitAudioDevice
  - Load assets
  - State machine: LOGO → MENU → TUTORIAL (all 3 pages) → NIGHT stub → GAMEOVER → back to MENU
  - Render logo, menu, tutorial[0..2], gameover, gamewin screens
  - Music: bgm_cool on menu/logo
- [ ] Verify all textures display at correct screen positions

### Milestone 2 — Day Phase
**Deliverable**: Full day phase with shop UI working.

- [ ] Create `day.h` + `day.c`
- [ ] Implement `day_render`: bg_day, club_day, room grid, selection frame, shop buttons
- [ ] Implement `day_update`: cursor navigation, buy/repair/grate/weapon/club actions
- [ ] HUD: money text overlay
- [ ] Win via club purchase → STATE_GAMEWIN
- [ ] ENTER advances to STATE_NIGHT (stubbed)
- [ ] Birds music in day phase

### Milestone 3 — Night Phase Static
**Deliverable**: Night phase renders building + babka, no enemies yet.

- [ ] Create `night.h` + `night.c`
- [ ] `night_enter` / `night_exit`
- [ ] `night_render`: bg_night, club animation, window grid (all 4 states × day/night), babka-in-window, weapon-in-window previews
- [ ] `night_render_hud`: 3-layer HUD portrait, money, timer, level
- [ ] Player input: babka moves between rooms (arrow keys)
- [ ] Weapon firing (space): spawn weight, room cooldown timer
- [ ] Weight physics: fall, ground crash with crash animation + sound
- [ ] Timer countdown → transition to STATE_DAY after 30s
- [ ] Crickets music

### Milestone 4 — Enemy AI
**Deliverable**: Enemies spawn, move, attack.

- [ ] Creature spawning (generator timer, lanes 0 + 1, random types)
- [ ] Creature movement (x += speed * dt)
- [ ] Attack system (cooldown timer → trigger attack → reset)
- [ ] Hooligan bullet spawning → bullet physics → room damage (broken windows, grate damage)
- [ ] Whore selfie flash effect (white overlay)
- [ ] Club threshold hit detection → g->hits++, bgm_cool volume
- [ ] Game over at 5 hits → STATE_GAMEOVER

### Milestone 5 — Kills and Combat
**Deliverable**: Weapons kill enemies, money accumulates.

- [ ] Weight–creature collision (hitL0 / hitL1 zones)
- [ ] Kill: death crash animation (hooligan_die / whore_die frame sequence)
- [ ] Cash pickup spawn + timed removal
- [ ] Money reward on kill
- [ ] Difficulty scaling tested across levels 1–5+

### Milestone 6 — Polish
**Deliverable**: Full game loop, all audio, tuned positions.

- [ ] All SFX hooked up (pot_destroy, tv_destroy, royal_deploy, window_broken, iron, selfie, bye, round_end)
- [ ] bgm_cool scaling with hit count
- [ ] Selfie delay (SELFIE_DELAY before flash activates)
- [ ] Day phase: price text green/red based on affordability
- [ ] Pixel-perfect position tuning from actual texture dimensions
- [ ] Full Night→Day→Night cycle with level progression
- [ ] End-to-end test: complete game from LOGO to GAMEWIN
- [ ] Update PROJECT_INDEX.md

---

## 15. Key Implementation Notes

### GIF Loading
All `.gif` assets are **sprite atlas images** (single-frame GIFs, sprites arranged horizontally). Use `LoadTexture()` for all GIFs. Do NOT use `LoadImageAnim()` — the sprite sheets are NOT multi-frame animated GIFs.

### Font Color (HUD)
Original Scala uses a purple-ish color `(167/255, 128/255, 183/255)`. In Raylib:
```c
Color HUD_FONT_COLOR = { 167, 128, 183, 255 };
```

### Day/Night Cursor Direction
In libgdx (Y-up), KEY_UP increases row index. In Raylib (Y-down), a higher row index is visually higher on screen (ROOM_Y decreases with row). Mapping:
- KEY_UP → babka_row+1 (move to higher floor = smaller ROOM_Y = top of screen)
- KEY_DOWN → babka_row-1 (move to lower floor)

### Music Lifecycle
Music streams require `UpdateMusicStream()` every frame to fill the audio buffer. Call it once per frame in the active state's update function. When transitioning states, call `StopMusicStream()` on the old music before `PlayMusicStream()` on the new.

### ManagedIsKeyPressed
Keep the existing pattern from main.c. Pass `frame_cmd` into `night_update` and `day_update`, and use `ManagedIsKeyPressed` defined in main.c (or move it to a shared header if needed). Since it's `static`, it stays in main.c and is invoked only from main.c's state dispatcher — night/day update functions receive the pre-decoded `frame_cmd` struct.

### Babka `ready_to_fire` vs Scala `cooldown`
In Scala, `room.cooldown = true` means the room is **ready to fire**. This naming is counterintuitive. In C, `room->ready_to_fire = true` clearly states the same. The Night rendering in Scala showed babka-in-window when `room.cooldown = true` (= ready), and showed babka-hands-up when `room.cooldown = false` (= waiting). In C:
- `ready_to_fire = true` → babka in window (idle/waiting pose)
- `ready_to_fire = false` → babka hands up + weapon displayed above (just fired, in cooldown)
