#include "night.h"

#include <math.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>

#include "game_types.h"

// ManagedIsKeyPressed is defined in main.c and combines physical keyboard
// input with TCP-injected key presses (for automated testing).
extern bool ManagedIsKeyPressed(int key);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool CanUseRoom(const Game* game, int row, int col) {
  return game->rooms[row][col].bought && !game->rooms[row][col].broken;
}

static float WeightWidth(WeightType type) {
  switch (type) {
    case WEIGHT_POT:
      return POT_W;
    case WEIGHT_TV:
      return TV_W;
    case WEIGHT_ROYAL:
      return ROYAL_W;
  }
  return POT_W;
}

// AABB overlap test (1D projection on x-axis).
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static bool AabbOverlapX(float pos_a, float width_a, float pos_b, float width_b) {
  return pos_a < pos_b + width_b && pos_a + width_a > pos_b;
}

// Returns a pointer to the SpriteAnim used for crash effects.
static const SpriteAnim* AnimForType(const GameAssets* assets, AnimType type) {
  switch (type) {
    case ANIM_HOOLIGAN_DIE:
      return &assets->anim_hooligan_die;
    case ANIM_WHORE_DIE:
      return &assets->anim_whore_die;
    case ANIM_POT_CRASH:
      return &assets->anim_pot_crash;
    case ANIM_TV_CRASH:
      return &assets->anim_tv_crash;
    case ANIM_ROYAL_CRASH:
      return &assets->anim_royal_crash;
    default:
      break;
  }
  return &assets->anim_pot_crash;
}

// Spawn a new AnimEffect into the first free slot.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void SpawnAnim(Game* game, AnimType type, float pos_x, float pos_y) {
  for (int idx = 0; idx < MAX_ANIMATIONS; idx++) {
    if (!game->anims[idx].active) {
      game->anims[idx] = (AnimEffect) {.type = type, .x = pos_x, .y = pos_y, .state_time = 0, .active = true};
      return;
    }
  }
}

// Spawn a CashSprite into the first free slot.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void SpawnCash(Game* game, float pos_x, float pos_y) {
  for (int idx = 0; idx < MAX_CASH_SPRITES; idx++) {
    if (!game->cash[idx].active) {
      game->cash[idx] = (CashSprite) {.x = pos_x, .y = pos_y, .active = true};
      return;
    }
  }
}

// Kill a creature and award money / effects.
static void KillCreature(Game* game, Creature* creature) {
  float lane_y = (creature->road == 0) ? BOTTOM_LANE : TOP_LANE;
  if (creature->type == CREATURE_HOOLIGAN) {
    SpawnAnim(game, ANIM_HOOLIGAN_DIE, creature->x, lane_y);
    game->money += HOOLIGAN_REWARD;
  } else {
    SpawnAnim(game, ANIM_WHORE_DIE, creature->x, lane_y);
    game->money += WHORE_REWARD;
  }
  SpawnCash(game, creature->x, lane_y - 10 + (float) (rand() % 20));
  creature->active = false;
}

// Reset room cooldowns for all rooms.
static void ResetRoomCooldowns(Game* game) {
  for (int row = 0; row < BUILDING_ROWS; row++) {
    for (int col = 0; col < BUILDING_COLS; col++) {
      game->rooms[row][col].cooldown_ready = true;
      game->rooms[row][col].cooldown_timer = 0;
    }
  }
}

// Clear all dynamic entity arrays.
static void ClearEntities(Game* game) {
  memset(game->creatures, 0, sizeof(game->creatures));
  memset(game->weights, 0, sizeof(game->weights));
  memset(game->bullets, 0, sizeof(game->bullets));
  memset(game->anims, 0, sizeof(game->anims));
  memset(game->cash, 0, sizeof(game->cash));
}

// ---------------------------------------------------------------------------
// Update subsystems (extracted to reduce cognitive complexity)
// ---------------------------------------------------------------------------

