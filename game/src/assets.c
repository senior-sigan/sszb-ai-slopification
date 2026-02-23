#include "assets.h"

#include <raylib.h>
#include <stdio.h>

#include "game_types.h"

void assets_load(GameAssets* assets) {
  // Screens
  assets->logo = LoadTexture("assets/textures/logo.png");
  assets->menu = LoadTexture("assets/textures/menu.gif");
  assets->tutor1 = LoadTexture("assets/textures/tutor_1.gif");
  assets->tutor2 = LoadTexture("assets/textures/tutor_2.gif");
  assets->tutor3 = LoadTexture("assets/textures/tutor_3.gif");
  assets->game_over = LoadTexture("assets/textures/gameover.gif");
  assets->game_win = LoadTexture("assets/textures/gamewin.gif");
  // Backgrounds
  assets->bg_night = LoadTexture("assets/textures/bg_night.gif");
  assets->bg_day = LoadTexture("assets/textures/bg_day.gif");
  assets->club_day = LoadTexture("assets/textures/club_day.gif");
  // Windows night
  assets->wnd_night_normal = LoadTexture("assets/textures/window_normal_night.gif");
  assets->wnd_night_broken = LoadTexture("assets/textures/window_broken_night.gif");
  assets->wnd_night_grate = LoadTexture("assets/textures/window_grid_night.gif");
  assets->wnd_night_disabled = LoadTexture("assets/textures/window_night_disabled.gif");
  // Windows day
  assets->wnd_day_normal = LoadTexture("assets/textures/window_normal_day.gif");
  assets->wnd_day_broken = LoadTexture("assets/textures/window_broken_day.gif");
  assets->wnd_day_grate = LoadTexture("assets/textures/window_grid_day.gif");
  assets->wnd_day_disabled = LoadTexture("assets/textures/window_day_disabled.gif");
  // In-window
  assets->flower_in_window = LoadTexture("assets/textures/flower_in_window.gif");
  assets->tv_in_window = LoadTexture("assets/textures/tv_in_window.gif");
  assets->piano_in_window = LoadTexture("assets/textures/piano_in_window.gif");
  // Lights
  assets->light_on = LoadTexture("assets/textures/light_on.png");
  assets->light_off = LoadTexture("assets/textures/light_off.png");
  assets->light_day = LoadTexture("assets/textures/light_day.png");
  // Babka
  assets->babka_in_window = LoadTexture("assets/textures/babka_in_window.gif");
  assets->babka_hands_up = LoadTexture("assets/textures/babka_hands_up.gif");
  // Weapons
  assets->pot_tex = LoadTexture("assets/textures/pot.gif");
  assets->tv_tex = LoadTexture("assets/textures/tv.gif");
  assets->royal_tex = LoadTexture("assets/textures/royal.gif");
  assets->bottle = LoadTexture("assets/textures/bottle.png");
  // Cash, HUD
  assets->cash = LoadTexture("assets/textures/cash.gif");
  assets->hud_back = LoadTexture("assets/textures/hud_back.png");
  assets->hud_main = LoadTexture("assets/textures/hud.png");
  assets->hud_front = LoadTexture("assets/textures/hud_front.png");
  // Shop UI (note: typo "inrerface" is intentional in filenames)
  assets->shop_buy = LoadTexture("assets/textures/inrerface_buy.gif");
  assets->shop_repair = LoadTexture("assets/textures/inrerface_repair.gif");
  assets->shop_grate = LoadTexture("assets/textures/inrerface_grid.gif");
  assets->shop_weapon = LoadTexture("assets/textures/inrerface_weapon.gif");
  assets->shop_weapon_pot = LoadTexture("assets/textures/inrerface_weapon_flower.gif");
  assets->shop_weapon_tv = LoadTexture("assets/textures/inrerface_weapon_tv.gif");
  assets->shop_weapon_royal = LoadTexture("assets/textures/inrerface_weapon_piano.gif");
  assets->shop_club = LoadTexture("assets/textures/interface_club.gif");
  // Sprite sheets
  assets->whore_sheet = LoadTexture("assets/textures/whore.gif");
  assets->gopstop_sheet = LoadTexture("assets/textures/gopstop.gif");
  assets->pot_crash_sheet = LoadTexture("assets/textures/pot_crash_animation.gif");
  assets->tv_crash_sheet = LoadTexture("assets/textures/tv_crash_animation.gif");
  assets->piano_crash_sheet = LoadTexture("assets/textures/piano_crash_animation.gif");
  assets->club_night_sheet = LoadTexture("assets/textures/club_night_1.gif");
  assets->frame_sheet = LoadTexture("assets/textures/frame.png");
  // Animations
  sprite_anim_setup(&assets->anim_whore_walk, assets->whore_sheet, 88, 128, (int[]) {0, 1, 2, 3}, 4, ANIM_NORMAL, true);
  sprite_anim_setup(&assets->anim_whore_attack, assets->whore_sheet, 88, 128, (int[]) {4, 5, 6, 7}, 4, ANIM_NORMAL,
                    true);
  sprite_anim_setup(&assets->anim_hooligan_walk, assets->gopstop_sheet, 120, 128, (int[]) {0, 1}, 2, ANIM_NORMAL, true);
  sprite_anim_setup(&assets->anim_hooligan_attack, assets->gopstop_sheet, 120, 128, (int[]) {2, 3, 4, 5, 6}, 5,
                    ANIM_NORMAL, false);
  sprite_anim_setup(&assets->anim_hooligan_die, assets->gopstop_sheet, 120, 128, (int[]) {5, 6, 5, 6}, 4, ANIM_NORMAL,
                    false);
  sprite_anim_setup(&assets->anim_whore_die, assets->whore_sheet, 88, 128, (int[]) {1, 3}, 2, ANIM_NORMAL, false);
  sprite_anim_setup(&assets->anim_pot_crash, assets->pot_crash_sheet, 80, 104, (int[]) {0, 1, 2, 3}, 4, ANIM_FAST,
                    false);
  sprite_anim_setup(&assets->anim_tv_crash, assets->tv_crash_sheet, 160, 96, (int[]) {0, 1, 2, 3, 4, 5}, 6, ANIM_FAST,
                    false);
  sprite_anim_setup(&assets->anim_royal_crash, assets->piano_crash_sheet, 200, 128, (int[]) {0, 1, 2, 3, 4, 5, 6, 7}, 8,
                    ANIM_FAST, false);
  sprite_anim_setup(&assets->anim_club_night, assets->club_night_sheet, 382, 484, (int[]) {0, 1}, 2, 0.6f, true);
  sprite_anim_setup(&assets->anim_frame, assets->frame_sheet, 138, 138, (int[]) {0, 1}, 2, 0.2f, true);
  // Font
  assets->main_font = LoadFontEx("assets/font/main.ttf", 44, NULL, 0);
  // Music
  assets->bgm_cool = LoadMusicStream("assets/sound/bgm_cool.mp3");
  assets->bgm_cool.looping = true;
  assets->bgm_crickets = LoadMusicStream("assets/sound/crickets.mp3");
  assets->bgm_crickets.looping = true;
  SetMusicVolume(assets->bgm_crickets, 0.2f);
  assets->bgm_birds = LoadMusicStream("assets/sound/birds.mp3");
  assets->bgm_birds.looping = true;
  SetMusicVolume(assets->bgm_birds, 0.2f);
  // SFX
  assets->snd_pot_destroy = LoadSound("assets/sound/pot_destroy.mp3");
  assets->snd_tv_destroy = LoadSound("assets/sound/tv_destroy.mp3");
  assets->snd_royal_deploy = LoadSound("assets/sound/royal_deploy.mp3");
  assets->snd_window_broken = LoadSound("assets/sound/window_broken.mp3");
  assets->snd_selfie = LoadSound("assets/sound/selfie.mp3");
  assets->snd_iron = LoadSound("assets/sound/iron.mp3");
  assets->snd_bye = LoadSound("assets/sound/bye.mp3");
  assets->snd_round_end = LoadSound("assets/sound/round_end.mp3");
}

