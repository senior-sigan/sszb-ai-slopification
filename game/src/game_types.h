#pragma once
#include <raylib.h>
#include <stdbool.h>

// ===== CONSTANTS =====
#define SCREEN_WIDTH 1366
#define SCREEN_HEIGHT 768
#define CMD_PORT 9999

#define BUILDING_ROWS 3
#define BUILDING_COLS 6
#define ROOM_SIZE 128

#define MAX_CREATURES 32
#define MAX_WEIGHTS 16
#define MAX_BULLETS 32
#define MAX_ANIMATIONS 32
#define MAX_CASH_SPRITES 32

#define LEVEL_TIME 30.0f
#define WEIGHT_SPEED 400
#define BOTTLE_SPEED 250
#define GRATE_LIVES_INIT 2
#define START_MONEY 10
#define CLUB_PRICE 400

#define POT_COOLDOWN 1.0f
#define TV_COOLDOWN 2.0f
#define ROYAL_COOLDOWN 3.0f

#define POT_W 48
#define POT_H 64
#define TV_W 92
#define TV_H 96
#define ROYAL_W 128
#define ROYAL_H 108
#define HOOLIGAN_W 92
#define WHORE_W 88
#define CREATURE_H 128

#define HIT_L0 192.0f
#define HIT_L1 128.0f
#define GROUND_LVL 64.0f

#define GRATE_MUL 15
#define REPAIR_MUL 5
#define BUY_MUL 10
#define ROYAL_WEAPON_MUL 20
#define TV_WEAPON_MUL 10
#define POT_WEAPON_MUL 5

#define HOOLIGAN_REWARD 7
#define WHORE_REWARD 6

#define ANIM_FAST 0.1f
#define ANIM_NORMAL 0.2f

#define SELFIE_DELAY 0.5f
#define BOTTOM_LANE 16
#define TOP_LANE 122
#define CLUB_X 1010
#define CLUB_Y_GDX 100

// Coordinate conversion: libGDX Y-up to raylib Y-down
#define FLIP_Y(y, h) (SCREEN_HEIGHT - (y) - (h))
#define ROOM_GDX_X(col) (((col) * 128) + 128)
#define ROOM_GDX_Y(row) (((row) * 128) + 256)

// ===== ENUMS =====
typedef enum {
  STATE_LOGO,
  STATE_MENU,
  STATE_TUTOR1,
  STATE_TUTOR2,
  STATE_TUTOR3,
  STATE_NIGHT,
  STATE_DAY,
  STATE_WIN,
  STATE_OVER
} GameState;

typedef enum { ROOM_POT, ROOM_TV, ROOM_ROYAL } RoomType;
typedef enum { CREATURE_HOOLIGAN, CREATURE_WHORE } CreatureType;
typedef enum { WEIGHT_POT, WEIGHT_TV, WEIGHT_ROYAL } WeightType;
typedef enum {
  ANIM_HOOLIGAN_DIE,
  ANIM_WHORE_DIE,
  ANIM_POT_CRASH,
  ANIM_TV_CRASH,
  ANIM_ROYAL_CRASH,
  ANIM_TYPE_COUNT
} AnimType;

// ===== STRUCTS =====
typedef struct {
  RoomType type;
  bool bought, broken, grate, armed;
  bool cooldown_ready;
  float cooldown_timer;
  int base_price;
  int grate_lives;
} Room;

typedef struct {
  CreatureType type;
  float x, width;
  int speed, health;
  bool cooldown_ready;
  float cooldown_timer;
  bool attacking;
  float state_time;
  int road;
  bool active;
} Creature;

typedef struct {
  WeightType type;
  float x, y;
  int speed;
  bool active;
  bool hit_l0;
  bool hit_l1;
} Weight;

typedef struct {
  float x, y;
  float target_x, target_y;
  int target_row, target_col;
  int speed;
  bool active;
} Bullet;