static void UpdateInputHandling(Game* game) {
  int row = game->cur_row;
  int col = game->cur_col;

  if (ManagedIsKeyPressed(KEY_DOWN) && row > 0 && CanUseRoom(game, row - 1, col)) {
    game->cur_row = row - 1;
  }
  if (ManagedIsKeyPressed(KEY_UP) && row < BUILDING_ROWS - 1 && CanUseRoom(game, row + 1, col)) {
    game->cur_row = row + 1;
  }
  if (ManagedIsKeyPressed(KEY_LEFT) && col > 0 && CanUseRoom(game, row, col - 1)) {
    game->cur_col = col - 1;
  }
  if (ManagedIsKeyPressed(KEY_RIGHT) && col < BUILDING_COLS - 1 && CanUseRoom(game, row, col + 1)) {
    game->cur_col = col + 1;
  }

  // Cheat: skip to day
  if (ManagedIsKeyPressed(KEY_L)) {
    game->level_time = LEVEL_TIME + 1.0f;
  }

  // Cheat: add hit
  if (ManagedIsKeyPressed(KEY_P)) {
    game->hits++;
    if (game->hits > 5) {
      game->hits = 5;
    }
  }

  // Force game over
  if (ManagedIsKeyPressed(KEY_ESCAPE)) {
    game->hits = 5;
  }
}

static void UpdateFireWeapon(Game* game) {
  if (!ManagedIsKeyPressed(KEY_SPACE)) {
    return;
  }
  Room* room = &game->rooms[game->cur_row][game->cur_col];
  if (!room->armed || !room->cooldown_ready || room->broken) {
    return;
  }
  // Find free weight slot
  for (int idx = 0; idx < MAX_WEIGHTS; idx++) {
    if (!game->weights[idx].active) {
      WeightType weapon_type;
      switch (room->type) {
        case ROOM_POT:
          weapon_type = WEIGHT_POT;
          break;
        case ROOM_TV:
          weapon_type = WEIGHT_TV;
          break;
        case ROOM_ROYAL:
          weapon_type = WEIGHT_ROYAL;
          break;
      }
      game->weights[idx] = (Weight) {.type = weapon_type,
                                     .x = (float) ((game->cur_col * 128) + 128),
                                     .y = (float) ((game->cur_row * 128) + 256),
                                     .speed = WEIGHT_SPEED,
                                     .active = true,
                                     .hit_l0 = false,
                                     .hit_l1 = false};
      break;
    }
  }
  room->cooldown_ready = false;
  room->cooldown_timer = room_cooldown_time(room);
}

static void SpawnCreatures(Game* game) {
  if (game->spawn_timer < difficulty_generator_timer(game->level)) {
    return;
  }
  game->spawn_timer = 0;
  bool spawn_r0 = false;
  bool spawn_r1 = false;
  difficulty_spawn_random(game->level, rand(), &spawn_r0, &spawn_r1);

  for (int lane = 0; lane < 2; lane++) {
    if ((lane == 0 && !spawn_r0) || (lane == 1 && !spawn_r1)) {
      continue;
    }
    // Find free creature slot
    for (int idx = 0; idx < MAX_CREATURES; idx++) {
      if (!game->creatures[idx].active) {
        bool is_hooligan = (rand() % 2 == 1);
        Creature* creature = &game->creatures[idx];
        creature->active = true;
        creature->road = lane;
        creature->x = 0;
        creature->attacking = false;
        creature->state_time = 0;
        creature->cooldown_ready = false;
        creature->health = 1;
        if (is_hooligan) {
          creature->type = CREATURE_HOOLIGAN;
          creature->width = HOOLIGAN_W;
          creature->speed = difficulty_hooligan_speed(game->level);
          creature->cooldown_timer = difficulty_hooligan_cooldown(game->level) / 2.0f;
        } else {
          creature->type = CREATURE_WHORE;
          creature->width = WHORE_W;
          creature->speed = difficulty_whore_speed(game->level);
          creature->cooldown_timer = difficulty_whore_cooldown(game->level);
        }
        break;
      }
    }
  }
}

