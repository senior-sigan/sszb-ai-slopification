#include "day.h"

// ManagedIsKeyPressed is defined in main.c and combines physical keyboard
// input with TCP-injected key presses (for automated testing).
extern bool ManagedIsKeyPressed(int key);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Day-phase room selection: a room can be selected if it is bought OR if any
// adjacent room (up/down/left/right) is already bought.  This lets the player
// buy rooms adjacent to ones they already own.
static bool can_use_room_day(const Game *g, int row, int col) {
    if (g->rooms[row][col].bought) return true;
    // Check neighbours
    if (row > 0 && g->rooms[row - 1][col].bought) return true;
    if (row < BUILDING_ROWS - 1 && g->rooms[row + 1][col].bought) return true;
    if (col > 0 && g->rooms[row][col - 1].bought) return true;
    if (col < BUILDING_COLS - 1 && g->rooms[row][col + 1].bought) return true;
    return false;
}

// ---------------------------------------------------------------------------
// day_enter
// ---------------------------------------------------------------------------
void day_enter(Game *g) {
    PlaySound(g->assets.snd_round_end);
    PlayMusicStream(g->assets.bgm_birds);
    g->frame_anim_time = 0;
}

// ---------------------------------------------------------------------------
// day_exit
// ---------------------------------------------------------------------------
void day_exit(Game *g) {
    if (IsMusicStreamPlaying(g->assets.bgm_birds))
        StopMusicStream(g->assets.bgm_birds);
    g->level++;
}

// ---------------------------------------------------------------------------
// day_update
// ---------------------------------------------------------------------------
void day_update(Game *g, float dt) {
    g->frame_anim_time += dt;

    // Keep music stream fed
    UpdateMusicStream(g->assets.bgm_birds);

    // ===================================================================
    // INPUT: Arrow keys to navigate rooms
    // ===================================================================
    int row = g->cur_row;
    int col = g->cur_col;

    if (ManagedIsKeyPressed(KEY_DOWN) && row > 0
        && can_use_room_day(g, row - 1, col))
        g->cur_row = row - 1;
    if (ManagedIsKeyPressed(KEY_UP) && row < BUILDING_ROWS - 1
        && can_use_room_day(g, row + 1, col))
        g->cur_row = row + 1;
    if (ManagedIsKeyPressed(KEY_LEFT) && col > 0
        && can_use_room_day(g, row, col - 1))
        g->cur_col = col - 1;
    if (ManagedIsKeyPressed(KEY_RIGHT) && col < BUILDING_COLS - 1
        && can_use_room_day(g, row, col + 1))
        g->cur_col = col + 1;

    Room *room = &g->rooms[g->cur_row][g->cur_col];

    // ===================================================================
    // INPUT: NUM_1 — Buy or Repair
    // ===================================================================
    if (ManagedIsKeyPressed(KEY_ONE)) {
        if (room->bought) {
            // Repair if broken
            if (room->broken) {
                int cost = room_repair_price(room);
                if (g->money >= cost) {
                    room->broken = false;
                    PlaySound(g->assets.snd_bye);
                    g->money -= cost;
                }
            }
        } else {
            // Buy
            int cost = room_buy_price(room);
            if (g->money >= cost) {
                room->bought = true;
                PlaySound(g->assets.snd_bye);
                g->money -= cost;
            }
        }
    }

    // ===================================================================
    // INPUT: NUM_2 — Install grate
    // ===================================================================
    if (ManagedIsKeyPressed(KEY_TWO)) {
        if (room->bought && !room->broken && !room->grate) {
            int cost = room_grate_price(room);
            if (g->money >= cost) {
                room->grate = true;
                PlaySound(g->assets.snd_bye);
                g->money -= cost;
            }
        }
    }

    // ===================================================================
    // INPUT: NUM_3 — Install weapon
    // ===================================================================
    if (ManagedIsKeyPressed(KEY_THREE)) {
        if (room->bought && !room->broken && !room->armed) {
            int cost = room_weapon_price(room);
            if (g->money >= cost) {
                room->armed = true;
                PlaySound(g->assets.snd_bye);
                g->money -= cost;
            }
        }
    }

    // ===================================================================
    // INPUT: NUM_4 — Buy club (win condition)
    // ===================================================================
    if (ManagedIsKeyPressed(KEY_FOUR)) {
        if (g->money >= CLUB_PRICE) {
            g->club_bought = true;
            g->money -= CLUB_PRICE;
        }
    }

    // ===================================================================
    // STATE TRANSITIONS
    // ===================================================================
    if (ManagedIsKeyPressed(KEY_ENTER) || g->club_bought) {
        if (g->club_bought) {
            g->state = STATE_WIN;
        } else {
            g->state = STATE_NIGHT;
        }
    }
}

