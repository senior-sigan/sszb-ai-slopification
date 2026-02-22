#include <raylib.h>
#include <stdio.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Game");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawFPS(5, 5);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
