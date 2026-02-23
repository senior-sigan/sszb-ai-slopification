#include "day.h"

#include <raylib.h>

#include "game_types.h"
#include "input.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Day-phase room selection: a room can be selected if it is bought OR if any
// adjacent room (up/down/left/right) is already bought.  This lets the player
// buy rooms adjacent to ones they already own.
static bool CanUseRoomDay(const Game* game, int row, int col) {
  if (game->rooms[row][col].bought) {
    return true;
  }
  // Check neighbours
  if (row > 0 && game->rooms[row - 1][col].bought) {
    return true;
  }
  if (row < BUILDING_ROWS - 1 && game->rooms[row + 1][col].bought) {
    return true;
  }
  if (col > 0 && game->rooms[row][col - 1].bought) {
    return true;
  }
  if (col < BUILDING_COLS - 1 && game->rooms[row][col + 1].bought) {
    return true;
  }
  return false;
}

// Handle arrow-key navigation between rooms during the day phase.
static void HandleNavigation(Game* game) {
  int row = game->cur_row;
  int col = game->cur_col;

  if (ManagedIsKeyPressed(KEY_DOWN) && row > 0 && CanUseRoomDay(game, row - 1, col)) {
    game->cur_row = row - 1;
  }
  if (ManagedIsKeyPressed(KEY_UP) && row < BUILDING_ROWS - 1 && CanUseRoomDay(game, row + 1, col)) {
    game->cur_row = row + 1;
  }
  if (ManagedIsKeyPressed(KEY_LEFT) && col > 0 && CanUseRoomDay(game, row, col - 1)) {
    game->cur_col = col - 1;
  }
  if (ManagedIsKeyPressed(KEY_RIGHT) && col < BUILDING_COLS - 1 && CanUseRoomDay(game, row, col + 1)) {
    game->cur_col = col + 1;
  }
}

// Try to buy or repair the currently selected room.
static void HandleBuyOrRepair(Game* game, Room* room) {
  if (room->bought) {
    // Repair if broken
    if (room->broken) {
      int cost = RoomRepairPrice(room);
      if (game->money >= cost) {
        room->broken = false;
        PlaySound(game->assets.snd_bye);
        game->money -= cost;
      }
    }
  } else {
    // Buy
    int cost = RoomBuyPrice(room);
    if (game->money >= cost) {
      room->bought = true;
      PlaySound(game->assets.snd_bye);
      game->money -= cost;
    }
  }
}

// Handle purchasing and upgrading rooms during the day phase.
static void HandleShopInput(Game* game) {
  Room* room = &game->rooms[game->cur_row][game->cur_col];

  // NUM_1 -- Buy or Repair
  if (ManagedIsKeyPressed(KEY_ONE)) {
    HandleBuyOrRepair(game, room);
  }

  // NUM_2 -- Install grate
  if (ManagedIsKeyPressed(KEY_TWO)) {
    if (room->bought && !room->broken && !room->grate) {
      int cost = RoomGratePrice(room);
      if (game->money >= cost) {
        room->grate = true;
        PlaySound(game->assets.snd_bye);
        game->money -= cost;
      }
    }
  }

  // NUM_3 -- Install weapon
  if (ManagedIsKeyPressed(KEY_THREE)) {
    if (room->bought && !room->broken && !room->armed) {
      int cost = RoomWeaponPrice(room);
      if (game->money >= cost) {
        room->armed = true;
        PlaySound(game->assets.snd_bye);
        game->money -= cost;
      }
    }
  }

  // NUM_4 -- Buy club (win condition)
  if (ManagedIsKeyPressed(KEY_FOUR)) {
    if (game->money >= CLUB_PRICE) {
      game->club_bought = true;
      game->money -= CLUB_PRICE;
    }
  }
}

// ---------------------------------------------------------------------------
// Render helpers
// ---------------------------------------------------------------------------

// Draw the in-window weapon, light, and window frame for a single room.
static void RenderRoom(GameAssets* assets, const Room* room, int grid_x, int grid_y) {
  // In-window weapon display
  if (!room->broken && room->armed) {
    Texture2D tex;
    int offset_x = 0;
    switch (room->type) {
      case ROOM_POT:
        tex = assets->flower_in_window;
        offset_x = 55;
        break;
      case ROOM_TV:
        tex = assets->tv_in_window;
        offset_x = 50;
        break;
      case ROOM_ROYAL:
        tex = assets->piano_in_window;
        offset_x = 60;
        break;
    }
    DrawTexture(tex, grid_x + offset_x, FLIP_Y(grid_y + 30, tex.height), WHITE);
  }

  // Light behind window
  if (!room->broken && room->bought) {
    DrawTexture(assets->light_on, grid_x, FLIP_Y(grid_y, assets->light_on.height), WHITE);
  } else if (!room->broken && !room->bought) {
    DrawTexture(assets->light_day, grid_x, FLIP_Y(grid_y, assets->light_day.height), WHITE);
  }

  // Window frame
  Texture2D wnd_tex;
  if (room->broken) {
    wnd_tex = assets->wnd_day_broken;
  } else if (room->grate) {
    wnd_tex = assets->wnd_day_grate;
  } else if (room->bought) {
    wnd_tex = assets->wnd_day_normal;
  } else {
    wnd_tex = assets->wnd_day_disabled;
  }
  DrawTexture(wnd_tex, grid_x, FLIP_Y(grid_y, 128), WHITE);
}

// Draw a price label at a given position using the selected color.
static void DrawPriceLabel(const Font* font, int cost, Color color, float pos_x) {
  const char* txt = TextFormat("~%d", cost);
  DrawTextEx(*font, txt, (Vector2) {pos_x, (float) FLIP_Y(70, 30)}, 30.0f, 1.0f, color);
}

