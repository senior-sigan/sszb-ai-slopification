#include <math.h>
#include <raylib.h>
#include <string.h>

#include "game_types.h"

Rectangle sprite_anim_frame(const SpriteAnim* anim, float time) {
  int total = anim->index_count;
  if (total == 0) {
    return (Rectangle) {0, 0, 0, 0};
  }
  float total_dur = (float) total * anim->frame_duration;
  int frame_idx;
  if (anim->looping) {
    float mod_time = fmodf(time, total_dur);
    frame_idx = (int) (mod_time / anim->frame_duration);
  } else {
    frame_idx = (int) (time / anim->frame_duration);
  }
  if (frame_idx >= total) {
    frame_idx = total - 1;
  }
  if (frame_idx < 0) {
    frame_idx = 0;
  }
  int col = anim->frame_indices[frame_idx];
  return (Rectangle) {.x = (float) (col * anim->frame_width),
                      .y = 0,
                      .width = (float) anim->frame_width,
                      .height = (float) anim->frame_height};
}

float sprite_anim_duration(const SpriteAnim* anim) {
  return (float) anim->index_count * anim->frame_duration;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void sprite_anim_setup(SpriteAnim* anim, Texture2D sheet, int frame_w, int frame_h, const int* idx, int count,
                       float dur, bool loop) {
  anim->sheet = sheet;
  anim->frame_width = frame_w;
  anim->frame_height = frame_h;
  anim->index_count = count > 16 ? 16 : count;
  for (int i = 0; i < anim->index_count; i++) {
    anim->frame_indices[i] = idx[i];
  }
  anim->frame_duration = dur;
  anim->looping = loop;
}

float room_cooldown_time(const Room* room) {
  switch (room->type) {
    case ROOM_POT:
      return POT_COOLDOWN;
    case ROOM_TV:
      return TV_COOLDOWN;
    case ROOM_ROYAL:
      return ROYAL_COOLDOWN;
  }
  return 1.0f;
}

int room_repair_price(const Room* room) {
  return room->base_price * REPAIR_MUL;
}
int room_buy_price(const Room* room) {
  return room->base_price * BUY_MUL;
}
int room_grate_price(const Room* room) {
  return room->base_price * GRATE_MUL;
}

int room_weapon_price(const Room* room) {
  switch (room->type) {
    case ROOM_POT:
      return room->base_price * POT_WEAPON_MUL;
    case ROOM_TV:
      return room->base_price * TV_WEAPON_MUL;
    case ROOM_ROYAL:
      return room->base_price * ROYAL_WEAPON_MUL;
  }
  return room->base_price * POT_WEAPON_MUL;
}

int difficulty_hooligan_speed(int lvl) {
  return 65 + lvl * 10 + rand() % (lvl * 10 > 0 ? lvl * 10 : 1);
}

float difficulty_hooligan_cooldown(int lvl) {
  switch (lvl) {
    case 1:
      return 10.0f;
    case 2:
      return 8.0f;
    case 3:  // NOLINT(bugprone-branch-clone)
      return 3.0f + (float) (rand() % 4);
    default:
      return 2.0f + (float) (rand() % 3);
  }
}

int difficulty_whore_speed(int lvl) {
  switch (lvl) {
    case 1:
    case 2:
      return 125;
    case 3:
      return 150;
    default:
      return 100 + lvl * 10 + rand() % (lvl * 10 > 0 ? lvl * 10 : 1);
  }
}

float difficulty_whore_cooldown(int lvl) {
  switch (lvl) {
    case 1:  // NOLINT(bugprone-branch-clone)
    case 2:
      return 3.0f + (float) (rand() % 4);
    default:
      return 2.0f + (float) (rand() % 6);
  }
}

float difficulty_generator_timer(int lvl) {
  switch (lvl) {
    case 1:
      return 2.2f;
    case 2:
      return 2.0f;
    case 3:
    case 4:
      return 1.8f;
    default:
      return 1.0f;
  }
}

void difficulty_spawn_random(int lvl, int raw,  // NOLINT(bugprone-easily-swappable-parameters)
                             bool* road0, bool* road1) {
  // Match Scala's rand.nextInt() % 10 behavior:
  // Scala's nextInt() returns signed ints, so % 10 gives [-9, 9].
  // Negative seeds fall through all range checks to (true, true).
  // C's rand() is always non-negative, so we simulate the ~50% negative
  // probability by using one bit of the raw value as a sign flag.
  int seed = (raw >> 1) % 10;
  bool negative = (raw & 1);

  if (negative && lvl >= 1 && lvl <= 5) {
    *road0 = true;
    *road1 = true;
    return;
  }

  switch (lvl) {
    case 1:
      if (seed <= 4) {
        *road0 = true;
        *road1 = false;
      } else {
        *road0 = false;
        *road1 = true;
      }
      break;
    case 2:
      if (seed <= 4) {
        *road0 = false;
        *road1 = true;
      } else {
        *road0 = true;
        *road1 = false;
      }
      break;
    case 3:
      if (seed <= 3) {
        *road0 = false;
        *road1 = true;
      } else if (seed <= 8) {
        *road0 = true;
        *road1 = false;
      } else {
        *road0 = true;
        *road1 = true;
      }
      break;
    case 4:
      if (seed <= 2) {
        *road0 = false;
        *road1 = true;
      } else if (seed <= 5) {
        *road0 = true;
        *road1 = false;
      } else {
        *road0 = true;
        *road1 = true;
      }
      break;
    case 5:
      if (seed <= 1) {
        *road0 = false;
        *road1 = true;
      } else if (seed <= 3) {
        *road0 = true;
        *road1 = false;
      } else {
        *road0 = true;
        *road1 = true;
      }
      break;
    default:
      *road0 = true;
      *road1 = true;
      break;
  }
}

// House layout from RenderFactory.scala
// Row 0: (TV,4) (Pot,4) (Pot,3) (Pot,3) (TV,3) (Royal,4)
// Row 1: (Pot,4) (Pot,3) (TV,3) (TV,2) (Pot,2) (Royal,3)
// Row 2: (Royal,3) (Royal,3) (Pot,1) (Royal,2) (TV,2) (Pot,1)
void game_init_house(Room rooms[BUILDING_ROWS][BUILDING_COLS]) {
  typedef struct {
    RoomType t;
    int p;
  } RP;
  static const RP layout[3][6] = {
      {{ROOM_TV, 4}, {ROOM_POT, 4}, {ROOM_POT, 3}, {ROOM_POT, 3}, {ROOM_TV, 3}, {ROOM_ROYAL, 4}},
      {{ROOM_POT, 4}, {ROOM_POT, 3}, {ROOM_TV, 3}, {ROOM_TV, 2}, {ROOM_POT, 2}, {ROOM_ROYAL, 3}},
      {{ROOM_ROYAL, 3}, {ROOM_ROYAL, 3}, {ROOM_POT, 1}, {ROOM_ROYAL, 2}, {ROOM_TV, 2}, {ROOM_POT, 1}},
  };
  for (int i = 0; i < BUILDING_ROWS; i++) {
    for (int j = 0; j < BUILDING_COLS; j++) {
      rooms[i][j] = (Room) {
          .type = layout[i][j].t,
          .base_price = layout[i][j].p,
          .bought = false,
          .broken = false,
          .grate = false,
          .armed = false,
          .cooldown_ready = true,
          .cooldown_timer = 0,
          .grate_lives = GRATE_LIVES_INIT,
      };
    }
  }
  // Starting rooms: (1,2) and (2,2) bought + armed
  rooms[1][2].bought = true;
  rooms[1][2].armed = true;
  rooms[2][2].bought = true;
  rooms[2][2].armed = true;
}

void game_reset(Game* game) {
  game->state = STATE_LOGO;
  game->state_timer = 0;
  game->level = 1;
  game->level_time = 0;
  game->hits = 0;
  game->money = START_MONEY;
  game->club_bought = false;
  game->cur_row = 1;
  game->cur_col = 2;
  game->spawn_timer = 0;
  game->selfie_active = false;
  game->selfie_pending = false;
  game->selfie_alpha = 0;
  game->selfie_time = 0;
  game->club_anim_time = 0;
  game->frame_anim_time = 0;
  game_init_house(game->rooms);
  memset(game->creatures, 0, sizeof(game->creatures));
  memset(game->weights, 0, sizeof(game->weights));
  memset(game->bullets, 0, sizeof(game->bullets));
  memset(game->anims, 0, sizeof(game->anims));
  memset(game->cash, 0, sizeof(game->cash));
}
