#include <raylib.h>
#include <stdio.h>
#include <sys/syslog.h>

#include "command_server.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define CMD_PORT 9999

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Game");
  SetTargetFPS(60);
  command_server_init(CMD_PORT);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawFPS(5, 5);
    EndDrawing();

    Command cmd = command_server_poll();
    if (cmd.type == CMD_SCREENSHOT) {
      TakeScreenshot(cmd.data);
      command_server_respond(true, "OK");
    }
  }

  command_server_cleanup();
  CloseWindow();
  return 0;
}