static void UpdateCreatureAttack(Game* game, Creature* creature) {
  creature->attacking = true;
  creature->state_time = 0;
  creature->cooldown_ready = false;

  if (creature->type == CREATURE_HOOLIGAN) {
    // Hooligan: shoot a bullet at a random bought, non-broken room
    int valid_rows[BUILDING_ROWS * BUILDING_COLS];
    int valid_cols[BUILDING_ROWS * BUILDING_COLS];
    int valid_count = 0;
    for (int row = 0; row < BUILDING_ROWS; row++) {
      for (int col = 0; col < BUILDING_COLS; col++) {
        if (game->rooms[row][col].bought && !game->rooms[row][col].broken) {
          valid_rows[valid_count] = row;
          valid_cols[valid_count] = col;
          valid_count++;
        }
      }
    }
    if (valid_count > 0) {
      int target_idx = rand() % valid_count;
      int target_row = valid_rows[target_idx];
      int target_col = valid_cols[target_idx];
      float target_x = (float) ((target_col * 128) + 128 + 64);
      float target_y = (float) ((target_row * 128) + 256 + 64);
      for (int bid = 0; bid < MAX_BULLETS; bid++) {
        if (!game->bullets[bid].active) {
          game->bullets[bid] = (Bullet) {.x = creature->x,
                                         .y = (float) (creature->road * 128),
                                         .target_x = target_x,
                                         .target_y = target_y,
                                         .target_row = target_row,
                                         .target_col = target_col,
                                         .speed = BOTTLE_SPEED,
                                         .active = true};
          break;
        }
      }
    }
    creature->cooldown_timer = difficulty_hooligan_cooldown(game->level);
  } else {
    // Whore: selfie effect (with 0.5s delay before flash)
    PlaySound(game->assets.snd_selfie);
    if (game->cur_row != 2) {
      game->selfie_pending = true;
      game->selfie_time = 0;
    }
    creature->cooldown_timer = difficulty_whore_cooldown(game->level);
  }
}

static void UpdateCreatures(Game* game, float delta) {
  for (int idx = 0; idx < MAX_CREATURES; idx++) {
    Creature* creature = &game->creatures[idx];
    if (!creature->active) {
      continue;
    }

    // Cooldown countdown
    if (!creature->cooldown_ready) {
      creature->cooldown_timer -= delta;
      if (creature->cooldown_timer <= 0) {
        creature->cooldown_ready = true;
      }
    }

    if (creature->cooldown_ready) {
      UpdateCreatureAttack(game, creature);
    }

    // Move forward (creatures keep walking even while attacking)
    if (!creature->cooldown_ready) {
      creature->x += (float) creature->speed * delta;
    }

    // Attacking animation timeout
    if (creature->attacking) {
      creature->state_time += delta;
      float anim_dur;
      if (creature->type == CREATURE_HOOLIGAN) {
        anim_dur = 5 * ANIM_NORMAL;  // 5 frames * 0.2s = 1.0s
      } else {
        anim_dur = 4 * ANIM_NORMAL;  // 4 frames * 0.2s = 0.8s
      }
      if (creature->state_time >= anim_dur) {
        creature->attacking = false;
        creature->state_time = 0;
      }
    }

    // Advance walk animation state_time (attacking is advanced above)
    if (!creature->attacking) {
      creature->state_time += delta;
    }
  }
}

static void UpdateBullets(Game* game, float delta) {
  for (int idx = 0; idx < MAX_BULLETS; idx++) {
    Bullet* bullet = &game->bullets[idx];
    if (!bullet->active) {
      continue;
    }

    float angle = atan2f(bullet->target_y - bullet->y, bullet->target_x - bullet->x);
    bullet->x += (float) bullet->speed * delta * cosf(angle);
    bullet->y += (float) bullet->speed * delta * sinf(angle);

    if (fabsf(bullet->x - bullet->target_x) < 5.0f && fabsf(bullet->y - bullet->target_y) < 5.0f) {
      Room* target_room = &game->rooms[bullet->target_row][bullet->target_col];
      if (bullet->target_row == game->cur_row && bullet->target_col == game->cur_col) {
        // Dodge: player is in the target room
      } else if (target_room->grate) {
        target_room->grate_lives--;
        PlaySound(game->assets.snd_iron);
        if (target_room->grate_lives <= 0) {
          target_room->grate = false;
          target_room->grate_lives = GRATE_LIVES_INIT;
        }
      } else {
        target_room->broken = true;
        PlaySound(game->assets.snd_window_broken);
      }
      bullet->active = false;
    }
  }
}

