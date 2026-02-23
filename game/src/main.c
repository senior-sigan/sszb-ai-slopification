#include <raylib.h>
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#include <stdarg.h>
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

#ifndef PLATFORM_WEB
static const char *state_name(GameState s) {
    switch (s) {
        case STATE_LOGO: return "LOGO";
        case STATE_MENU: return "MENU";
        case STATE_TUTOR1: return "TUTOR1";
        case STATE_TUTOR2: return "TUTOR2";
        case STATE_TUTOR3: return "TUTOR3";
        case STATE_NIGHT: return "NIGHT";
        case STATE_DAY: return "DAY";
        case STATE_WIN: return "WIN";
        case STATE_OVER: return "OVER";
    }
    return "UNKNOWN";
}

static int game_get_field_int(const Game *g, const char *field, bool *ok) {
    *ok = true;
    if (strcmp(field, "state") == 0) return (int)g->state;
    if (strcmp(field, "money") == 0) return g->money;
    if (strcmp(field, "hits") == 0) return g->hits;
    if (strcmp(field, "level") == 0) return g->level;
    if (strcmp(field, "cur_row") == 0) return g->cur_row;
    if (strcmp(field, "cur_col") == 0) return g->cur_col;
    if (strcmp(field, "club_bought") == 0) return g->club_bought ? 1 : 0;
    if (strcmp(field, "creatures") == 0) {
        int count = 0;
        for (int i = 0; i < MAX_CREATURES; i++)
            if (g->creatures[i].active) count++;
        return count;
    }
    *ok = false;
    return 0;
}

static void script_respond(const char *fmt, ...) {
    if (script_runner.client_fd < 0) return;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    buf[len] = '\n';
    send(script_runner.client_fd, buf, (size_t)(len + 1), 0);
}
#endif

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
            if (script_runner.active) {
                script_runner_report(&script_runner, "SHOT", frame_cmd.filename);
            } else {
                command_server_respond(true, "OK");
            }
            break;

        case CMD_MOVE_MOUSE:
            SetMousePosition(frame_cmd.pos.x, frame_cmd.pos.y);
            if (!script_runner.active) command_server_respond(true, "OK");
            break;

        case CMD_KEY_PRESS:
            if (!script_runner.active) command_server_respond(true, "OK");
            break;

        case CMD_MOUSE_PRESS:
            if (!script_runner.active) command_server_respond(true, "OK");
            break;

        case CMD_QUIT:
            if (!script_runner.active) command_server_respond(true, "OK");
            running = false;
            break;

        case CMD_ASSERT_STATE: {
            char msg[128];
            if ((int)game.state == frame_cmd.assert_state) {
                script_runner.pass_count++;
                snprintf(msg, sizeof(msg), "ASSERT_STATE %s", state_name(game.state));
                script_runner_report(&script_runner, "PASS", msg);
            } else {
                script_runner.fail_count++;
                snprintf(msg, sizeof(msg), "ASSERT_STATE expected %s got %s",
                         state_name((GameState)frame_cmd.assert_state), state_name(game.state));
                script_runner_report(&script_runner, "FAIL", msg);
            }
        } break;

        case CMD_ASSERT_EQ:
        case CMD_ASSERT_GE:
        case CMD_ASSERT_LE: {
            bool ok;
            int val = game_get_field_int(&game, frame_cmd.field_check.field, &ok);
            if (!ok) {
                script_runner.fail_count++;
                char msg[128];
                snprintf(msg, sizeof(msg), "unknown field '%s'", frame_cmd.field_check.field);
                script_runner_report(&script_runner, "FAIL", msg);
            } else {
                bool pass = false;
                const char *op = "?";
                if (frame_cmd.type == CMD_ASSERT_EQ) { pass = (val == frame_cmd.field_check.value); op = "=="; }
                else if (frame_cmd.type == CMD_ASSERT_GE) { pass = (val >= frame_cmd.field_check.value); op = ">="; }
                else if (frame_cmd.type == CMD_ASSERT_LE) { pass = (val <= frame_cmd.field_check.value); op = "<="; }
                char msg[128];
                snprintf(msg, sizeof(msg), "ASSERT %s %s %d (actual=%d)",
                         frame_cmd.field_check.field, op, frame_cmd.field_check.value, val);
                if (pass) {
                    script_runner.pass_count++;
                    script_runner_report(&script_runner, "PASS", msg);
                } else {
                    script_runner.fail_count++;
                    script_runner_report(&script_runner, "FAIL", msg);
                }
            }
        } break;

        case CMD_GET: {
            bool ok;
            int val = game_get_field_int(&game, frame_cmd.field_check.field, &ok);
            if (ok) {
                script_respond("%s=%d", frame_cmd.field_check.field, val);
            } else {
                script_respond("ERROR unknown field '%s'", frame_cmd.field_check.field);
            }
        } break;

        case CMD_NONE:
        case CMD_SCRIPT:
        case CMD_WAIT:
        case CMD_WAIT_STATE:
        case CMD_LOG:
            break;
    }
    #endif

    float dt = GetFrameTime();

    // Track previous state for transition detection
    GameState prev_state = game.state;

    // Update current state
    state_update(dt);

    // Check WAIT_STATE completion
    #ifndef PLATFORM_WEB
    if (script_runner.active && script_runner.wait_state_target >= 0) {
        if ((int)game.state == script_runner.wait_state_target) {
            char msg[128];
            snprintf(msg, sizeof(msg), "WAIT_STATE %s reached", state_name(game.state));
            script_runner_report(&script_runner, "OK", msg);
            script_runner.wait_state_target = -1;
        }
    }
    #endif

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
