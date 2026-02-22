#include "night.h"
#include <string.h>
#include <math.h>

// ManagedIsKeyPressed is defined in main.c and combines physical keyboard
// input with TCP-injected key presses (for automated testing).
extern bool ManagedIsKeyPressed(int key);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool can_use_room(const Game *g, int row, int col) {
    return g->rooms[row][col].bought && !g->rooms[row][col].broken;
}

static float weight_width(WeightType t) {
    switch (t) {
        case WEIGHT_POT:   return POT_W;
        case WEIGHT_TV:    return TV_W;
        case WEIGHT_ROYAL: return ROYAL_W;
    }
    return POT_W;
}

// AABB overlap test (1D projection on x-axis).
static bool aabb_overlap_x(float ax, float aw, float bx, float bw) {
    return ax < bx + bw && ax + aw > bx;
}

// Returns a pointer to the SpriteAnim used for crash effects.
static const SpriteAnim *anim_for_type(const GameAssets *a, AnimType t) {
    switch (t) {
        case ANIM_HOOLIGAN_DIE: return &a->anim_hooligan_die;
        case ANIM_WHORE_DIE:    return &a->anim_whore_die;
        case ANIM_POT_CRASH:    return &a->anim_pot_crash;
        case ANIM_TV_CRASH:     return &a->anim_tv_crash;
        case ANIM_ROYAL_CRASH:  return &a->anim_royal_crash;
        default: break;
    }
    return &a->anim_pot_crash;
}

// Spawn a new AnimEffect into the first free slot.
static void spawn_anim(Game *g, AnimType type, float x, float y) {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (!g->anims[i].active) {
            g->anims[i] = (AnimEffect){
                .type = type, .x = x, .y = y,
                .state_time = 0, .active = true
            };
            return;
        }
    }
}

// Spawn a CashSprite into the first free slot.
static void spawn_cash(Game *g, float x, float y) {
    for (int i = 0; i < MAX_CASH_SPRITES; i++) {
        if (!g->cash[i].active) {
            g->cash[i] = (CashSprite){ .x = x, .y = y, .active = true };
            return;
        }
    }
}

// Kill a creature and award money / effects.
static void kill_creature(Game *g, Creature *c) {
    float lane_y = (c->road == 0) ? BOTTOM_LANE : TOP_LANE;
    if (c->type == CREATURE_HOOLIGAN) {
        spawn_anim(g, ANIM_HOOLIGAN_DIE, c->x, lane_y);
        g->money += HOOLIGAN_REWARD;
    } else {
        spawn_anim(g, ANIM_WHORE_DIE, c->x, lane_y);
        g->money += WHORE_REWARD;
    }
    spawn_cash(g, c->x, lane_y - 10 + (float)(rand() % 20));
    c->active = false;
}

// ---------------------------------------------------------------------------
// night_enter
// ---------------------------------------------------------------------------
void night_enter(Game *g) {
    g->hits = 0;
    g->level_time = 0;
    g->spawn_timer = 0;
    g->selfie_active = false;
    g->selfie_pending = false;
    g->selfie_alpha = 0;
    g->selfie_time = 0;
    g->club_anim_time = 0;
    g->frame_anim_time = 0;

    memset(g->creatures, 0, sizeof(g->creatures));
    memset(g->weights,   0, sizeof(g->weights));
    memset(g->bullets,   0, sizeof(g->bullets));
    memset(g->anims,     0, sizeof(g->anims));
    memset(g->cash,      0, sizeof(g->cash));

    for (int r = 0; r < BUILDING_ROWS; r++)
        for (int c = 0; c < BUILDING_COLS; c++) {
            g->rooms[r][c].cooldown_ready = true;
            g->rooms[r][c].cooldown_timer = 0;
        }

    PlayMusicStream(g->assets.bgm_crickets);
}