// Check collision between a weight and creatures on the top lane (road 1).
static void CheckWeightCollisionTopLane(Game* game, Weight* weight, float width) {
  for (int cid = 0; cid < MAX_CREATURES; cid++) {
    Creature* creature = &game->creatures[cid];
    if (!creature->active || creature->road != 1) {
      continue;
    }
    if (AabbOverlapX(weight->x, width, creature->x, creature->width)) {
      KillCreature(game, creature);
    }
  }
}

// Check collision between a weight and all creatures on both lanes.
static void CheckWeightCollisionAllLanes(Game* game, Weight* weight, float width) {
  for (int cid = 0; cid < MAX_CREATURES; cid++) {
    Creature* creature = &game->creatures[cid];
    if (!creature->active) {
      continue;
    }
    if (AabbOverlapX(weight->x, width, creature->x, creature->width)) {
      KillCreature(game, creature);
    }
  }
}

// Handle weight hitting the ground: play sound and spawn crash animation.
static void WeightHitGround(Game* game, Weight* weight) {
  switch (weight->type) {
    case WEIGHT_POT:
      PlaySound(game->assets.snd_pot_destroy);
      SpawnAnim(game, ANIM_POT_CRASH, weight->x - 8, weight->y);
      break;
    case WEIGHT_TV:
      PlaySound(game->assets.snd_tv_destroy);
      SpawnAnim(game, ANIM_TV_CRASH, weight->x - 34, weight->y);
      break;
    case WEIGHT_ROYAL:
      PlaySound(game->assets.snd_royal_deploy);
      SpawnAnim(game, ANIM_ROYAL_CRASH, weight->x - 30, weight->y);
      break;
  }
  weight->active = false;
}

static void UpdateWeights(Game* game, float delta) {
  for (int idx = 0; idx < MAX_WEIGHTS; idx++) {
    Weight* weight = &game->weights[idx];
    if (!weight->active) {
      continue;
    }

    // Fall downward (libGDX: y decreases = falling)
    weight->y -= (float) weight->speed * delta;

    float width = WeightWidth(weight->type);

    // Collision at HIT_L0 -- check road 1 (top lane) only, fire once
    if (!weight->hit_l0 && weight->y < HIT_L0) {
      weight->hit_l0 = true;
      CheckWeightCollisionTopLane(game, weight, width);
    }

    // Collision at HIT_L1 -- check ALL creatures on both lanes, fire once
    if (!weight->hit_l1 && weight->y < HIT_L1) {
      weight->hit_l1 = true;
      CheckWeightCollisionAllLanes(game, weight, width);
    }

    // Hit ground
    if (weight->y < GROUND_LVL) {
      WeightHitGround(game, weight);
    }
  }
}

static void UpdateRoomCooldowns(Game* game, float delta) {
  for (int row = 0; row < BUILDING_ROWS; row++) {
    for (int col = 0; col < BUILDING_COLS; col++) {
      Room* room = &game->rooms[row][col];
      if (!room->cooldown_ready) {
        room->cooldown_timer -= delta;
        if (room->cooldown_timer <= 0) {
          room->cooldown_ready = true;
          room->cooldown_timer = 0;
        }
      }
    }
  }
}

static void UpdateLooseControl(Game* game) {
  bool any_missed = false;
  for (int idx = 0; idx < MAX_CREATURES; idx++) {
    Creature* creature = &game->creatures[idx];
    if (!creature->active) {
      continue;
    }
    if (creature->x > CLUB_X) {
      game->hits++;
      creature->active = false;
      any_missed = true;
    }
  }
  if (game->hits > 5) {
    game->hits = 5;
  }
  if (any_missed) {
    if (IsMusicStreamPlaying(game->assets.bgm_cool)) {
      PauseMusicStream(game->assets.bgm_cool);
    }
    SetMusicVolume(game->assets.bgm_cool, 0.2f * (float) game->hits);
    PlayMusicStream(game->assets.bgm_cool);
  }
}

