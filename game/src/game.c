#include "game_types.h"
#include <math.h>
#include <string.h>

Rectangle sprite_anim_frame(const SpriteAnim *a, float t) {
    int total = a->index_count;
    if (total == 0) return (Rectangle){0,0,0,0};
    float total_dur = (float)total * a->frame_duration;
    int fi;
    if (a->looping) {
        float mt = fmodf(t, total_dur);
        fi = (int)(mt / a->frame_duration);
    } else {
        fi = (int)(t / a->frame_duration);
    }
    if (fi >= total) fi = total - 1;
    if (fi < 0) fi = 0;
    int col = a->frame_indices[fi];
    return (Rectangle){ .x = (float)(col * a->frame_width), .y = 0,
                        .width = (float)a->frame_width, .height = (float)a->frame_height };
}

float sprite_anim_duration(const SpriteAnim *a) {
    return (float)a->index_count * a->frame_duration;
}

void sprite_anim_setup(SpriteAnim *a, Texture2D sheet, int fw, int fh,
                        const int *idx, int count, float dur, bool loop) {
    a->sheet = sheet;
    a->frame_width = fw;
    a->frame_height = fh;
    a->index_count = count > 16 ? 16 : count;
    for (int i = 0; i < a->index_count; i++) a->frame_indices[i] = idx[i];
    a->frame_duration = dur;
    a->looping = loop;
}

float room_cooldown_time(const Room *r) {
    switch (r->type) {
        case ROOM_POT: return POT_COOLDOWN;
        case ROOM_TV: return TV_COOLDOWN;
        case ROOM_ROYAL: return ROYAL_COOLDOWN;
    }
    return 1.0f;
}

int room_repair_price(const Room *r) { return r->base_price * REPAIR_MUL; }
int room_buy_price(const Room *r) { return r->base_price * BUY_MUL; }
int room_grate_price(const Room *r) { return r->base_price * GRATE_MUL; }

int room_weapon_price(const Room *r) {
    switch (r->type) {
        case ROOM_POT: return r->base_price * POT_WEAPON_MUL;
        case ROOM_TV: return r->base_price * TV_WEAPON_MUL;
        case ROOM_ROYAL: return r->base_price * ROYAL_WEAPON_MUL;
    }
    return r->base_price * POT_WEAPON_MUL;
}

int difficulty_hooligan_speed(int lvl) {
    return 65 + lvl * 10 + rand() % (lvl * 10 > 0 ? lvl * 10 : 1);
}

float difficulty_hooligan_cooldown(int lvl) {
    switch (lvl) {
        case 1: return 10.0f;
        case 2: return 8.0f;
        case 3: return 3.0f + (float)(rand() % 4);
        default: return 2.0f + (float)(rand() % 3);
    }
}

int difficulty_whore_speed(int lvl) {
    switch (lvl) {
        case 1: return 120;
        case 2: return 125;
        case 3: return 150;
        default: return 100 + lvl * 10 + rand() % (lvl * 10 > 0 ? lvl * 10 : 1);
    }
}

float difficulty_whore_cooldown(int lvl) {
    switch (lvl) {
        case 1: return 4.0f + (float)(rand() % 4);
        case 2: return 3.0f + (float)(rand() % 4);
        default: return 2.0f + (float)(rand() % 6);
    }
}

float difficulty_generator_timer(int lvl) {
    switch (lvl) {
        case 1: return 2.2f;
        case 2: return 2.0f;
        case 3: return 1.8f;
        case 4: return 1.8f;
        default: return 1.0f;
    }
}

void difficulty_spawn_random(int lvl, int seed, bool *r0, bool *r1) {
    int s = abs(seed) % 10;
    switch (lvl) {
        case 1: *r0 = (s < 5); *r1 = (s >= 5); break;
        case 2: *r0 = (s >= 5); *r1 = (s <= 4); break;
        case 3: *r0 = (s >= 4 && s <= 8); *r1 = (s <= 3); break;
        case 4: *r0 = (s >= 3 && s <= 5); *r1 = (s <= 2); break;
        case 5: *r0 = (s >= 2 && s <= 3); *r1 = (s <= 1); break;
        default: *r0 = true; *r1 = true; break;
    }
}

// House layout from RenderFactory.scala
// Row 0: (TV,4) (Pot,4) (Pot,3) (Pot,3) (TV,3) (Royal,4)
// Row 1: (Pot,4) (Pot,3) (TV,3) (TV,2) (Pot,2) (Royal,3)
// Row 2: (Royal,3) (Royal,3) (Pot,1) (Royal,2) (TV,2) (Pot,1)
void game_init_house(Room rooms[BUILDING_ROWS][BUILDING_COLS]) {
    typedef struct { RoomType t; int p; } RP;
    static const RP layout[3][6] = {
        {{ROOM_TV,4},{ROOM_POT,4},{ROOM_POT,3},{ROOM_POT,3},{ROOM_TV,3},{ROOM_ROYAL,4}},
        {{ROOM_POT,4},{ROOM_POT,3},{ROOM_TV,3},{ROOM_TV,2},{ROOM_POT,2},{ROOM_ROYAL,3}},
        {{ROOM_ROYAL,3},{ROOM_ROYAL,3},{ROOM_POT,1},{ROOM_ROYAL,2},{ROOM_TV,2},{ROOM_POT,1}},
    };
    for (int i = 0; i < BUILDING_ROWS; i++) {
        for (int j = 0; j < BUILDING_COLS; j++) {
            rooms[i][j] = (Room){
                .type = layout[i][j].t,
                .base_price = layout[i][j].p,
                .bought = false, .broken = false, .grate = false, .armed = false,
                .cooldown_ready = true, .cooldown_timer = 0,
                .grate_lives = GRATE_LIVES_INIT
            };
        }
    }
    // Starting rooms: (1,2) and (2,2) bought + armed
    rooms[1][2].bought = true;
    rooms[1][2].armed = true;
    rooms[2][2].bought = true;
    rooms[2][2].armed = true;
}

void game_reset(Game *g) {
    g->state = STATE_LOGO;
    g->state_timer = 0;
    g->level = 1;
    g->level_time = 0;
    g->hits = 0;
    g->money = START_MONEY;
    g->club_bought = false;
    g->cur_row = 1;
    g->cur_col = 2;
    g->spawn_timer = 0;
    g->selfie_active = false;
    g->selfie_alpha = 0;
    g->selfie_time = 0;
    g->club_anim_time = 0;
    g->frame_anim_time = 0;
    game_init_house(g->rooms);
    memset(g->creatures, 0, sizeof(g->creatures));
    memset(g->weights, 0, sizeof(g->weights));
    memset(g->bullets, 0, sizeof(g->bullets));
    memset(g->anims, 0, sizeof(g->anims));
    memset(g->cash, 0, sizeof(g->cash));
}