// Render shop UI buttons and their price labels at the bottom of the screen.
static void RenderShopUI(Game* game) {
  GameAssets* assets = &game->assets;
  Color green = (Color) {54, 131, 87, 255};
  Color red = (Color) {255, 0, 0, 255};

  Room* sel = &game->rooms[game->cur_row][game->cur_col];

  // Buy/Repair button (position 0,0 in libGDX)
  if (!sel->bought) {
    DrawTexture(assets->shop_buy, 0, FLIP_Y(0, assets->shop_buy.height), WHITE);
    int cost = RoomBuyPrice(sel);
    Color price_color = (game->money >= cost) ? green : red;
    DrawPriceLabel(&assets->main_font, cost, price_color, 200.0f);
  } else {
    DrawTexture(assets->shop_repair, 0, FLIP_Y(0, assets->shop_repair.height), WHITE);
    if (sel->broken) {
      int cost = RoomRepairPrice(sel);
      Color price_color = (game->money >= cost) ? green : red;
      DrawPriceLabel(&assets->main_font, cost, price_color, 200.0f);
    }
  }

  // Grate button (position 343,0 in libGDX)
  DrawTexture(assets->shop_grate, 343, FLIP_Y(0, assets->shop_grate.height), WHITE);
  if (sel->bought && !sel->broken && !sel->grate) {
    int cost = RoomGratePrice(sel);
    Color price_color = (game->money >= cost) ? green : red;
    DrawPriceLabel(&assets->main_font, cost, price_color, 543.0f);
  }

  // Weapon button (position 686,0 in libGDX)
  {
    Texture2D weapon_btn;
    switch (sel->type) {
      case ROOM_POT:
        weapon_btn = assets->shop_weapon_pot;
        break;
      case ROOM_TV:
        weapon_btn = assets->shop_weapon_tv;
        break;
      case ROOM_ROYAL:
        weapon_btn = assets->shop_weapon_royal;
        break;
    }
    DrawTexture(weapon_btn, 686, FLIP_Y(0, weapon_btn.height), WHITE);
  }
  if (sel->bought && !sel->broken && !sel->armed) {
    int cost = RoomWeaponPrice(sel);
    Color price_color = (game->money >= cost) ? green : red;
    DrawPriceLabel(&assets->main_font, cost, price_color, 886.0f);
  }

  // Club button (position 1029,0 in libGDX)
  DrawTexture(assets->shop_club, 1029, FLIP_Y(0, assets->shop_club.height), WHITE);
  {
    Color price_color = (game->money >= CLUB_PRICE) ? green : red;
    DrawPriceLabel(&assets->main_font, CLUB_PRICE, price_color, 1229.0f);
  }
}

// ---------------------------------------------------------------------------
// DayEnter
// ---------------------------------------------------------------------------
void DayEnter(Game* game) {
  PlaySound(game->assets.snd_round_end);
  PlayMusicStream(game->assets.bgm_birds);
  game->frame_anim_time = 0;
}

// ---------------------------------------------------------------------------
// DayExit
// ---------------------------------------------------------------------------
void DayExit(Game* game) {
  if (IsMusicStreamPlaying(game->assets.bgm_birds)) {
    StopMusicStream(game->assets.bgm_birds);
  }
  game->level++;
}

// ---------------------------------------------------------------------------
// DayUpdate
// ---------------------------------------------------------------------------
void DayUpdate(Game* game, float delta) {
  game->frame_anim_time += delta;

  // Keep music stream fed
  UpdateMusicStream(game->assets.bgm_birds);

  // Arrow-key navigation
  HandleNavigation(game);

  // Shop purchases
  HandleShopInput(game);

  // State transitions
  if (ManagedIsKeyPressed(KEY_ENTER) || game->club_bought) {
    if (game->club_bought) {
      game->state = STATE_WIN;
    } else {
      game->state = STATE_NIGHT;
    }
  }
}

// ---------------------------------------------------------------------------
// DayRender
// ---------------------------------------------------------------------------
void DayRender(Game* game) {
  GameAssets* assets = &game->assets;

  // 1. Background
  DrawTexture(assets->bg_day, 0, 0, WHITE);

  // 2. Club building (day)
  DrawTexture(assets->club_day, CLUB_X, FLIP_Y(CLUB_Y_GDX, assets->club_day.height), WHITE);

  // 3. For each room: in-window weapons, lights, window frames
  for (int row = 0; row < BUILDING_ROWS; row++) {
    for (int col = 0; col < BUILDING_COLS; col++) {
      Room* room = &game->rooms[row][col];
      int grid_x = ROOM_GDX_X(col);
      int grid_y = ROOM_GDX_Y(row);
      RenderRoom(assets, room, grid_x, grid_y);
    }
  }

  // 4. Money display
  {
    Color purple = (Color) {167, 128, 183, 255};
    const char* money_text = TextFormat("~: %d", game->money);
    DrawTextEx(assets->main_font, money_text, (Vector2) {1150.0f, (float) FLIP_Y(685, 20)}, 44.0f, 1.0f, purple);
  }

  // 5. Selection frame (animated, 2 frames, 138x138)
  {
    Rectangle src = SpriteAnimFrame(&assets->anim_frame, game->frame_anim_time);
    int grid_x = ROOM_GDX_X(game->cur_col);
    int grid_y = ROOM_GDX_Y(game->cur_row);
    Rectangle dst = {(float) (grid_x - 5), (float) FLIP_Y(grid_y - 5, 138), 138.0f, 138.0f};
    DrawTexturePro(assets->frame_sheet, src, dst, (Vector2) {0, 0}, 0, WHITE);
  }

  // 6. Shop UI buttons
  RenderShopUI(game);
}