static void UpdateSelfieFlash(Game* game, float delta) {
  if (game->selfie_pending) {
    game->selfie_time += delta;
    if (game->selfie_time >= SELFIE_DELAY) {
      game->selfie_pending = false;
      game->selfie_active = true;
      game->selfie_time = 0;
      game->selfie_alpha = 1.0f;
    }
  }
  if (game->selfie_active) {
    game->selfie_time += delta;
    float attack_time = 2.0f;
    game->selfie_alpha = cosf(game->selfie_time * PI / 2.0f / attack_time);
    if (game->selfie_time >= attack_time) {
      game->selfie_active = false;
      game->selfie_time = 0;
      game->selfie_alpha = 0;
    }
  }
}

static void UpdateAnimations(Game* game, float delta) {
  for (int idx = 0; idx < MAX_ANIMATIONS; idx++) {
    AnimEffect* effect = &game->anims[idx];
    if (!effect->active) {
      continue;
    }
    effect->state_time += delta;
    const SpriteAnim* anim = AnimForType(&game->assets, effect->type);
    if (effect->state_time >= sprite_anim_duration(anim)) {
      effect->active = false;
    }
  }
}

// ---------------------------------------------------------------------------
// Render subsystems (extracted to reduce cognitive complexity)
// ---------------------------------------------------------------------------

static void RenderClubBuilding(Game* game) {
  GameAssets* assets = &game->assets;
  Rectangle src = sprite_anim_frame(&assets->anim_club_night, game->club_anim_time);
  Rectangle dst = {(float) CLUB_X, (float) FLIP_Y(CLUB_Y_GDX, 484), src.width, src.height};
  DrawTexturePro(assets->club_night_sheet, src, dst, (Vector2) {0, 0}, 0, WHITE);
}

static void RenderBabkaInWindow(Game* game) {
  GameAssets* assets = &game->assets;
  int row = game->cur_row;
  int col = game->cur_col;
  if (!game->rooms[row][col].cooldown_ready) {
    int grid_x = ROOM_GDX_X(col);
    int grid_y = ROOM_GDX_Y(row);
    DrawTexture(assets->babka_in_window, grid_x, FLIP_Y(grid_y, assets->babka_in_window.height), WHITE);
  }
}

static void RenderInWindowWeapons(Game* game) {
  GameAssets* assets = &game->assets;
  for (int row = 0; row < BUILDING_ROWS; row++) {
    for (int col = 0; col < BUILDING_COLS; col++) {
      Room* room = &game->rooms[row][col];
      if (!room->broken && room->armed && room->cooldown_ready) {
        int grid_x = ROOM_GDX_X(col);
        int grid_y = ROOM_GDX_Y(row);
        Texture2D tex;
        int offset_x;
        int offset_y;
        switch (room->type) {
          case ROOM_POT:
            tex = assets->flower_in_window;
            offset_x = 55;
            offset_y = 30;
            break;
          case ROOM_TV:
            tex = assets->tv_in_window;
            offset_x = 50;
            offset_y = 30;
            break;
          case ROOM_ROYAL:
            tex = assets->piano_in_window;
            offset_x = 60;
            offset_y = 30;
            break;
        }
        DrawTexture(tex, grid_x + offset_x, FLIP_Y(grid_y + offset_y, tex.height), WHITE);
      }
    }
  }
}

static void RenderLights(Game* game) {
  GameAssets* assets = &game->assets;
  for (int row = 0; row < BUILDING_ROWS; row++) {
    for (int col = 0; col < BUILDING_COLS; col++) {
      Room* room = &game->rooms[row][col];
      int grid_x = ROOM_GDX_X(col);
      int grid_y = ROOM_GDX_Y(row);
      if (!room->broken && room->bought) {
        DrawTexture(assets->light_on, grid_x, FLIP_Y(grid_y, assets->light_on.height), WHITE);
      } else if (!room->broken) {
        // Room is not broken and not bought -- show light off
        DrawTexture(assets->light_off, grid_x, FLIP_Y(grid_y, assets->light_off.height), WHITE);
      }
    }
  }
}

