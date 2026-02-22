#include "assets.h"
#include <stdio.h>

void assets_load(GameAssets *a) {
    // Screens
    a->logo = LoadTexture("assets/textures/logo.png");
    a->menu = LoadTexture("assets/textures/menu.gif");
    a->tutor1 = LoadTexture("assets/textures/tutor_1.gif");
    a->tutor2 = LoadTexture("assets/textures/tutor_2.gif");
    a->tutor3 = LoadTexture("assets/textures/tutor_3.gif");
    a->game_over = LoadTexture("assets/textures/gameover.gif");
    a->game_win = LoadTexture("assets/textures/gamewin.gif");
    // Backgrounds
    a->bg_night = LoadTexture("assets/textures/bg_night.gif");
    a->bg_day = LoadTexture("assets/textures/bg_day.gif");
    a->club_day = LoadTexture("assets/textures/club_day.gif");
    // Windows night
    a->wnd_night_normal = LoadTexture("assets/textures/window_normal_night.gif");
    a->wnd_night_broken = LoadTexture("assets/textures/window_broken_night.gif");
    a->wnd_night_grate = LoadTexture("assets/textures/window_grid_night.gif");
    a->wnd_night_disabled = LoadTexture("assets/textures/window_night_disabled.gif");
    // Windows day
    a->wnd_day_normal = LoadTexture("assets/textures/window_normal_day.gif");
    a->wnd_day_broken = LoadTexture("assets/textures/window_broken_day.gif");
    a->wnd_day_grate = LoadTexture("assets/textures/window_grid_day.gif");
    a->wnd_day_disabled = LoadTexture("assets/textures/window_day_disabled.gif");
    // In-window
    a->flower_in_window = LoadTexture("assets/textures/flower_in_window.gif");
    a->tv_in_window = LoadTexture("assets/textures/tv_in_window.gif");
    a->piano_in_window = LoadTexture("assets/textures/piano_in_window.gif");
    // Lights
    a->light_on = LoadTexture("assets/textures/light_on.png");
    a->light_off = LoadTexture("assets/textures/light_off.png");
    a->light_day = LoadTexture("assets/textures/light_day.png");
    // Babka
    a->babka_in_window = LoadTexture("assets/textures/babka_in_window.gif");
    a->babka_hands_up = LoadTexture("assets/textures/babka_hands_up.gif");
    // Weapons
    a->pot_tex = LoadTexture("assets/textures/pot.gif");
    a->tv_tex = LoadTexture("assets/textures/tv.gif");
    a->royal_tex = LoadTexture("assets/textures/royal.gif");
    a->bottle = LoadTexture("assets/textures/bottle.png");
    // Cash, HUD
    a->cash = LoadTexture("assets/textures/cash.gif");
    a->hud_back = LoadTexture("assets/textures/hud_back.png");
    a->hud_main = LoadTexture("assets/textures/hud.png");
    a->hud_front = LoadTexture("assets/textures/hud_front.png");
    // Shop UI (note: typo "inrerface" is intentional in filenames)
    a->shop_buy = LoadTexture("assets/textures/inrerface_buy.gif");
    a->shop_repair = LoadTexture("assets/textures/inrerface_repair.gif");
    a->shop_grate = LoadTexture("assets/textures/inrerface_grid.gif");
    a->shop_weapon = LoadTexture("assets/textures/inrerface_weapon.gif");
    a->shop_weapon_pot = LoadTexture("assets/textures/inrerface_weapon_flower.gif");
    a->shop_weapon_tv = LoadTexture("assets/textures/inrerface_weapon_tv.gif");
    a->shop_weapon_royal = LoadTexture("assets/textures/inrerface_weapon_piano.gif");
    a->shop_club = LoadTexture("assets/textures/interface_club.gif");
    // Sprite sheets
    a->whore_sheet = LoadTexture("assets/textures/whore.gif");
    a->gopstop_sheet = LoadTexture("assets/textures/gopstop.gif");
    a->pot_crash_sheet = LoadTexture("assets/textures/pot_crash_animation.gif");
    a->tv_crash_sheet = LoadTexture("assets/textures/tv_crash_animation.gif");
    a->piano_crash_sheet = LoadTexture("assets/textures/piano_crash_animation.gif");
    a->club_night_sheet = LoadTexture("assets/textures/club_night_1.gif");
    a->frame_sheet = LoadTexture("assets/textures/frame.png");
    // Animations
    sprite_anim_setup(&a->anim_whore_walk, a->whore_sheet, 88, 128, (int[]){0,1,2,3}, 4, ANIM_NORMAL, true);
    sprite_anim_setup(&a->anim_whore_attack, a->whore_sheet, 88, 128, (int[]){4,5,6,7}, 4, ANIM_NORMAL, true);
    sprite_anim_setup(&a->anim_hooligan_walk, a->gopstop_sheet, 120, 128, (int[]){0,1}, 2, ANIM_NORMAL, true);
    sprite_anim_setup(&a->anim_hooligan_attack, a->gopstop_sheet, 120, 128, (int[]){2,3,4,5,6}, 5, ANIM_NORMAL, false);
    sprite_anim_setup(&a->anim_hooligan_die, a->gopstop_sheet, 120, 128, (int[]){5,6,5,6}, 4, ANIM_NORMAL, false);
    sprite_anim_setup(&a->anim_whore_die, a->whore_sheet, 88, 128, (int[]){1,3}, 2, ANIM_NORMAL, false);
    sprite_anim_setup(&a->anim_pot_crash, a->pot_crash_sheet, 80, 104, (int[]){0,1,2,3}, 4, ANIM_FAST, false);
    sprite_anim_setup(&a->anim_tv_crash, a->tv_crash_sheet, 160, 96, (int[]){0,1,2,3,4,5}, 6, ANIM_FAST, false);
    sprite_anim_setup(&a->anim_royal_crash, a->piano_crash_sheet, 200, 128, (int[]){0,1,2,3,4,5,6,7}, 8, ANIM_FAST, false);
    sprite_anim_setup(&a->anim_club_night, a->club_night_sheet, 382, 484, (int[]){0,1}, 2, 0.6f, true);
    sprite_anim_setup(&a->anim_frame, a->frame_sheet, 138, 138, (int[]){0,1}, 2, 0.2f, true);
    // Font
    a->main_font = LoadFontEx("assets/font/main.ttf", 44, NULL, 0);
    // Music
    a->bgm_cool = LoadMusicStream("assets/sound/bgm_cool.mp3");
    a->bgm_cool.looping = true;
    a->bgm_crickets = LoadMusicStream("assets/sound/crickets.mp3");
    a->bgm_crickets.looping = true;
    SetMusicVolume(a->bgm_crickets, 0.2f);
    a->bgm_birds = LoadMusicStream("assets/sound/birds.mp3");
    a->bgm_birds.looping = true;
    SetMusicVolume(a->bgm_birds, 0.2f);
    // SFX
    a->snd_pot_destroy = LoadSound("assets/sound/pot_destroy.mp3");
    a->snd_tv_destroy = LoadSound("assets/sound/tv_destroy.mp3");
    a->snd_royal_deploy = LoadSound("assets/sound/royal_deploy.mp3");
    a->snd_window_broken = LoadSound("assets/sound/window_broken.mp3");
    a->snd_selfie = LoadSound("assets/sound/selfie.mp3");
    a->snd_iron = LoadSound("assets/sound/iron.mp3");
    a->snd_bye = LoadSound("assets/sound/bye.mp3");
    a->snd_round_end = LoadSound("assets/sound/round_end.mp3");
}

