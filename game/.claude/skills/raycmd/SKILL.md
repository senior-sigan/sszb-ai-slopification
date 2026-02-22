---
name: raycmd
description: "TCP command protocol for remote control of raylib games"
---

# /raycmd - Raylib TCP Command Protocol

Reference for the TCP command server that allows remote control of raylib games.

## Architecture

The game runs a non-blocking TCP server on `localhost:9999`. Each connection accepts one command per line, receives a response (`OK\n` or `ERROR ...\n`), then the connection is closed.

### Files

- `src/command_server.h` — `Command` struct, `CommandType` enum, server API
- `src/command_server.c` — TCP server, command parsing (`parse_line`)

## TCP Protocol

Port: **9999** (defined as `CMD_PORT` in `main.c`)

Send a single line (terminated by `\n`), read the response, close the connection.

### Commands

| Command | Format | Description |
|---------|--------|-------------|
| SCREENSHOT | `SCREENSHOT <filename>\n` | Save screenshot to file |
| KEY_PRESS | `KEY_PRESS <raylib_key_code>\n` | Simulate key press |
| MOUSE_PRESS | `MOUSE_PRESS <raylib_button>\n` | Simulate mouse button press |
| MOVE_MOUSE | `MOVE_MOUSE <x> <y>\n` | Move mouse cursor to position |
| QUIT | `QUIT\n` | Graceful shutdown |

### Key codes (raylib)

Common key codes for `KEY_PRESS`:

- `262` — KEY_RIGHT
- `263` — KEY_LEFT
- `264` — KEY_DOWN
- `265` — KEY_UP

Full list: [raylib keyboardkeys](https://www.raylib.com/cheatsheet/cheatsheet.html)

### Mouse buttons (raylib)

- `0` — MOUSE_BUTTON_LEFT
- `1` — MOUSE_BUTTON_RIGHT
- `2` — MOUSE_BUTTON_MIDDLE

## Sending Commands from Shell

```bash
# Single command via nc
echo "SCREENSHOT frame.png" | nc -w 2 localhost 9999

# Move mouse, then screenshot
echo "MOVE_MOUSE 320 240" | nc -w 2 localhost 9999
sleep 0.3
echo "SCREENSHOT after_move.png" | nc -w 2 localhost 9999

# Simulate key press
echo "KEY_PRESS 262" | nc -w 2 localhost 9999

# Simulate mouse click
echo "MOUSE_PRESS 0" | nc -w 2 localhost 9999

# Shutdown game
echo "QUIT" | nc -w 2 localhost 9999
```

Important: add `sleep 0.3` between commands — each command is processed in one game frame.

## Command Struct

```c
typedef struct {
    CommandType type;
    union {
        char filename[512];       // CMD_SCREENSHOT
        int key_code;             // CMD_KEY_PRESS
        int mouse_button;         // CMD_MOUSE_PRESS
        struct { int x, y; } pos; // CMD_MOVE_MOUSE
    };
} Command;
```

Access fields directly: `cmd.filename`, `cmd.key_code`, `cmd.mouse_button`, `cmd.pos.x`, `cmd.pos.y`.

## Using in Game Code

### Initialization and cleanup

```c
#include "command_server.h"

int main(void) {
    InitWindow(640, 480, "Game");
    command_server_init(9999);
    // ... game loop ...
    command_server_cleanup();
    CloseWindow();
}
```

### Managed input wrappers

Replace raylib input functions with managed versions that transparently handle both real and TCP input:

```c
static Command frame_cmd;

static bool ManagedIsKeyPressed(int key) {
    return IsKeyPressed(key) ||
           (frame_cmd.type == CMD_KEY_PRESS && frame_cmd.key_code == key);
}

static bool ManagedIsMouseButtonPressed(int button) {
    return IsMouseButtonPressed(button) ||
           (frame_cmd.type == CMD_MOUSE_PRESS && frame_cmd.mouse_button == button);
}
```

### Game loop pattern

```c
while (running && !WindowShouldClose()) {
    // 1. Poll TCP command
    frame_cmd = command_server_poll();

    // 2. Handle transport-level commands (not input)
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

    // 3. Game logic — uses Managed*, agnostic to input source
    if (ManagedIsKeyPressed(KEY_RIGHT)) player_pos.x += SPEED;
    if (ManagedIsKeyPressed(KEY_LEFT))  player_pos.x -= SPEED;
    if (ManagedIsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { /* click */ }

    // 4. Drawing
    BeginDrawing();
    // ...
    EndDrawing();
}
```

Key principle: game logic uses `ManagedIsKeyPressed` / `ManagedIsMouseButtonPressed` instead of raw raylib calls. This makes the game code unaware of TCP control.

## Build and Test

```bash
make build          # compile
make run            # run game

# In another terminal:
echo "SCREENSHOT test.png" | nc -w 2 localhost 9999
echo "QUIT" | nc -w 2 localhost 9999
```