static void RenderWindowFrames(Game* game) {
  GameAssets* assets = &game->assets;
  for (int row = 0; row < BUILDING_ROWS; row++) {
    for (int col = 0; col < BUILDING_COLS; col++) {
      Room* room = &game->rooms[row][col];
      int grid_x = ROOM_GDX_X(col);
      int grid_y = ROOM_GDX_Y(row);
      Texture2D tex;
      if (room->broken) {
        tex = assets->wnd_night_broken;
      } else if (room->grate) {
        tex = assets->wnd_night_grate;
      } else if (room->bought || (row == game->cur_row && col == game->cur_col && room->cooldown_ready)) {
        tex = assets->wnd_night_normal;
      } else {
        tex = assets->wnd_night_disabled;
      }
      DrawTexture(tex, grid_x, FLIP_Y(grid_y, tex.height), WHITE);
    }
  }
}

static void RenderBabkaHandsUp(Game* game) {
  GameAssets* assets = &game->assets;
  int row = game->cur_row;
  int col = game->cur_col;
  Room* room = &game->rooms[row][col];
  if (!room->cooldown_ready) {
    return;
  }
  // Weapon above babka
  if (room->armed) {
    float weapon_x;
    float weapon_y;
    Texture2D weapon_tex;
    switch (room->type) {
      case ROOM_POT:
        weapon_tex = assets->pot_tex;
        weapon_x = (float) ((col * 128) + 128);
        weapon_y = (float) ((row * 128) + 256 + 120);
        break;
      case ROOM_TV:
        weapon_tex = assets->tv_tex;
        weapon_x = (float) ((col * 128) + 128 + 23);
        weapon_y = (float) ((row * 128) + 256 + 115);
        break;
      case ROOM_ROYAL:
        weapon_tex = assets->royal_tex;
        weapon_x = (float) ((col * 128) + 128 - 10);
        weapon_y = (float) ((row * 128) + 256 + 120);
        break;
    }
    DrawTexture(weapon_tex, (int) weapon_x, FLIP_Y((int) weapon_y, weapon_tex.height), WHITE);
  }
  // Babka hands up
  float babka_x = (float) ((col * 128) + 128 + 23);
  float babka_y = (float) ((row * 128) + 256 + 30);
  DrawTexture(assets->babka_hands_up, (int) babka_x, FLIP_Y((int) babka_y, assets->babka_hands_up.height), WHITE);
}

static void RenderCashSprites(Game* game) {
  GameAssets* assets = &game->assets;
  for (int idx = 0; idx < MAX_CASH_SPRITES; idx++) {
    if (!game->cash[idx].active) {
      continue;
    }
    DrawTexture(assets->cash, (int) game->cash[idx].x, FLIP_Y((int) game->cash[idx].y, assets->cash.height), WHITE);
  }
}

static void RenderCreatures(Game* game) {
  GameAssets* assets = &game->assets;
  for (int lane = 1; lane >= 0; lane--) {
    for (int idx = 0; idx < MAX_CREATURES; idx++) {
      Creature* creature = &game->creatures[idx];
      if (!creature->active || creature->road != lane) {
        continue;
      }

      float lane_y = (creature->road == 0) ? BOTTOM_LANE : TOP_LANE;

      const SpriteAnim* sanim;
      if (creature->type == CREATURE_HOOLIGAN) {
        sanim = creature->attacking ? &assets->anim_hooligan_attack : &assets->anim_hooligan_walk;
      } else {
        sanim = creature->attacking ? &assets->anim_whore_attack : &assets->anim_whore_walk;
      }

      Rectangle src = sprite_anim_frame(sanim, creature->state_time);
      Rectangle dst = {creature->x, (float) FLIP_Y((int) lane_y, CREATURE_H), (float) sanim->frame_width,
                       (float) sanim->frame_height};
      DrawTexturePro(sanim->sheet, src, dst, (Vector2) {0, 0}, 0, WHITE);
    }
  }
}

