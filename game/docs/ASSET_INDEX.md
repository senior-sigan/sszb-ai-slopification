# Asset Index: Save Soul of Zlaya Babka

Total: 60 files (51 textures, 11 sounds, 2 fonts)

---

## Textures (`assets/textures/`)

### Backgrounds & Scene (7 files)

| File | Description |
|------|-------------|
| `bg_day.gif` | Daytime background — 3-story apartment building (3x6 window grid), tan brick facade, gray roof, red fences, sidewalk, road, light blue sky |
| `bg_night.gif` | Nighttime background — same building, dark navy sky with stars, darker road/sidewalk |
| `club_day.gif` | Club "Lust" — daytime, purple brick facade, blue glass doors, gray sign with "CLUB Lust" text, unlit |
| `club_night_1.gif` | Club "Lust" — nighttime sprite sheet (2 frames), neon sign blinks green/pink for animation |
| `light_day.png` | Solid light blue square — daytime window fill color |
| `light_off.png` | Solid gray square — unlit window at night |
| `light_on.png` | Solid yellow square — lit window at night |

### Windows (8 files) — 4 states x 2 times of day

| File | State | Time | Description |
|------|-------|------|-------------|
| `window_normal_day.gif` | Intact | Day | Green frame, clean glass with highlight |
| `window_normal_night.gif` | Intact | Night | Same, slightly darker tones |
| `window_broken_day.gif` | Broken | Day | Gray-green frame, cracked glass with dark impact silhouette |
| `window_broken_night.gif` | Broken | Night | Same, night palette |
| `window_day_disabled.gif` | Disabled | Day | Dark blue frame, clean glass, non-interactive |
| `window_night_disabled.gif` | Disabled | Night | Same, night palette |
| `window_grid_day.gif` | Armored | Day | Green frame + gray vertical metal bars (protection upgrade) |
| `window_grid_night.gif` | Armored | Night | Same, night palette |

### Characters (5 files)

| File | Description |
|------|-------------|
| `babka_hands_up.gif` | Babka sprite — arms raised, red beret, brown coat, alarmed pose |
| `babka_in_window.gif` | Babka peeking from window/fence — green checkered shawl, grumpy face |
| `gopstop.gif` | Hooligan sprite sheet (7 frames) — gopnik in dark tracksuit, squat animation, green bottle |
| `whore.gif` | Selfie girl sprite sheet (7 frames) — blonde, pink top, blue skirt, red boots, phone/flash animation |
| `cash.gif` | Money icon — small green bill stack with "P" (ruble) symbol |

### Weapons & Projectiles (9 files)

| File | Type | Object | Description |
|------|------|--------|-------------|
| `pot.gif` | In-flight | Flower pot | Terracotta pot with green leaves and red tulips |
| `pot_crash_animation.gif` | Crash (4 frames) | Flower pot | Shatters, flowers scatter, dirt flies |
| `flower_in_window.gif` | Window preview | Flower pot | Single tulip peeking from window |
| `tv.gif` | In-flight | TV | Retro CRT with brown cabinet, blue screen, V-antenna |
| `tv_crash_animation.gif` | Crash (6 frames) | TV | Screen cracks, casing splits, smoke/dust |
| `tv_in_window.gif` | Window preview | TV | Antenna peeking from window |
| `royal.gif` | In-flight | Piano | Dark brown upright piano, visible keyboard |
| `piano_crash_animation.gif` | Crash (8 frames) | Piano | Elaborate destruction with golden sparkles (strings snapping) |
| `piano_in_window.gif` | Window preview | Piano | Piano visible in window |
| `bottle.png` | Projectile | Bottle | Tiny sprite, green/brown, enemy projectile (thrown by hooligans) |

### UI / HUD (6 files)

| File | Description |
|------|-------------|
| `hud.png` | Sprite sheet — babka portrait in grayscale, multiple frames/states |
| `hud_back.png` | HUD background layer — mostly transparent strip |
| `hud_front.png` | HUD foreground overlay — dark silhouette shapes over portrait |
| `frame.png` | Green selection frame — corner brackets for highlighting selected window |
| `logo.png` | Developer logo — "CAT_IN_THE_DARK_WOOD" with pixel cat, black background |
| `t2.png` | Russian text tutorial screen — key bindings on black background |

