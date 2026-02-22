#include <raylib.h>
#include <stdio.h>

#define COMMAND_SERVER_IMPLEMENTATION
#include "command_server.h"

#define WINDOW_WIDTH 1366
#define WINDOW_HEIGHT 768
#define CMD_PORT 9999
#define PLAYER_SPEED 10
#define PLAYER_RADIUS 20

static Command frame_cmd;

static bool ManagedIsKeyPressed(int key) {
  return IsKeyPressed(key) || (frame_cmd.type == CMD_KEY_PRESS && frame_cmd.key_code == key);
}

static bool ManagedIsMouseButtonPressed(int button) {
  return IsMouseButtonPressed(button) || (frame_cmd.type == CMD_MOUSE_PRESS && frame_cmd.mouse_button == button);
}

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Game");
  SetTargetFPS(60);
  command_server_init(CMD_PORT);

  Vector2 player_pos = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
  Vector2 click_pos = {-1, -1};
  bool show_click = false;
  char last_cmd_text[128] = "None";

  bool running = true;
  while (running && !WindowShouldClose()) {
    // Poll TCP command for this frame
    frame_cmd = command_server_poll();
    switch (frame_cmd.type) {
      case CMD_SCREENSHOT:
        TakeScreenshot(frame_cmd.filename);
        command_server_respond(true, "OK");
        snprintf(last_cmd_text, sizeof(last_cmd_text), "SCREENSHOT %s", frame_cmd.filename);
        break;

      case CMD_MOVE_MOUSE:
        SetMousePosition(frame_cmd.pos.x, frame_cmd.pos.y);
        command_server_respond(true, "OK");
        snprintf(last_cmd_text, sizeof(last_cmd_text), "MOVE_MOUSE %d %d", frame_cmd.pos.x, frame_cmd.pos.y);
        break;

      case CMD_KEY_PRESS:
        command_server_respond(true, "OK");
        snprintf(last_cmd_text, sizeof(last_cmd_text), "KEY_PRESS %d", frame_cmd.key_code);
        break;

      case CMD_MOUSE_PRESS:
        command_server_respond(true, "OK");
        snprintf(last_cmd_text, sizeof(last_cmd_text), "MOUSE_PRESS %d", frame_cmd.mouse_button);
        break;

      case CMD_QUIT:
        command_server_respond(true, "OK");
        running = false;
        break;

      case CMD_NONE:
        break;
    }

    // Game logic — transparent to input source (keyboard/mouse or TCP)
    if (ManagedIsKeyPressed(KEY_RIGHT)) player_pos.x += PLAYER_SPEED;
    if (ManagedIsKeyPressed(KEY_LEFT)) player_pos.x -= PLAYER_SPEED;
    if (ManagedIsKeyPressed(KEY_DOWN)) player_pos.y += PLAYER_SPEED;
    if (ManagedIsKeyPressed(KEY_UP)) player_pos.y -= PLAYER_SPEED;

    if (ManagedIsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      click_pos = GetMousePosition();
      show_click = true;
    }

    // Drawing
    BeginDrawing();
    ClearBackground(BLACK);

    // Player circle
    DrawCircleV(player_pos, PLAYER_RADIUS, GREEN);

    // Mouse crosshair
    Vector2 mouse = GetMousePosition();
    DrawLine((int) mouse.x - 10, (int) mouse.y, (int) mouse.x + 10, (int) mouse.y, WHITE);
    DrawLine((int) mouse.x, (int) mouse.y - 10, (int) mouse.x, (int) mouse.y + 10, WHITE);

    // Click marker
    if (show_click) {
      DrawCircleV(click_pos, 5, RED);
      DrawCircleLines((int) click_pos.x, (int) click_pos.y, 10, RED);
    }

    // HUD text
    DrawFPS(5, 5);
    DrawText(TextFormat("Player: %.0f, %.0f", player_pos.x, player_pos.y), 5, 25, 16, RAYWHITE);
    DrawText(TextFormat("Mouse: %.0f, %.0f", mouse.x, mouse.y), 5, 45, 16, RAYWHITE);
    DrawText(TextFormat("Last cmd: %s", last_cmd_text), 5, 65, 16, YELLOW);

    EndDrawing();
  }

  command_server_cleanup();
  CloseWindow();
  return 0;
}