static void RenderDeathAnimations(Game* game) {
  GameAssets* assets = &game->assets;
  for (int idx = 0; idx < MAX_ANIMATIONS; idx++) {
    AnimEffect* effect = &game->anims[idx];
    if (!effect->active) {
      continue;
    }
    const SpriteAnim* anim = AnimForType(assets, effect->type);
    Rectangle src = sprite_anim_frame(anim, effect->state_time);
    Rectangle dst = {effect->x, (float) FLIP_Y((int) effect->y, anim->frame_height), (float) anim->frame_width,
                     (float) anim->frame_height};
    DrawTexturePro(anim->sheet, src, dst, (Vector2) {0, 0}, 0, WHITE);
  }
}

static void RenderFallingWeights(Game* game) {
  GameAssets* assets = &game->assets;
  for (int idx = 0; idx < MAX_WEIGHTS; idx++) {
    Weight* weight = &game->weights[idx];
    if (!weight->active) {
      continue;
    }
    Texture2D tex;
    switch (weight->type) {
      case WEIGHT_POT:
        tex = assets->pot_tex;
        break;
      case WEIGHT_TV:
        tex = assets->tv_tex;
        break;
      case WEIGHT_ROYAL:
        tex = assets->royal_tex;
        break;
    }
    DrawTexture(tex, (int) weight->x, FLIP_Y((int) weight->y, tex.height), WHITE);
  }
}

static void RenderBullets(Game* game) {
  GameAssets* assets = &game->assets;
  for (int idx = 0; idx < MAX_BULLETS; idx++) {
    Bullet* bullet = &game->bullets[idx];
    if (!bullet->active) {
      continue;
    }
    DrawTexture(assets->bottle, (int) bullet->x, FLIP_Y((int) bullet->y, assets->bottle.height), WHITE);
  }
}

static void RenderFrameSelector(Game* game) {
  GameAssets* assets = &game->assets;
  Rectangle src = sprite_anim_frame(&assets->anim_frame, game->frame_anim_time);
  int grid_x = ROOM_GDX_X(game->cur_col);
  int grid_y = ROOM_GDX_Y(game->cur_row);
  // Frame is 138x138, slightly larger than the 128x128 room.
  // Center it over the room: offset by (138-128)/2 = 5 in each direction.
  Rectangle dst = {(float) (grid_x - 5), (float) FLIP_Y(grid_y - 5, 138), 138.0f, 138.0f};
  DrawTexturePro(assets->frame_sheet, src, dst, (Vector2) {0, 0}, 0, WHITE);
}

static void RenderHud(Game* game) {
  GameAssets* assets = &game->assets;
  // HUD background
  int hud_gx = 961;
  int hud_gy = 699;
  DrawTexture(assets->hud_back, hud_gx, FLIP_Y(hud_gy, assets->hud_back.height), WHITE);

  // HUD main (clipped by hits)
  if (game->hits > 0) {
    int clip_w = 80 * game->hits;
    Rectangle hud_src = {0, 0, (float) clip_w, (float) assets->hud_main.height};
    Rectangle hud_dst = {(float) hud_gx, (float) FLIP_Y(hud_gy, assets->hud_main.height), (float) clip_w,
                         (float) assets->hud_main.height};
    DrawTexturePro(assets->hud_main, hud_src, hud_dst, (Vector2) {0, 0}, 0, WHITE);
  }

  // HUD front overlay
  DrawTexture(assets->hud_front, hud_gx, FLIP_Y(hud_gy, assets->hud_front.height), WHITE);

  // Text: money, time, level
  Color purple = (Color) {167, 128, 183, 255};
  float font_size = 44.0f;
  float spacing = 1.0f;

  const char* money_text = TextFormat("~: %d", game->money);
  DrawTextEx(assets->main_font, money_text, (Vector2) {1150.0f, (float) FLIP_Y(685, 20)}, font_size, spacing, purple);

  int remaining = (int) (LEVEL_TIME - game->level_time);
  if (remaining < 0) {
    remaining = 0;
  }
  const char* time_text = TextFormat("time:%d", remaining);
  DrawTextEx(assets->main_font, time_text, (Vector2) {1150.0f, (float) FLIP_Y(650, 20)}, font_size, spacing, purple);

  const char* level_text = TextFormat("lvl:%d", game->level);
  DrawTextEx(assets->main_font, level_text, (Vector2) {1150.0f, (float) FLIP_Y(615, 20)}, font_size, spacing, purple);
}

