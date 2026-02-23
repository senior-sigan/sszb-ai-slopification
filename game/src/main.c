#include <raylib.h>
#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef PLATFORM_WEB
#define COMMAND_SERVER_IMPLEMENTATION
#include "command_server.h"
#endif

#include "assets.h"
#include "day.h"
#include "game_types.h"
#include "night.h"

#ifndef PLATFORM_WEB
static Command frame_cmd;
static bool running = true;
#endif
static Game game;

#ifndef PLATFORM_WEB
static const char* StateName(GameState state) {
  switch (state) {
    case STATE_LOGO:
      return "LOGO";
    case STATE_MENU:
      return "MENU";
    case STATE_TUTOR1:
      return "TUTOR1";
    case STATE_TUTOR2:
      return "TUTOR2";
    case STATE_TUTOR3:
      return "TUTOR3";
    case STATE_NIGHT:
      return "NIGHT";
    case STATE_DAY:
      return "DAY";
    case STATE_WIN:
      return "WIN";
    case STATE_OVER:
      return "OVER";
  }
  return "UNKNOWN";
}

static int GameGetFieldInt(const Game* game_ptr, const char* field, bool* found) {
  *found = true;
  if (strcmp(field, "state") == 0) {
    return (int) game_ptr->state;
  }
  if (strcmp(field, "money") == 0) {
    return game_ptr->money;
  }
  if (strcmp(field, "hits") == 0) {
    return game_ptr->hits;
  }
  if (strcmp(field, "level") == 0) {
    return game_ptr->level;
  }
  if (strcmp(field, "cur_row") == 0) {
    return game_ptr->cur_row;
  }
  if (strcmp(field, "cur_col") == 0) {
    return game_ptr->cur_col;
  }
  if (strcmp(field, "club_bought") == 0) {
    return game_ptr->club_bought ? 1 : 0;
  }
  if (strcmp(field, "creatures") == 0) {
    int count = 0;
    for (int i = 0; i < MAX_CREATURES; i++) {
      if (game_ptr->creatures[i].active) {
        count++;
      }
    }
    return count;
  }
  *found = false;
  return 0;
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

static void StateUpdate(float delta) {
  switch (game.state) {
    case STATE_LOGO:
      game.state_timer += delta;
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
      NightUpdate(&game, delta);
      break;

    case STATE_DAY:
      DayUpdate(&game, delta);
      break;

    case STATE_WIN:
    case STATE_OVER:
      if (ManagedIsKeyPressed(KEY_ENTER)) {
        game_reset(&game);
        game.state = STATE_MENU;
      }
      break;
  }
}

static void StateRender(void) {
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
      NightRender(&game);
      break;

    case STATE_DAY:
      DayRender(&game);
      break;

    case STATE_WIN:
      DrawTexture(game.assets.game_win, 0, 0, WHITE);
      break;

    case STATE_OVER:
      DrawTexture(game.assets.game_over, 0, 0, WHITE);
      break;
  }
}

#ifndef PLATFORM_WEB
static void HandleAssertField(void) {
  bool found;
  int val = GameGetFieldInt(&game, frame_cmd.field_check.field, &found);
  if (!found) {
    script_runner.fail_count++;
    char msg[128];
    snprintf(msg, sizeof(msg), "unknown field '%s'", frame_cmd.field_check.field);
    script_runner_report(&script_runner, "FAIL", msg);
    return;
  }
  bool pass = false;
  const char* oper = "?";
  if (frame_cmd.type == CMD_ASSERT_EQ) {
    pass = (val == frame_cmd.field_check.value);
    oper = "==";
  } else if (frame_cmd.type == CMD_ASSERT_GE) {
    pass = (val >= frame_cmd.field_check.value);
    oper = ">=";
  } else if (frame_cmd.type == CMD_ASSERT_LE) {
    pass = (val <= frame_cmd.field_check.value);
    oper = "<=";
  }
  char msg[128];
  snprintf(msg, sizeof(msg), "ASSERT %s %s %d (actual=%d)", frame_cmd.field_check.field, oper,
           frame_cmd.field_check.value, val);
  if (pass) {
    script_runner.pass_count++;
    script_runner_report(&script_runner, "PASS", msg);
  } else {
    script_runner.fail_count++;
    script_runner_report(&script_runner, "FAIL", msg);
  }
}

static void HandleCommand(void) {
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
      if (!script_runner.active) {
        command_server_respond(true, "OK");
      }
      break;

    case CMD_KEY_PRESS:
    case CMD_MOUSE_PRESS:
      if (!script_runner.active) {
        command_server_respond(true, "OK");
      }
      break;

    case CMD_QUIT:
      if (!script_runner.active) {
        command_server_respond(true, "OK");
      }
      running = false;
      break;

    case CMD_ASSERT_STATE: {
      char msg[128];
      if ((int) game.state == frame_cmd.assert_state) {
        script_runner.pass_count++;
        snprintf(msg, sizeof(msg), "ASSERT_STATE %s", StateName(game.state));
        script_runner_report(&script_runner, "PASS", msg);
      } else {
        script_runner.fail_count++;
        snprintf(msg, sizeof(msg), "ASSERT_STATE expected %s got %s", StateName((GameState) frame_cmd.assert_state),
                 StateName(game.state));
        script_runner_report(&script_runner, "FAIL", msg);
      }
    } break;

    case CMD_ASSERT_EQ:
    case CMD_ASSERT_GE:
    case CMD_ASSERT_LE:
      HandleAssertField();
      break;

    case CMD_GET: {
      bool found;
      int val = GameGetFieldInt(&game, frame_cmd.field_check.field, &found);
      if (found) {
        script_runner_respond(&script_runner, "%s=%d", frame_cmd.field_check.field, val);
      } else {
        script_runner_respond(&script_runner, "ERROR unknown field '%s'", frame_cmd.field_check.field);
      }
    } break;

    case CMD_NONE:
    case CMD_SCRIPT:
    case CMD_WAIT:
    case CMD_WAIT_STATE:
    case CMD_LOG:
      break;
  }
}
#endif

void Update(void) {
#ifndef PLATFORM_WEB
  // Poll TCP command
  frame_cmd = command_server_poll();

  // Handle TCP commands
  HandleCommand();
#endif

  float delta = GetFrameTime();

  // Track previous state for transition detection
  GameState prev_state = game.state;

  // Update current state
  StateUpdate(delta);

  // Check WAIT_STATE completion
#ifndef PLATFORM_WEB
  if (script_runner.active && script_runner.wait_state_target >= 0) {
    if ((int) game.state == script_runner.wait_state_target) {
      char msg[128];
      snprintf(msg, sizeof(msg), "WAIT_STATE %s reached", StateName(game.state));
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
        NightExit(&game);
        break;
      case STATE_DAY:
        DayExit(&game);
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
        NightEnter(&game);
        break;
      case STATE_DAY:
        DayEnter(&game);
        break;
      default:
        break;
    }
  }

  // Render
  BeginDrawing();
  ClearBackground(BLACK);
  StateRender();
  DrawFPS(5, 5);
  EndDrawing();
}

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Save Soul of Zlaya Babka");
  srand((unsigned int) time(NULL));
  InitAudioDevice();
#ifndef PLATFORM_WEB
  command_server_init(CMD_PORT);
#endif

  assets_load(&game.assets);
  game_reset(&game);

#ifdef PLATFORM_WEB
  emscripten_set_main_loop(Update, 0, 1);
#else
  SetTargetFPS(60);
  while (running && !WindowShouldClose()) {
    Update();
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