void assets_unload(GameAssets* assets) {
  // Unload all textures
  UnloadTexture(assets->logo);
  UnloadTexture(assets->menu);
  UnloadTexture(assets->tutor1);
  UnloadTexture(assets->tutor2);
  UnloadTexture(assets->tutor3);
  UnloadTexture(assets->game_over);
  UnloadTexture(assets->game_win);
  UnloadTexture(assets->bg_night);
  UnloadTexture(assets->bg_day);
  UnloadTexture(assets->club_day);
  UnloadTexture(assets->wnd_night_normal);
  UnloadTexture(assets->wnd_night_broken);
  UnloadTexture(assets->wnd_night_grate);
  UnloadTexture(assets->wnd_night_disabled);
  UnloadTexture(assets->wnd_day_normal);
  UnloadTexture(assets->wnd_day_broken);
  UnloadTexture(assets->wnd_day_grate);
  UnloadTexture(assets->wnd_day_disabled);
  UnloadTexture(assets->flower_in_window);
  UnloadTexture(assets->tv_in_window);
  UnloadTexture(assets->piano_in_window);
  UnloadTexture(assets->light_on);
  UnloadTexture(assets->light_off);
  UnloadTexture(assets->light_day);
  UnloadTexture(assets->babka_in_window);
  UnloadTexture(assets->babka_hands_up);
  UnloadTexture(assets->pot_tex);
  UnloadTexture(assets->tv_tex);
  UnloadTexture(assets->royal_tex);
  UnloadTexture(assets->bottle);
  UnloadTexture(assets->cash);
  UnloadTexture(assets->hud_back);
  UnloadTexture(assets->hud_main);
  UnloadTexture(assets->hud_front);
  UnloadTexture(assets->shop_buy);
  UnloadTexture(assets->shop_repair);
  UnloadTexture(assets->shop_grate);
  UnloadTexture(assets->shop_weapon);
  UnloadTexture(assets->shop_weapon_pot);
  UnloadTexture(assets->shop_weapon_tv);
  UnloadTexture(assets->shop_weapon_royal);
  UnloadTexture(assets->shop_club);
  UnloadTexture(assets->whore_sheet);
  UnloadTexture(assets->gopstop_sheet);
  UnloadTexture(assets->pot_crash_sheet);
  UnloadTexture(assets->tv_crash_sheet);
  UnloadTexture(assets->piano_crash_sheet);
  UnloadTexture(assets->club_night_sheet);
  UnloadTexture(assets->frame_sheet);
  UnloadFont(assets->main_font);
  UnloadMusicStream(assets->bgm_cool);
  UnloadMusicStream(assets->bgm_crickets);
  UnloadMusicStream(assets->bgm_birds);
  UnloadSound(assets->snd_pot_destroy);
  UnloadSound(assets->snd_tv_destroy);
  UnloadSound(assets->snd_royal_deploy);
  UnloadSound(assets->snd_window_broken);
  UnloadSound(assets->snd_selfie);
  UnloadSound(assets->snd_iron);
  UnloadSound(assets->snd_bye);
  UnloadSound(assets->snd_round_end);
}
