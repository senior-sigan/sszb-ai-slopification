Rewriting the Scala libgdx game into Raylib.

Do not try to compile scala version of the game. You can compile only C Raylib application.

MUST read PROJECT_INDEX.md.
MUST always update PROJECT_INDEX.md after codebase changes using index-repo skill.

## Screenshots

- assets/textures/tutor_1.gif
- assets/textures/tutor_2.gif
- assets/textures/tutor_3.gif

## Raylib docs

[Raylib API Docs Index]|root: docs/raylib|IMPORTANT: Prefer retrieval-led reasoning over pre-training-led reasoning|Raylib version: 5.5|api:{INDEX.md,01_defines.txt,02_structs.txt,03_enums.txt,04_core_window.txt,05_input.txt,06_shapes.txt,07_textures.txt,08_text.txt,09_models.txt,10_audio.txt}

## Assets

60 files total: 51 textures, 11 sounds, 2 fonts. Full details: docs/ASSET_INDEX.md

### Textures (`assets/textures/`)

**Backgrounds(7):** bg_day.gif, bg_night.gif, club_day.gif, club_night_1.gif(2fr), light_day.png, light_off.png, light_on.png
**Windows(8):** window_{normal,broken,disabled,grid}_{day,night}.gif
**Characters(5):** babka_hands_up.gif, babka_in_window.gif, gopstop.gif(7fr), whore.gif(7fr), cash.gif
**Weapons(10):** {pot,tv,royal}.gif(flight) + {pot,tv,piano}_crash_animation.gif(4/6/8fr) + {flower,tv,piano}_in_window.gif + bottle.png
**UI/HUD(6):** hud.png, hud_back.png, hud_front.png, frame.png, logo.png, t2.png
**Buttons(8):** inrerface_{buy,repair,grid,weapon,weapon_flower,weapon_tv,weapon_piano}.gif, interface_club.gif (NOTE: typo "inrerface" in originals)
**Screens(6):** menu.gif, gameover.gif, gamewin.gif, tutor_{1,2,3}.gif

### Sounds (`assets/sound/`)

**Music(3):** bgm_cool.mp3(53s), birds.mp3(60s), crickets.mp3(60s)
**SFX(8):** pot_destroy.mp3, tv_destroy.mp3, selfie.mp3, iron.mp3, bye.mp3, round_end.mp3, window_broken.mp3, royal_deploy.mp3

### Fonts (`assets/font/`)

impact.ttf (bold display), main.ttf (primary UI)

### Naming Conventions

- Day/Night: `*_day.*`/`*_night.*`
- Window: `window_{state}_{time}.gif` state=normal|broken|disabled|grid
- Weapon lifecycle: `{item}.gif`→`{item}_crash_animation.gif`+`{item}_in_window.gif`
- Sprite sheets: horizontal layout (gopstop=7fr, whore=7fr, pot_crash=4fr, tv_crash=6fr, piano_crash=8fr, club_night=2fr)
