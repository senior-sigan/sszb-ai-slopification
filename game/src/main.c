#include <raylib.h>
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef PLATFORM_WEB
#define COMMAND_SERVER_IMPLEMENTATION
#include "command_server.h"
#endif

#include "game_types.h"
#include "assets.h"
#include "night.h"
#include "day.h"

#ifndef PLATFORM_WEB
static Command frame_cmd;
static bool running = true;
#endif
static Game game;

bool ManagedIsKeyPressed(int key) {
#ifdef PLATFORM_WEB
    return IsKeyPressed(key);
#else
    return IsKeyPressed(key) || (frame_cmd.type == CMD_KEY_PRESS && frame_cmd.key_code == key);
#endif
}

bool ManagedIsMouseButtonPressed(int button) {
#ifdef PLATFORM_WEB
    return IsMouseButtonPressed(button);
#else
    return IsMouseButtonPressed(button) || (frame_cmd.type == CMD_MOUSE_PRESS && frame_cmd.mouse_button == button);
#endif
}

static void state_update(float dt) {
    switch (game.state) {
        case STATE_LOGO:
            game.state_timer += dt;
            if (game.state_timer >= 1.0f) {
                game.state = STATE_MENU;
            }
            break;

        case STATE_MENU:
            if (ManagedIsKeyPressed(KEY_ENTER)) {
                game.state = STATE_TUTOR1;
            }
            break;

        case STATE_TUTOR1:
            if (ManagedIsKeyPressed(KEY_ENTER)) {
                game.state = STATE_TUTOR2;
            }
            break;

        case STATE_TUTOR2:
            if (ManagedIsKeyPressed(KEY_ENTER)) {
                game.state = STATE_TUTOR3;
            }
            break;

        case STATE_TUTOR3:
            if (ManagedIsKeyPressed(KEY_ENTER)) {
                game.state = STATE_NIGHT;
            }
            break;

        case STATE_NIGHT:
            night_update(&game, dt);
            break;

        case STATE_DAY:
            day_update(&game, dt);
            break;

        case STATE_WIN:
            if (ManagedIsKeyPressed(KEY_ENTER)) {
                game_reset(&game);
                game.state = STATE_MENU;
            }
            break;

        case STATE_OVER:
            if (ManagedIsKeyPressed(KEY_ENTER)) {
                game_reset(&game);
                game.state = STATE_MENU;
            }
            break;
    }
}

static void state_render(void) {
    switch (game.state) {
        case STATE_LOGO: {
            int logo_x = (SCREEN_WIDTH - game.assets.logo.width) / 2;
            int logo_y = (SCREEN_HEIGHT - game.assets.logo.height) / 2;
            DrawTexture(game.assets.logo, logo_x, logo_y, WHITE);
        } break;

        case STATE_MENU:
            DrawTexture(game.assets.menu, 0, 0, WHITE);
            break;

        case STATE_TUTOR1:
            DrawTexture(game.assets.tutor1, 0, 0, WHITE);
            break;

        case STATE_TUTOR2:
            DrawTexture(game.assets.tutor2, 0, 0, WHITE);
            break;

        case STATE_TUTOR3:
            DrawTexture(game.assets.tutor3, 0, 0, WHITE);
            break;

        case STATE_NIGHT:
            night_render(&game);
            break;

        case STATE_DAY:
            day_render(&game);
            break;

        case STATE_WIN:
            DrawTexture(game.assets.game_win, 0, 0, WHITE);
            break;

        case STATE_OVER:
            DrawTexture(game.assets.game_over, 0, 0, WHITE);
            break;
    }
}

void update(void) {
    #ifndef PLATFORM_WEB
    // Poll TCP command
    frame_cmd = command_server_poll();

    // Handle TCP commands
    switch (frame_cmd.type) {
        case CMD_SCREENSHOT:
            TakeScreenshot(frame_cmd.filename);
            command_server_respond(true, "OK");
            break;

        case CMD_MOVE_MOUSE:
            SetMousePosition(frame_cmd.pos.x, frame_cmd.pos.y);
            command_server_respond(true, "OK");
            break;

        case CMD_KEY_PRESS:
            command_server_respond(true, "OK");
            break;

        case CMD_MOUSE_PRESS:
            command_server_respond(true, "OK");
            break;

        case CMD_QUIT:
            command_server_respond(true, "OK");
            running = false;
            break;

        case CMD_NONE:
            break;
    }
    #endif

    float dt = GetFrameTime();

    // Track previous state for transition detection
    GameState prev_state = game.state;

    // Update current state
    state_update(dt);

    // Detect state transitions
    if (prev_state != game.state) {
        // Exit handlers for old state
        switch (prev_state) {
            case STATE_NIGHT:
                night_exit(&game);
                break;
            case STATE_DAY:
                day_exit(&game);
                break;
            default:
                break;
        }

        // Enter handlers for new state
        switch (game.state) {
            case STATE_LOGO:
                game.state_timer = 0;
                break;
            case STATE_NIGHT:
                night_enter(&game);
                break;
            case STATE_DAY:
                day_enter(&game);
                break;
            case STATE_OVER:
                break;
            default:
                break;
        }
    }

    // Render
    BeginDrawing();
    ClearBackground(BLACK);
    state_render();
    DrawFPS(5, 5);
    EndDrawing();
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Save Soul of Zlaya Babka");
    srand((unsigned int)time(NULL));
    InitAudioDevice();
#ifndef PLATFORM_WEB
    command_server_init(CMD_PORT);
#endif

    assets_load(&game.assets);
    game_reset(&game);

#ifdef PLATFORM_WEB
    emscripten_set_main_loop(update, 0, 1);
#else
    SetTargetFPS(60);
    while (running && !WindowShouldClose()) {
        update();
    }
#endif

#ifndef PLATFORM_WEB
    command_server_cleanup();
#endif
    assets_unload(&game.assets);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