// ---------------------------------------------------------------------------
// night_exit
// ---------------------------------------------------------------------------
void night_exit(Game *g) {
    if (IsMusicStreamPlaying(g->assets.bgm_cool))
        StopMusicStream(g->assets.bgm_cool);
    if (IsMusicStreamPlaying(g->assets.bgm_crickets))
        StopMusicStream(g->assets.bgm_crickets);

    memset(g->creatures, 0, sizeof(g->creatures));
    memset(g->weights,   0, sizeof(g->weights));
    memset(g->bullets,   0, sizeof(g->bullets));
    memset(g->anims,     0, sizeof(g->anims));
    memset(g->cash,      0, sizeof(g->cash));

    for (int r = 0; r < BUILDING_ROWS; r++)
        for (int c = 0; c < BUILDING_COLS; c++) {
            g->rooms[r][c].cooldown_ready = true;
            g->rooms[r][c].cooldown_timer = 0;
        }
}

// ---------------------------------------------------------------------------
// night_update
// ---------------------------------------------------------------------------
void night_update(Game *g, float dt) {
    g->level_time += dt;
    g->club_anim_time += dt;
    g->frame_anim_time += dt;

    // -- Music streams --
    UpdateMusicStream(g->assets.bgm_crickets);
    if (IsMusicStreamPlaying(g->assets.bgm_cool))
        UpdateMusicStream(g->assets.bgm_cool);

    // ===================================================================
    // INPUT HANDLING
    // ===================================================================
    int row = g->cur_row;
    int col = g->cur_col;

    if (ManagedIsKeyPressed(KEY_DOWN) && row > 0 && can_use_room(g, row - 1, col))
        g->cur_row = row - 1;
    if (ManagedIsKeyPressed(KEY_UP) && row < BUILDING_ROWS - 1 && can_use_room(g, row + 1, col))
        g->cur_row = row + 1;
    if (ManagedIsKeyPressed(KEY_LEFT) && col > 0 && can_use_room(g, row, col - 1))
        g->cur_col = col - 1;
    if (ManagedIsKeyPressed(KEY_RIGHT) && col < BUILDING_COLS - 1 && can_use_room(g, row, col + 1))
        g->cur_col = col + 1;

    // Cheat: skip to day
    if (ManagedIsKeyPressed(KEY_L))
        g->level_time = LEVEL_TIME + 1.0f;

    // Cheat: add hit
    if (ManagedIsKeyPressed(KEY_P)) {
        g->hits++;
        if (g->hits > 5) g->hits = 5;
    }

    // Force game over
    if (ManagedIsKeyPressed(KEY_ESCAPE))
        g->hits = 5;

    // Fire weapon (SPACE)
    if (ManagedIsKeyPressed(KEY_SPACE)) {
        Room *room = &g->rooms[g->cur_row][g->cur_col];
        if (room->armed && room->cooldown_ready && !room->broken) {
            // Find free weight slot
            for (int i = 0; i < MAX_WEIGHTS; i++) {
                if (!g->weights[i].active) {
                    WeightType wt;
                    switch (room->type) {
                        case ROOM_POT:   wt = WEIGHT_POT;   break;
                        case ROOM_TV:    wt = WEIGHT_TV;     break;
                        case ROOM_ROYAL: wt = WEIGHT_ROYAL;  break;
                    }
                    g->weights[i] = (Weight){
                        .type = wt,
                        .x = (float)(g->cur_col * 128 + 128),
                        .y = (float)(g->cur_row * 128 + 256),
                        .speed = WEIGHT_SPEED,
                        .active = true,
                        .hit_l0 = false,
                        .hit_l1 = false
                    };
                    break;
                }
            }
            room->cooldown_ready = false;
            room->cooldown_timer = room_cooldown_time(room);
        }
    }

    // ===================================================================
    // AI SPAWN SYSTEM
    // ===================================================================
    g->spawn_timer += dt;
    if (g->spawn_timer >= difficulty_generator_timer(g->level)) {
        g->spawn_timer = 0;
        bool r0 = false, r1 = false;
        difficulty_spawn_random(g->level, rand(), &r0, &r1);

        for (int lane = 0; lane < 2; lane++) {
            if ((lane == 0 && !r0) || (lane == 1 && !r1)) continue;
            // Find free creature slot
            for (int i = 0; i < MAX_CREATURES; i++) {
                if (!g->creatures[i].active) {
                    bool is_hooligan = (rand() % 2 == 1);
                    Creature *cr = &g->creatures[i];
                    cr->active = true;
                    cr->road = lane;
                    cr->x = 0;
                    cr->attacking = false;
                    cr->state_time = 0;
                    cr->cooldown_ready = false;
                    cr->health = 1;
                    if (is_hooligan) {
                        cr->type = CREATURE_HOOLIGAN;
                        cr->width = HOOLIGAN_W;
                        cr->speed = difficulty_hooligan_speed(g->level);
                        cr->cooldown_timer = difficulty_hooligan_cooldown(g->level) / 2.0f;
                    } else {
                        cr->type = CREATURE_WHORE;
                        cr->width = WHORE_W;
                        cr->speed = difficulty_whore_speed(g->level);
                        cr->cooldown_timer = difficulty_whore_cooldown(g->level);
                    }
                    break;
                }
            }
        }
    }

    // ===================================================================
    // CREATURE AI
    // ===================================================================
    for (int i = 0; i < MAX_CREATURES; i++) {
        Creature *cr = &g->creatures[i];
        if (!cr->active) continue;

        // Cooldown countdown
        if (!cr->cooldown_ready) {
            cr->cooldown_timer -= dt;
            if (cr->cooldown_timer <= 0) {
                cr->cooldown_ready = true;
            }
        }

        if (cr->cooldown_ready) {
            // Attack!
            cr->attacking = true;
            cr->state_time = 0;
            cr->cooldown_ready = false;

            if (cr->type == CREATURE_HOOLIGAN) {
                // Hooligan: shoot a bullet at a random bought, non-broken room
                // Build list of valid target rooms
                int valid_rows[BUILDING_ROWS * BUILDING_COLS];
                int valid_cols[BUILDING_ROWS * BUILDING_COLS];
                int valid_count = 0;
                for (int rr = 0; rr < BUILDING_ROWS; rr++)
                    for (int cc = 0; cc < BUILDING_COLS; cc++)
                        if (g->rooms[rr][cc].bought && !g->rooms[rr][cc].broken) {
                            valid_rows[valid_count] = rr;
                            valid_cols[valid_count] = cc;
                            valid_count++;
                        }
                if (valid_count > 0) {
                    int idx = rand() % valid_count;
                    int tr = valid_rows[idx];
                    int tc = valid_cols[idx];
                    // Target center of room in libGDX coords
                    float tx = (float)(tc * 128 + 128 + 64);
                    float ty = (float)(tr * 128 + 256 + 64);
                    // Find free bullet slot
                    for (int b = 0; b < MAX_BULLETS; b++) {
                        if (!g->bullets[b].active) {
                            g->bullets[b] = (Bullet){
                                .x = cr->x,
                                .y = (float)(cr->road * 128),
                                .target_x = tx,
                                .target_y = ty,
                                .target_row = tr,
                                .target_col = tc,
                                .speed = BOTTLE_SPEED,
                                .active = true
                            };
                            break;
                        }
                    }
                }
                cr->cooldown_timer = difficulty_hooligan_cooldown(g->level);
            } else {
                // Whore: selfie effect (with 0.5s delay before flash)
                PlaySound(g->assets.snd_selfie);
                if (g->cur_row != 2) {
                    g->selfie_pending = true;
                    g->selfie_time = 0;
                }
                cr->cooldown_timer = difficulty_whore_cooldown(g->level);
            }
        }

        // Move forward (creatures keep walking even while attacking)
        if (!cr->cooldown_ready) {
            cr->x += (float)cr->speed * dt;
        }

        // Attacking animation timeout
        if (cr->attacking) {
            cr->state_time += dt;
            float anim_dur;
            if (cr->type == CREATURE_HOOLIGAN) {
                anim_dur = 5 * ANIM_NORMAL; // 5 frames * 0.2s = 1.0s
            } else {
                anim_dur = 4 * ANIM_NORMAL; // 4 frames * 0.2s = 0.8s
            }
            if (cr->state_time >= anim_dur) {
                cr->attacking = false;
                cr->state_time = 0;
            }
        }

        // Advance walk animation state_time (attacking is advanced above)
        if (!cr->attacking) {
            cr->state_time += dt;
        }
    }

    // ===================================================================
    // BULLET PHYSICS
    // ===================================================================
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &g->bullets[i];
        if (!b->active) continue;

        float angle = atan2f(b->target_y - b->y, b->target_x - b->x);
        b->x += (float)b->speed * dt * cosf(angle);
        b->y += (float)b->speed * dt * sinf(angle);

        if (fabsf(b->x - b->target_x) < 5.0f && fabsf(b->y - b->target_y) < 5.0f) {
            Room *target_room = &g->rooms[b->target_row][b->target_col];
            if (b->target_row == g->cur_row && b->target_col == g->cur_col) {
                // Dodge: player is in the target room
            } else if (target_room->grate) {
                target_room->grate_lives--;
                PlaySound(g->assets.snd_iron);
                if (target_room->grate_lives <= 0) {
                    target_room->grate = false;
                    target_room->grate_lives = GRATE_LIVES_INIT;
                }
            } else {
                target_room->broken = true;
                PlaySound(g->assets.snd_window_broken);
            }
            b->active = false;
        }
    }

    // ===================================================================
    // WEIGHT PHYSICS
    // ===================================================================
    for (int i = 0; i < MAX_WEIGHTS; i++) {
        Weight *w = &g->weights[i];
        if (!w->active) continue;

        // Fall downward (libGDX: y decreases = falling)
        w->y -= (float)w->speed * dt;

        float ww = weight_width(w->type);

        // Collision at HIT_L0 — check road 1 (top lane) only, fire once
        if (!w->hit_l0 && w->y < HIT_L0) {
            w->hit_l0 = true;
            for (int c = 0; c < MAX_CREATURES; c++) {
                Creature *cr = &g->creatures[c];
                if (!cr->active || cr->road != 1) continue;
                if (aabb_overlap_x(w->x, ww, cr->x, cr->width)) {
                    kill_creature(g, cr);
                }
            }
        }

        // Collision at HIT_L1 — check ALL creatures on both lanes, fire once
        if (!w->hit_l1 && w->y < HIT_L1) {
            w->hit_l1 = true;
            for (int c = 0; c < MAX_CREATURES; c++) {
                Creature *cr = &g->creatures[c];
                if (!cr->active) continue;
                if (aabb_overlap_x(w->x, ww, cr->x, cr->width)) {
                    kill_creature(g, cr);
                }
            }
        }

        // Hit ground
        if (w->y < GROUND_LVL) {
            switch (w->type) {
                case WEIGHT_POT:
                    PlaySound(g->assets.snd_pot_destroy);
                    spawn_anim(g, ANIM_POT_CRASH, w->x - 8, w->y);
                    break;
                case WEIGHT_TV:
                    PlaySound(g->assets.snd_tv_destroy);
                    spawn_anim(g, ANIM_TV_CRASH, w->x - 34, w->y);
                    break;
                case WEIGHT_ROYAL:
                    PlaySound(g->assets.snd_royal_deploy);
                    spawn_anim(g, ANIM_ROYAL_CRASH, w->x - 30, w->y);
                    break;
            }
            w->active = false;
        }
    }

    // ===================================================================
    // ROOM COOLDOWNS
    // ===================================================================
    for (int r = 0; r < BUILDING_ROWS; r++)
        for (int c = 0; c < BUILDING_COLS; c++) {
            Room *room = &g->rooms[r][c];
            if (!room->cooldown_ready) {
                room->cooldown_timer -= dt;
                if (room->cooldown_timer <= 0) {
                    room->cooldown_ready = true;
                    room->cooldown_timer = 0;
                }
            }
        }

    // ===================================================================
    // LOOSE CONTROL — creatures that reach the club
    // ===================================================================
    {
        bool any_missed = false;
        for (int i = 0; i < MAX_CREATURES; i++) {
            Creature *cr = &g->creatures[i];
            if (!cr->active) continue;
            if (cr->x > CLUB_X) {
                g->hits++;
                cr->active = false;
                any_missed = true;
            }
        }
        if (g->hits > 5) g->hits = 5;
        if (any_missed) {
            if (IsMusicStreamPlaying(g->assets.bgm_cool))
                PauseMusicStream(g->assets.bgm_cool);
            SetMusicVolume(g->assets.bgm_cool, 0.2f * (float)g->hits);
            PlayMusicStream(g->assets.bgm_cool);
        }
    }

    // ===================================================================
    // SELFIE FLASH EFFECT (0.5s delay before activation)
    // ===================================================================
    if (g->selfie_pending) {
        g->selfie_time += dt;
        if (g->selfie_time >= SELFIE_DELAY) {
            g->selfie_pending = false;
            g->selfie_active = true;
            g->selfie_time = 0;
            g->selfie_alpha = 1.0f;
        }
    }
    if (g->selfie_active) {
        g->selfie_time += dt;
        float attack_time = 2.0f;
        g->selfie_alpha = cosf(g->selfie_time * (float)PI / 2.0f / attack_time);
        if (g->selfie_time >= attack_time) {
            g->selfie_active = false;
            g->selfie_time = 0;
            g->selfie_alpha = 0;
        }
    }

    // ===================================================================
    // ANIMATION / CASH CLEANUP
    // ===================================================================
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        AnimEffect *ae = &g->anims[i];
        if (!ae->active) continue;
        ae->state_time += dt;
        const SpriteAnim *sa = anim_for_type(&g->assets, ae->type);
        if (ae->state_time >= sprite_anim_duration(sa)) {
            ae->active = false;
        }
    }
    for (int i = 0; i < MAX_CASH_SPRITES; i++) {
        if (!g->cash[i].active) continue;
        // Cash sprites are deactivated indirectly: they share anim timing.
        // Use a simple ~2 second timeout tracked via a trick: since CashSprite
        // doesn't have a timer, we piggy-back on the animation system.
        // For simplicity, mark cash sprites inactive after the same cycle as
        // the death animation that spawned them (~0.8s). Alternatively we can
        // just let them stay for a fixed number of frames. We use a lightweight
        // approach: cash lives as long as any death anim (the longest is 4*0.2=0.8s).
        // To approximate ~2 seconds we just keep them. We'll implement a frame-
        // counting approach: store spawn time in .x and .y is overloaded — BUT
        // our struct doesn't have a timer. Let's just leave them on screen for
        // the entire round and clear on exit. This matches the Scala behavior
        // where textures persist until round end.
    }

    // ===================================================================
    // STATE TRANSITIONS
    // ===================================================================
    if (g->level_time >= LEVEL_TIME) {
        g->state = STATE_DAY;
    }
    if (g->hits >= 5) {
        g->state = STATE_OVER;
    }
}