typedef struct {
  AnimType type;
  float x, y;
  float state_time;
  bool active;
} AnimEffect;

typedef struct {
  float x, y;
  bool active;
} CashSprite;

typedef struct {
  Texture2D sheet;
  int frame_width, frame_height;
  int frame_indices[16];
  int index_count;
  float frame_duration;
  bool looping;
} SpriteAnim;

typedef struct {
  Texture2D logo, menu, tutor1, tutor2, tutor3, game_over, game_win;
  Texture2D bg_night, bg_day, club_day;
  Texture2D wnd_night_normal, wnd_night_broken, wnd_night_grate, wnd_night_disabled;
  Texture2D wnd_day_normal, wnd_day_broken, wnd_day_grate, wnd_day_disabled;
  Texture2D flower_in_window, tv_in_window, piano_in_window;
  Texture2D light_on, light_off, light_day;
  Texture2D babka_in_window, babka_hands_up;
  Texture2D pot_tex, tv_tex, royal_tex;
  Texture2D bottle;
  Texture2D cash;
  Texture2D hud_back, hud_main, hud_front;
  Texture2D shop_buy, shop_repair, shop_grate, shop_weapon;
  Texture2D shop_weapon_pot, shop_weapon_tv, shop_weapon_royal, shop_club;
  Texture2D whore_sheet, gopstop_sheet;
  Texture2D pot_crash_sheet, tv_crash_sheet, piano_crash_sheet;
  Texture2D club_night_sheet, frame_sheet;
  SpriteAnim anim_whore_walk, anim_whore_attack;
  SpriteAnim anim_hooligan_walk, anim_hooligan_attack;
  SpriteAnim anim_hooligan_die, anim_whore_die;
  SpriteAnim anim_pot_crash, anim_tv_crash, anim_royal_crash;
  SpriteAnim anim_club_night, anim_frame;
  Font main_font;
  Music bgm_cool, bgm_crickets, bgm_birds;
  Sound snd_pot_destroy, snd_tv_destroy, snd_royal_deploy;
  Sound snd_window_broken, snd_selfie, snd_iron, snd_bye, snd_round_end;
} GameAssets;

typedef struct {
  GameState state;
  float state_timer;
  Room rooms[BUILDING_ROWS][BUILDING_COLS];
  int cur_row, cur_col;
  int level;
  float level_time;
  int hits;
  int money;
  bool club_bought;
  Creature creatures[MAX_CREATURES];
  Weight weights[MAX_WEIGHTS];
  Bullet bullets[MAX_BULLETS];
  AnimEffect anims[MAX_ANIMATIONS];
  CashSprite cash[MAX_CASH_SPRITES];
  float spawn_timer;
  bool selfie_active;
  bool selfie_pending;
  float selfie_alpha, selfie_time;
  float club_anim_time, frame_anim_time;
  GameAssets assets;
} Game;

// ===== FUNCTION DECLARATIONS =====
Rectangle SpriteAnimFrame(const SpriteAnim* anim, float time);
float SpriteAnimDuration(const SpriteAnim* anim);
void SpriteAnimSetup(SpriteAnim* anim, Texture2D sheet, int frame_w, int frame_h, const int* idx, int count, float dur,
                     bool loop);

float RoomCooldownTime(const Room* room);
int RoomRepairPrice(const Room* room);
int RoomBuyPrice(const Room* room);
int RoomWeaponPrice(const Room* room);
int RoomGratePrice(const Room* room);

int DifficultyHooliganSpeed(int lvl);
float DifficultyHooliganCooldown(int lvl);
int DifficultyWhoreSpeed(int lvl);
float DifficultyWhoreCooldown(int lvl);
float DifficultyGeneratorTimer(int lvl);
void DifficultySpawnRandom(int lvl, int raw, bool* road0, bool* road1);

void GameInitHouse(Room rooms[BUILDING_ROWS][BUILDING_COLS]);
void GameReset(Game* game);