static void RenderSelfieFlash(Game* game) {
  if (!game->selfie_active) {
    return;
  }
  unsigned char alpha = (unsigned char) (game->selfie_alpha * 255.0f);
  if (game->selfie_alpha < 0) {
    alpha = 0;
  }
  DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color) {255, 255, 255, alpha});
}

// ---------------------------------------------------------------------------
// NightEnter
// ---------------------------------------------------------------------------
void NightEnter(Game* game) {
  game->hits = 0;
  game->level_time = 0;
  game->spawn_timer = 0;
  game->selfie_active = false;
  game->selfie_pending = false;
  game->selfie_alpha = 0;
  game->selfie_time = 0;
  game->club_anim_time = 0;
  game->frame_anim_time = 0;

  ClearEntities(game);
  ResetRoomCooldowns(game);

  PlayMusicStream(game->assets.bgm_crickets);
}

// ---------------------------------------------------------------------------
// NightExit
// ---------------------------------------------------------------------------
void NightExit(Game* game) {
  if (IsMusicStreamPlaying(game->assets.bgm_cool)) {
    StopMusicStream(game->assets.bgm_cool);
  }
  if (IsMusicStreamPlaying(game->assets.bgm_crickets)) {
    StopMusicStream(game->assets.bgm_crickets);
  }

  ClearEntities(game);
  ResetRoomCooldowns(game);
}

// ---------------------------------------------------------------------------
// NightUpdate
// ---------------------------------------------------------------------------
void NightUpdate(Game* game, float delta) {
  game->level_time += delta;
  game->club_anim_time += delta;
  game->frame_anim_time += delta;

  // -- Music streams --
  UpdateMusicStream(game->assets.bgm_crickets);
  if (IsMusicStreamPlaying(game->assets.bgm_cool)) {
    UpdateMusicStream(game->assets.bgm_cool);
  }

  UpdateInputHandling(game);
  UpdateFireWeapon(game);

  game->spawn_timer += delta;
  SpawnCreatures(game);

  UpdateCreatures(game, delta);
  UpdateBullets(game, delta);
  UpdateWeights(game, delta);
  UpdateRoomCooldowns(game, delta);
  UpdateLooseControl(game);
  UpdateSelfieFlash(game, delta);
  UpdateAnimations(game, delta);

  // State transitions
  if (game->level_time >= LEVEL_TIME) {
    game->state = STATE_DAY;
  }
  if (game->hits >= 5) {
    game->state = STATE_OVER;
  }
}

// ---------------------------------------------------------------------------
// NightRender
// ---------------------------------------------------------------------------
void NightRender(Game* game) {
  GameAssets* assets = &game->assets;

  // 1. Background
  DrawTexture(assets->bg_night, 0, 0, WHITE);

  // 2. Club building (animated)
  RenderClubBuilding(game);

  // 3. Babka in window (if current room cooldown NOT ready)
  RenderBabkaInWindow(game);

  // 4. In-window weapon displays
  RenderInWindowWeapons(game);

  // 5. Light/background behind windows
  RenderLights(game);

  // 6. Window frames
  RenderWindowFrames(game);

  // 7. Babka hands up + weapon above (if current room cooldown_ready)
  RenderBabkaHandsUp(game);

  // 8. Cash sprites
  RenderCashSprites(game);

  // 9. Creatures
  RenderCreatures(game);

  // 10. Death/crash animations
  RenderDeathAnimations(game);

  // 11. Falling weapons (weights)
  RenderFallingWeights(game);

  // 12. Bullets (bottles)
  RenderBullets(game);

  // 13. Frame selector at current room position
  RenderFrameSelector(game);

  // 14. HUD
  RenderHud(game);

  // 15. Selfie flash
  RenderSelfieFlash(game);
}