### Interface Buttons (8 files) — toolbar action panels with hotkey badges

| File | Hotkey | Description |
|------|--------|-------------|
| `inrerface_buy.gif` | 1 | Red coin purse — buy action |
| `inrerface_repair.gif` | 1 | Crossed hammer & screwdriver — repair action |
| `inrerface_grid.gif` | 2 | Window with vertical bars — buy armor/grate |
| `inrerface_weapon.gif` | 3 | Empty weapon slot (no weapon equipped) |
| `inrerface_weapon_flower.gif` | 3 | Weapon slot: flower pot equipped |
| `inrerface_weapon_tv.gif` | 3 | Weapon slot: CRT TV equipped |
| `inrerface_weapon_piano.gif` | 3 | Weapon slot: upright piano equipped |
| `interface_club.gif` | 4 | Purple nightclub building — buy club (win condition) |

Note: "inrerface" is a typo in original filenames (should be "interface").

### Screens (5 files)

| File | Description |
|------|-------------|
| `menu.gif` | Main menu — brick wall, babka in window, title "SAVE THE SOUL of zlaya babka", "PRESS ENTER TO PLAY" |
| `gameover.gif` | Game Over — brick wall, shattered window, red "GAME OVER" text |
| `gamewin.gif` | Victory — city street with apartment building, "KNITTING shop" replaces club, yellow "YOU WIN" |
| `tutor_1.gif` | Tutorial 1 — story intro: babka introduces herself, "I'm sick of dancing youngsters" |
| `tutor_2.gif` | Tutorial 2 — night phase: space to attack, enemy types (selfie girl blinds, hooligan breaks windows), club music damages |
| `tutor_3.gif` | Tutorial 3 — day phase: arrow keys to select flat, hotkeys 1-4 for buy/repair/armor/weaponize/club, currency "P:10" |

---

## Sounds (`assets/sound/`) — 11 files

### Ambient / Music (3 files)

| File | Size | Duration | Description |
|------|------|----------|-------------|
| `bgm_cool.mp3` | 832K | 53s | Background music — night phase theme |
| `birds.mp3` | 235K | 60s | Bird ambience — day phase background |
| `crickets.mp3` | 939K | 60s | Cricket ambience — night phase background |

### Sound Effects (8 files)

| File | Size | Duration | Description |
|------|------|----------|-------------|
| `pot_destroy.mp3` | 11K | 0.6s | Flower pot smashing on impact |
| `tv_destroy.mp3` | 11K | 0.6s | TV smashing on impact |
| `selfie.mp3` | 8K | 0.5s | Camera flash/selfie sound (whore enemy attack) |
| `iron.mp3` | 29K | 1.6s | Metal/iron impact sound |
| `bye.mp3` | 25K | 1.6s | Farewell/defeat sound |
| `round_end.mp3` | 32K | 2.0s | Night phase round ending |
| `window_broken.mp3` | 37K | 2.3s | Window glass breaking |
| `royal_deploy.mp3` | 48K | 3.0s | Piano dropping/deploying sound |

---

## Fonts (`assets/font/`) — 2 files

| File | Size | Description |
|------|------|-------------|
| `impact.ttf` | 133K | Impact font (Monotype, 1991-1996) — bold display text |
| `main.ttf` | 238K | Main game font — primary UI/HUD text |

---

## Asset Naming Conventions

- **Day/Night variants**: `*_day.*` / `*_night.*`
- **Window states**: `window_{state}_{time}.gif` where state = normal, broken, disabled, grid
- **Weapon lifecycle**: `{item}.gif` (in-flight) → `{item}_crash_animation.gif` (impact) + `{item}_in_window.gif` (preview)
- **Interface buttons**: `inrerface_{action}.gif` (note: typo in "inrerface")
- **Sprite sheets**: horizontal layout, frames side-by-side (gopstop=7, whore=7, pot_crash=4, tv_crash=6, piano_crash=8, club_night=2)