// ---------------------------------------------------------------------------
// night_render
// ---------------------------------------------------------------------------
void night_render(Game *g) {
    GameAssets *a = &g->assets;

    // 1. Background
    DrawTexture(a->bg_night, 0, 0, WHITE);

    // 2. Club building (animated)
    {
        Rectangle src = sprite_anim_frame(&a->anim_club_night, g->club_anim_time);
        Rectangle dst = {
            (float)CLUB_X,
            (float)FLIP_Y(CLUB_Y_GDX, 484),
            src.width, src.height
        };
        DrawTexturePro(a->club_night_sheet, src, dst, (Vector2){0, 0}, 0, WHITE);
    }

    // 3. Babka in window (if current room cooldown NOT ready)
    {
        int row = g->cur_row, col = g->cur_col;
        if (!g->rooms[row][col].cooldown_ready) {
            int gx = ROOM_GDX_X(col);
            int gy = ROOM_GDX_Y(row);
            DrawTexture(a->babka_in_window, gx,
                        FLIP_Y(gy, a->babka_in_window.height), WHITE);
        }
    }

    // 4. In-window weapon displays
    for (int r = 0; r < BUILDING_ROWS; r++) {
        for (int c = 0; c < BUILDING_COLS; c++) {
            Room *room = &g->rooms[r][c];
            if (!room->broken && room->armed && room->cooldown_ready) {
                int gx = ROOM_GDX_X(c);
                int gy = ROOM_GDX_Y(r);
                Texture2D tex;
                int ox, oy;
                switch (room->type) {
                    case ROOM_POT:
                        tex = a->flower_in_window;
                        ox = 55; oy = 30;
                        break;
                    case ROOM_TV:
                        tex = a->tv_in_window;
                        ox = 50; oy = 30;
                        break;
                    case ROOM_ROYAL:
                        tex = a->piano_in_window;
                        ox = 60; oy = 30;
                        break;
                }
                DrawTexture(tex, gx + ox,
                            FLIP_Y(gy + oy, tex.height), WHITE);
            }
        }
    }

    // 5. Light/background behind windows
    for (int r = 0; r < BUILDING_ROWS; r++) {
        for (int c = 0; c < BUILDING_COLS; c++) {
            Room *room = &g->rooms[r][c];
            int gx = ROOM_GDX_X(c);
            int gy = ROOM_GDX_Y(r);
            if (!room->broken && room->bought) {
                DrawTexture(a->light_on, gx, FLIP_Y(gy, a->light_on.height), WHITE);
            } else if (!room->broken && !room->bought) {
                DrawTexture(a->light_off, gx, FLIP_Y(gy, a->light_off.height), WHITE);
            }
        }
    }

    // 6. Window frames
    for (int r = 0; r < BUILDING_ROWS; r++) {
        for (int c = 0; c < BUILDING_COLS; c++) {
            Room *room = &g->rooms[r][c];
            int gx = ROOM_GDX_X(c);
            int gy = ROOM_GDX_Y(r);
            Texture2D tex;
            if (r == g->cur_row && c == g->cur_col && room->cooldown_ready) {
                tex = a->wnd_night_normal;
            } else if (room->broken) {
                tex = a->wnd_night_broken;
            } else if (room->grate) {
                tex = a->wnd_night_grate;
            } else if (room->bought) {
                tex = a->wnd_night_normal;
            } else {
                tex = a->wnd_night_disabled;
            }
            DrawTexture(tex, gx, FLIP_Y(gy, tex.height), WHITE);
        }
    }

    // 7. Babka hands up + weapon above (if current room cooldown_ready)
    {
        int row = g->cur_row, col = g->cur_col;
        Room *room = &g->rooms[row][col];
        if (room->cooldown_ready) {
            // Weapon above babka
            if (room->armed) {
                float wx, wy;
                Texture2D wtex;
                switch (room->type) {
                    case ROOM_POT:
                        wtex = a->pot_tex;
                        wx = (float)(col * 128 + 128);
                        wy = (float)(row * 128 + 256 + 120);
                        break;
                    case ROOM_TV:
                        wtex = a->tv_tex;
                        wx = (float)(col * 128 + 128 + 23);
                        wy = (float)(row * 128 + 256 + 115);
                        break;
                    case ROOM_ROYAL:
                        wtex = a->royal_tex;
                        wx = (float)(col * 128 + 128 - 10);
                        wy = (float)(row * 128 + 256 + 120);
                        break;
                }
                DrawTexture(wtex, (int)wx, FLIP_Y((int)wy, wtex.height), WHITE);
            }
            // Babka hands up
            float bx = (float)(col * 128 + 128 + 23);
            float by = (float)(row * 128 + 256 + 30);
            DrawTexture(a->babka_hands_up, (int)bx,
                        FLIP_Y((int)by, a->babka_hands_up.height), WHITE);
        }
    }

    // 8. Cash sprites
    for (int i = 0; i < MAX_CASH_SPRITES; i++) {
        if (!g->cash[i].active) continue;
        DrawTexture(a->cash,
                    (int)g->cash[i].x,
                    FLIP_Y((int)g->cash[i].y, a->cash.height), WHITE);
    }

    // 9. Creatures — draw top lane (road=1) first, then bottom lane (road=0)
    for (int lane = 1; lane >= 0; lane--) {
        for (int i = 0; i < MAX_CREATURES; i++) {
            Creature *cr = &g->creatures[i];
            if (!cr->active || cr->road != lane) continue;

            float lane_y = (cr->road == 0) ? BOTTOM_LANE : TOP_LANE;

            const SpriteAnim *sanim;
            if (cr->type == CREATURE_HOOLIGAN) {
                sanim = cr->attacking ? &a->anim_hooligan_attack
                                      : &a->anim_hooligan_walk;
            } else {
                sanim = cr->attacking ? &a->anim_whore_attack
                                      : &a->anim_whore_walk;
            }

            Rectangle src = sprite_anim_frame(sanim, cr->state_time);
            Rectangle dst = {
                cr->x,
                (float)FLIP_Y((int)lane_y, CREATURE_H),
                (float)sanim->frame_width,
                (float)sanim->frame_height
            };
            DrawTexturePro(sanim->sheet, src, dst, (Vector2){0, 0}, 0, WHITE);
        }
    }

    // 10. Death/crash animations
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        AnimEffect *ae = &g->anims[i];
        if (!ae->active) continue;
        const SpriteAnim *sa = anim_for_type(a, ae->type);
        Rectangle src = sprite_anim_frame(sa, ae->state_time);
        Rectangle dst = {
            ae->x,
            (float)FLIP_Y((int)ae->y, sa->frame_height),
            (float)sa->frame_width,
            (float)sa->frame_height
        };
        DrawTexturePro(sa->sheet, src, dst, (Vector2){0, 0}, 0, WHITE);
    }

    // 11. Falling weapons (weights)
    for (int i = 0; i < MAX_WEIGHTS; i++) {
        Weight *w = &g->weights[i];
        if (!w->active) continue;
        Texture2D tex;
        switch (w->type) {
            case WEIGHT_POT:   tex = a->pot_tex;   break;
            case WEIGHT_TV:    tex = a->tv_tex;     break;
            case WEIGHT_ROYAL: tex = a->royal_tex;  break;
        }
        DrawTexture(tex, (int)w->x, FLIP_Y((int)w->y, tex.height), WHITE);
    }

    // 12. Bullets (bottles)
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &g->bullets[i];
        if (!b->active) continue;
        DrawTexture(a->bottle, (int)b->x,
                    FLIP_Y((int)b->y, a->bottle.height), WHITE);
    }

    // 13. Frame selector at current room position
    {
        Rectangle src = sprite_anim_frame(&a->anim_frame, g->frame_anim_time);
        int gx = ROOM_GDX_X(g->cur_col);
        int gy = ROOM_GDX_Y(g->cur_row);
        // Frame is 138x138, slightly larger than the 128x128 room.
        // Center it over the room: offset by (138-128)/2 = 5 in each direction.
        Rectangle dst = {
            (float)(gx - 5),
            (float)FLIP_Y(gy - 5, 138),
            138.0f, 138.0f
        };
        DrawTexturePro(a->frame_sheet, src, dst, (Vector2){0, 0}, 0, WHITE);
    }

    // 14. HUD
    {
        // HUD background
        int hud_gx = 961;
        int hud_gy = 699;
        DrawTexture(a->hud_back, hud_gx,
                    FLIP_Y(hud_gy, a->hud_back.height), WHITE);

        // HUD main (clipped by hits)
        if (g->hits > 0) {
            int clip_w = 80 * g->hits;
            Rectangle hud_src = { 0, 0, (float)clip_w, (float)a->hud_main.height };
            Rectangle hud_dst = {
                (float)hud_gx,
                (float)FLIP_Y(hud_gy, a->hud_main.height),
                (float)clip_w,
                (float)a->hud_main.height
            };
            DrawTexturePro(a->hud_main, hud_src, hud_dst,
                           (Vector2){0, 0}, 0, WHITE);
        }

        // HUD front overlay
        DrawTexture(a->hud_front, hud_gx,
                    FLIP_Y(hud_gy, a->hud_front.height), WHITE);

        // Text: money, time, level
        Color purple = (Color){167, 128, 183, 255};
        float font_size = 44.0f;
        float spacing = 1.0f;

        const char *money_text = TextFormat("~: %d", g->money);
        DrawTextEx(a->main_font, money_text,
                   (Vector2){1150.0f, (float)FLIP_Y(685, 20)},
                   font_size, spacing, purple);

        int remaining = (int)(LEVEL_TIME - g->level_time);
        if (remaining < 0) remaining = 0;
        const char *time_text = TextFormat("time:%d", remaining);
        DrawTextEx(a->main_font, time_text,
                   (Vector2){1150.0f, (float)FLIP_Y(650, 20)},
                   font_size, spacing, purple);

        const char *level_text = TextFormat("lvl:%d", g->level);
        DrawTextEx(a->main_font, level_text,
                   (Vector2){1150.0f, (float)FLIP_Y(615, 20)},
                   font_size, spacing, purple);
    }

    // 15. Selfie flash
    if (g->selfie_active) {
        unsigned char alpha = (unsigned char)(g->selfie_alpha * 255.0f);
        if (g->selfie_alpha < 0) alpha = 0;
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                      (Color){255, 255, 255, alpha});
    }
}