void assets_unload(GameAssets *a) {
    // Unload all textures
    UnloadTexture(a->logo);
    UnloadTexture(a->menu);
    UnloadTexture(a->tutor1);
    UnloadTexture(a->tutor2);
    UnloadTexture(a->tutor3);
    UnloadTexture(a->game_over);
    UnloadTexture(a->game_win);
    UnloadTexture(a->bg_night);
    UnloadTexture(a->bg_day);
    UnloadTexture(a->club_day);
    UnloadTexture(a->wnd_night_normal);
    UnloadTexture(a->wnd_night_broken);
    UnloadTexture(a->wnd_night_grate);
    UnloadTexture(a->wnd_night_disabled);
    UnloadTexture(a->wnd_day_normal);
    UnloadTexture(a->wnd_day_broken);
    UnloadTexture(a->wnd_day_grate);
    UnloadTexture(a->wnd_day_disabled);
    UnloadTexture(a->flower_in_window);
    UnloadTexture(a->tv_in_window);
    UnloadTexture(a->piano_in_window);
    UnloadTexture(a->light_on);
    UnloadTexture(a->light_off);
    UnloadTexture(a->light_day);
    UnloadTexture(a->babka_in_window);
    UnloadTexture(a->babka_hands_up);
    UnloadTexture(a->pot_tex);
    UnloadTexture(a->tv_tex);
    UnloadTexture(a->royal_tex);
    UnloadTexture(a->bottle);
    UnloadTexture(a->cash);
    UnloadTexture(a->hud_back);
    UnloadTexture(a->hud_main);
    UnloadTexture(a->hud_front);
    UnloadTexture(a->shop_buy);
    UnloadTexture(a->shop_repair);
    UnloadTexture(a->shop_grate);
    UnloadTexture(a->shop_weapon);
    UnloadTexture(a->shop_weapon_pot);
    UnloadTexture(a->shop_weapon_tv);
    UnloadTexture(a->shop_weapon_royal);
    UnloadTexture(a->shop_club);
    UnloadTexture(a->whore_sheet);
    UnloadTexture(a->gopstop_sheet);
    UnloadTexture(a->pot_crash_sheet);
    UnloadTexture(a->tv_crash_sheet);
    UnloadTexture(a->piano_crash_sheet);
    UnloadTexture(a->club_night_sheet);
    UnloadTexture(a->frame_sheet);
    UnloadFont(a->main_font);
    UnloadMusicStream(a->bgm_cool);
    UnloadMusicStream(a->bgm_crickets);
    UnloadMusicStream(a->bgm_birds);
    UnloadSound(a->snd_pot_destroy);
    UnloadSound(a->snd_tv_destroy);
    UnloadSound(a->snd_royal_deploy);
    UnloadSound(a->snd_window_broken);
    UnloadSound(a->snd_selfie);
    UnloadSound(a->snd_iron);
    UnloadSound(a->snd_bye);
    UnloadSound(a->snd_round_end);
}