// ---------------------------------------------------------------------------
// day_render
// ---------------------------------------------------------------------------
void day_render(Game *g) {
    GameAssets *a = &g->assets;

    // 1. Background
    DrawTexture(a->bg_day, 0, 0, WHITE);

    // 2. Club building (day)
    DrawTexture(a->club_day, CLUB_X,
                FLIP_Y(CLUB_Y_GDX, a->club_day.height), WHITE);

    // 3. For each room: in-window weapons, lights, window frames
    for (int i = 0; i < BUILDING_ROWS; i++) {
        for (int j = 0; j < BUILDING_COLS; j++) {
            Room *room = &g->rooms[i][j];
            int gx = ROOM_GDX_X(j);
            int gy = ROOM_GDX_Y(i);

            // 3a. In-window weapon display
            if (!room->broken && room->armed) {
                Texture2D tex;
                int ox = 0;
                switch (room->type) {
                    case ROOM_POT:
                        tex = a->flower_in_window;
                        ox = 55;
                        break;
                    case ROOM_TV:
                        tex = a->tv_in_window;
                        ox = 50;
                        break;
                    case ROOM_ROYAL:
                        tex = a->piano_in_window;
                        ox = 60;
                        break;
                }
                DrawTexture(tex, gx + ox,
                            FLIP_Y(gy + 30, tex.height), WHITE);
            }

            // 3b. Light behind window
            if (!room->broken && room->bought) {
                DrawTexture(a->light_on, gx,
                            FLIP_Y(gy, a->light_on.height), WHITE);
            } else if (!room->broken && !room->bought) {
                DrawTexture(a->light_day, gx,
                            FLIP_Y(gy, a->light_day.height), WHITE);
            }

            // 3c. Window frame
            Texture2D wnd_tex;
            if (room->broken) {
                wnd_tex = a->wnd_day_broken;
            } else if (room->grate) {
                wnd_tex = a->wnd_day_grate;
            } else if (room->bought) {
                wnd_tex = a->wnd_day_normal;
            } else {
                wnd_tex = a->wnd_day_disabled;
            }
            DrawTexture(wnd_tex, gx, FLIP_Y(gy, 128), WHITE);
        }
    }

    // 4. Money display
    {
        Color purple = (Color){167, 128, 183, 255};
        const char *money_text = TextFormat("~: %d", g->money);
        DrawTextEx(a->main_font, money_text,
                   (Vector2){1150.0f, (float)FLIP_Y(685, 20)},
                   44.0f, 1.0f, purple);
    }

    // 5. Selection frame (animated, 2 frames, 138x138)
    {
        Rectangle src = sprite_anim_frame(&a->anim_frame, g->frame_anim_time);
        int gx = ROOM_GDX_X(g->cur_col);
        int gy = ROOM_GDX_Y(g->cur_row);
        Rectangle dst = {
            (float)(gx - 5),
            (float)FLIP_Y(gy - 5, 138),
            138.0f, 138.0f
        };
        DrawTexturePro(a->frame_sheet, src, dst, (Vector2){0, 0}, 0, WHITE);
    }

    // 6. Shop UI buttons
    // Font for prices: loaded at size 44, drawn at size 30
    float price_size = 30.0f;
    float price_scale = price_size / 44.0f;
    (void)price_scale;  // scale factor for reference; DrawTextEx uses absolute size
    Color green = (Color){54, 131, 87, 255};
    Color red   = (Color){255, 0, 0, 255};

    Room *sel = &g->rooms[g->cur_row][g->cur_col];

    // 6a. Buy/Repair button (position 0,0 in libGDX)
    if (!sel->bought) {
        DrawTexture(a->shop_buy, 0,
                    FLIP_Y(0, a->shop_buy.height), WHITE);
        int cost = room_buy_price(sel);
        Color c = (g->money >= cost) ? green : red;
        const char *txt = TextFormat("~%d", cost);
        DrawTextEx(a->main_font, txt,
                   (Vector2){200.0f, (float)FLIP_Y(70, 30)},
                   price_size, 1.0f, c);
    } else {
        DrawTexture(a->shop_repair, 0,
                    FLIP_Y(0, a->shop_repair.height), WHITE);
        if (sel->broken) {
            int cost = room_repair_price(sel);
            Color c = (g->money >= cost) ? green : red;
            const char *txt = TextFormat("~%d", cost);
            DrawTextEx(a->main_font, txt,
                       (Vector2){200.0f, (float)FLIP_Y(70, 30)},
                       price_size, 1.0f, c);
        }
    }

    // 6b. Grate button (position 343,0 in libGDX)
    DrawTexture(a->shop_grate, 343,
                FLIP_Y(0, a->shop_grate.height), WHITE);
    if (sel->bought && !sel->broken && !sel->grate) {
        int cost = room_grate_price(sel);
        Color c = (g->money >= cost) ? green : red;
        const char *txt = TextFormat("~%d", cost);
        DrawTextEx(a->main_font, txt,
                   (Vector2){543.0f, (float)FLIP_Y(70, 30)},
                   price_size, 1.0f, c);
    }

    // 6c. Weapon button (position 686,0 in libGDX)
    {
        Texture2D weapon_btn;
        switch (sel->type) {
            case ROOM_POT:   weapon_btn = a->shop_weapon_pot;   break;
            case ROOM_TV:    weapon_btn = a->shop_weapon_tv;    break;
            case ROOM_ROYAL: weapon_btn = a->shop_weapon_royal; break;
        }
        DrawTexture(weapon_btn, 686,
                    FLIP_Y(0, weapon_btn.height), WHITE);
    }
    if (sel->bought && !sel->broken && !sel->armed) {
        int cost = room_weapon_price(sel);
        Color c = (g->money >= cost) ? green : red;
        const char *txt = TextFormat("~%d", cost);
        DrawTextEx(a->main_font, txt,
                   (Vector2){886.0f, (float)FLIP_Y(70, 30)},
                   price_size, 1.0f, c);
    }

    // 6d. Club button (position 1029,0 in libGDX)
    DrawTexture(a->shop_club, 1029,
                FLIP_Y(0, a->shop_club.height), WHITE);
    {
        Color c = (g->money >= CLUB_PRICE) ? green : red;
        const char *txt = TextFormat("~%d", CLUB_PRICE);
        DrawTextEx(a->main_font, txt,
                   (Vector2){1229.0f, (float)FLIP_Y(70, 30)},
                   price_size, 1.0f, c);
    }
}
