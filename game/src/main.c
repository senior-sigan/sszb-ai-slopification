#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_server.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define CMD_PORT 9999
#define PLAYER_SPEED 10
#define PLAYER_RADIUS 20

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Game");
  SetTargetFPS(60);
  command_server_init(CMD_PORT);

  Vector2 player_pos = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
  Vector2 click_pos = {-1, -1};
  bool show_click = false;
  char last_cmd_text[128] = "None";

  while (!WindowShouldClose()) {
    // Real keyboard input
    if (IsKeyPressed(KEY_RIGHT)) player_pos.x += PLAYER_SPEED;
    if (IsKeyPressed(KEY_LEFT)) player_pos.x -= PLAYER_SPEED;
    if (IsKeyPressed(KEY_DOWN)) player_pos.y += PLAYER_SPEED;
    if (IsKeyPressed(KEY_UP)) player_pos.y -= PLAYER_SPEED;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      click_pos = GetMousePosition();
      show_click = true;
    }

    // TCP command processing
    Command cmd = command_server_poll();
    switch (cmd.type) {
      case CMD_SCREENSHOT:
        TakeScreenshot(cmd.data);
        command_server_respond(true, "OK");
        snprintf(last_cmd_text, sizeof(last_cmd_text), "SCREENSHOT %s",
                 cmd.data);
        break;

      case CMD_KEY_PRESS: {
        int key = atoi(cmd.data);
        switch (key) {
          case KEY_RIGHT:
            player_pos.x += PLAYER_SPEED;
            break;
          case KEY_LEFT:
            player_pos.x -= PLAYER_SPEED;
            break;
          case KEY_DOWN:
            player_pos.y += PLAYER_SPEED;
            break;
          case KEY_UP:
            player_pos.y -= PLAYER_SPEED;
            break;
          default:
            break;
        }
        command_server_respond(true, "OK");
        snprintf(last_cmd_text, sizeof(last_cmd_text), "KEY_PRESS %d", key);
        break;
      }

      case CMD_MOUSE_PRESS: {
        int button = atoi(cmd.data);
        if (button == MOUSE_BUTTON_LEFT) {
          click_pos = GetMousePosition();
          show_click = true;
        }
        command_server_respond(true, "OK");
        snprintf(last_cmd_text, sizeof(last_cmd_text), "MOUSE_PRESS %d",
                 button);
        break;
      }

      case CMD_MOVE_MOUSE: {
        int x = 0, y = 0;
        sscanf(cmd.data, "%d %d", &x, &y);
        SetMousePosition(x, y);
        command_server_respond(true, "OK");
        snprintf(last_cmd_text, sizeof(last_cmd_text), "MOVE_MOUSE %d %d", x,
                 y);
        break;
      }

      case CMD_NONE:
        break;
    }

    // Drawing
    BeginDrawing();
    ClearBackground(BLACK);

    // Player circle
    DrawCircleV(player_pos, PLAYER_RADIUS, GREEN);

    // Mouse crosshair
    Vector2 mouse = GetMousePosition();
    DrawLine((int)mouse.x - 10, (int)mouse.y, (int)mouse.x + 10, (int)mouse.y,
             WHITE);
    DrawLine((int)mouse.x, (int)mouse.y - 10, (int)mouse.x, (int)mouse.y + 10,
             WHITE);

    // Click marker
    if (show_click) {
      DrawCircleV(click_pos, 5, RED);
      DrawCircleLines((int)click_pos.x, (int)click_pos.y, 10, RED);
    }

    // HUD text
    DrawFPS(5, 5);
    DrawText(TextFormat("Player: %.0f, %.0f", player_pos.x, player_pos.y), 5,
             25, 16, RAYWHITE);
    DrawText(TextFormat("Mouse: %.0f, %.0f", mouse.x, mouse.y), 5, 45, 16,
             RAYWHITE);
    DrawText(TextFormat("Last cmd: %s", last_cmd_text), 5, 65, 16, YELLOW);

    EndDrawing();
  }

  command_server_cleanup();
  CloseWindow();
  return 0;
}
