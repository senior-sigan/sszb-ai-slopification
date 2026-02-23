---
name: raycmd
description: "TCP command protocol for remote control of raylib games"
---

# /raycmd - Raylib TCP Command Protocol

Reference for the TCP command server that allows remote control of raylib games.

## Architecture

The game runs a non-blocking TCP server on `localhost:9999`. Two modes of operation:

1. **Single-command mode**: connect, send one line, receive `OK\n` or `ERROR ...\n`, connection closed.
2. **Script mode**: send `SCRIPT <file>\n`, connection stays open, script executes line-per-frame, report sent on completion.

### Files

- `src/command_server.h` — STB-style header-only: `Command` struct, `CommandType` enum, `ScriptRunner`, server API
- `src/main.c` — game loop integration, ASSERT/GET/WAIT_STATE handling, `game_get_field_int()`

## TCP Protocol

Port: **9999** (defined as `CMD_PORT` in `main.c`)

### Single-Command Mode

Send a single line (terminated by `\n`), read the response, close the connection.

| Command | Format | Description |
|---------|--------|-------------|
| SCREENSHOT | `SCREENSHOT <filename>\n` | Save screenshot to file |
| KEY_PRESS | `KEY_PRESS <raylib_key_code>\n` | Simulate key press |
| MOUSE_PRESS | `MOUSE_PRESS <raylib_button>\n` | Simulate mouse button press |
| MOVE_MOUSE | `MOVE_MOUSE <x> <y>\n` | Move mouse cursor to position |
| SCRIPT | `SCRIPT <filepath>\n` | Load and execute a `.sszb` test script |
| QUIT | `QUIT\n` | Graceful shutdown |

### Key codes (raylib)

Common key codes for `KEY_PRESS`:

- `257` — KEY_ENTER
- `262` — KEY_RIGHT
- `263` — KEY_LEFT
- `264` — KEY_DOWN
- `265` — KEY_UP
- `32` — KEY_SPACE
- `49` — KEY_ONE
- `50` — KEY_TWO
- `51` — KEY_THREE
- `52` — KEY_FOUR
- `76` — KEY_L (cheat: skip night)
- `80` — KEY_P (cheat: add hit)

Full list: [raylib keyboardkeys](https://www.raylib.com/cheatsheet/cheatsheet.html)

### Mouse buttons (raylib)

- `0` — MOUSE_BUTTON_LEFT
- `1` — MOUSE_BUTTON_RIGHT
- `2` — MOUSE_BUTTON_MIDDLE

## Script Engine (.sszb files)

Scripts are loaded via `SCRIPT <filepath>` TCP command. The connection stays open. Each line is executed once per game frame. On completion, a test report is sent back via the same TCP connection.

### Script Commands

| Command | Format | Description |
|---------|--------|-------------|
| KEY | `KEY <code>` | Simulate key press (same codes as KEY_PRESS) |
| MOUSE | `MOUSE <button>` | Simulate mouse button press |
| MOVE | `MOVE <x> <y>` | Move mouse cursor |
| SHOT | `SHOT <filename>` | Take screenshot |
| WAIT | `WAIT <frames>` | Pause for N frames (60 frames ~ 1 sec at 60 FPS) |
| WAIT_STATE | `WAIT_STATE <state> [timeout]` | Wait until game reaches state (timeout in frames, default 600) |
| ASSERT_STATE | `ASSERT_STATE <state>` | Assert current game state |
| ASSERT_EQ | `ASSERT_EQ <field> <value>` | Assert field == value |
| ASSERT_GE | `ASSERT_GE <field> <value>` | Assert field >= value |
| ASSERT_LE | `ASSERT_LE <field> <value>` | Assert field <= value |
| GET | `GET <field>` | Send field value to client (e.g. `money=10`) |
| LOG | `LOG <text>` | Print message to stdout and include in report |
| QUIT | `QUIT` | Send report, then shut down game |
| # | `# comment` | Comment (ignored) |

### State Names

`LOGO`, `MENU`, `TUTOR1`, `TUTOR2`, `TUTOR3`, `NIGHT`, `DAY`, `WIN`, `OVER`

### Queryable Fields

| Field | Type | Description |
|-------|------|-------------|
| `state` | int | Current game state (enum value) |
| `money` | int | Current money |
| `hits` | int | Current hits taken |
| `level` | int | Current level number |
| `cur_row` | int | Selected row in building |
| `cur_col` | int | Selected column in building |
| `creatures` | int | Number of active creatures |
| `club_bought` | bool (0/1) | Whether club has been purchased |

### Execution Model

- One script line per game frame (comments/blanks skip instantly)
- `WAIT N` decrements a counter each frame, blocking line advancement
- `WAIT_STATE` checks game state each frame until match or timeout
- `ASSERT_*` and `GET` are forwarded to `main.c` for evaluation against live game state
- `QUIT` sends the report first, then returns `CMD_QUIT` to shut down the game

### Report Format

Sent via TCP on script completion:

```
=== SCRIPT REPORT ===
PASS: 13
FAIL: 0
TOTAL: 13
---
[LOG] line 5: Phase 1: Logo to Menu
[PASS] line 7: ASSERT_STATE MENU
[SHOT] line 8: 01_menu.png
...
=== END REPORT ===
```

### Example Script

```bash
# test_full_cycle.sszb
LOG Phase 1: Logo to Menu
WAIT 90
ASSERT_STATE MENU

LOG Phase 2: Menu to Night
KEY 257
WAIT 2
ASSERT_STATE TUTOR1
KEY 257
WAIT 2
ASSERT_STATE TUTOR2
KEY 257
WAIT 2
ASSERT_STATE TUTOR3
KEY 257
WAIT 2
ASSERT_STATE NIGHT

LOG Phase 3: Night gameplay
ASSERT_EQ level 1
ASSERT_EQ money 10
KEY 32
WAIT 60

# Skip to day via cheat
KEY 76
WAIT 5
ASSERT_STATE DAY

QUIT
```

## Sending Commands from Shell

```bash
# Single command via nc
echo "SCREENSHOT frame.png" | nc -w 2 localhost 9999

# Simulate key press
echo "KEY_PRESS 262" | nc -w 2 localhost 9999

# Run a test script (connection stays open for report)
(echo "SCRIPT tests/test_full_cycle.sszb"; sleep 120) | nc localhost 9999

# Shutdown game
echo "QUIT" | nc -w 2 localhost 9999
```

Important: in single-command mode, add `sleep 0.3` between commands — each command is processed in one game frame.

## Running Tests

```bash
make test                                    # build + run full test
./tests/run_test.sh                          # run default test
./tests/run_test.sh tests/my_test.sszb       # run specific test
```

The test runner (`tests/run_test.sh`) launches the game, waits for TCP, sends `SCRIPT`, captures the report, and checks for `FAIL: 0`.

## Command Struct

```c
typedef struct {
    CommandType type;
    union {
        char filename[512];       // CMD_SCREENSHOT, CMD_SCRIPT
        int key_code;             // CMD_KEY_PRESS
        int mouse_button;         // CMD_MOUSE_PRESS
        struct { int x, y; } pos; // CMD_MOVE_MOUSE
        int wait_frames;          // CMD_WAIT
        struct {                   // CMD_WAIT_STATE
            int target_state;
            int timeout;
        } wait_state;
        int assert_state;         // CMD_ASSERT_STATE
        struct {                   // CMD_ASSERT_EQ/GE/LE, CMD_GET
            char field[64];
            int value;
        } field_check;
        char log_text[256];       // CMD_LOG
    };
} Command;
```

## ScriptRunner Struct

```c
typedef struct {
    char lines[SCRIPT_MAX_LINES][SCRIPT_MAX_LINE_LEN];  // parsed script lines
    int line_count;          // total lines
    int current_line;        // execution pointer
    int wait_remaining;      // frames left for WAIT
    int wait_state_target;   // target state for WAIT_STATE (-1 = not waiting)
    int wait_state_timeout;  // frames remaining before WAIT_STATE fails
    bool active;             // script is running
    int pass_count;          // passed assertions
    int fail_count;          // failed assertions
    char report[SCRIPT_REPORT_MAX][SCRIPT_MAX_LINE_LEN];  // report lines
    int report_count;
    int client_fd;           // TCP fd for sending report
} ScriptRunner;
```

Global instance: `extern ScriptRunner script_runner;`

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
    // 1. Poll TCP command (returns script commands when script is active)
    frame_cmd = command_server_poll();

    // 2. Handle commands
    switch (frame_cmd.type) {
        case CMD_SCREENSHOT:
            TakeScreenshot(frame_cmd.filename);
            if (!script_runner.active) command_server_respond(true, "OK");
            break;
        case CMD_KEY_PRESS:
        case CMD_MOUSE_PRESS:
            if (!script_runner.active) command_server_respond(true, "OK");
            break;
        case CMD_ASSERT_STATE:
            // Check game.state == frame_cmd.assert_state
            // Update script_runner.pass_count or fail_count
            break;
        case CMD_ASSERT_EQ: case CMD_ASSERT_GE: case CMD_ASSERT_LE:
            // Query game_get_field_int(), compare, update counters
            break;
        case CMD_GET:
            // Send field value via script_respond()
            break;
        case CMD_QUIT:
            running = false;
            break;
        case CMD_NONE:
            break;
    }

    // 3. Game logic
    state_update(dt);

    // 4. Check WAIT_STATE completion after state update
    if (script_runner.active && script_runner.wait_state_target >= 0) {
        if ((int)game.state == script_runner.wait_state_target) {
            script_runner.wait_state_target = -1;
        }
    }

    // 5. Drawing
    BeginDrawing();
    // ...
    EndDrawing();
}
```

Key principle: `command_server_poll()` transparently returns commands from either TCP or from the active script. Game code uses `ManagedIsKeyPressed` / `ManagedIsMouseButtonPressed` and is unaware of the input source.

## Build and Test

```bash
make build          # compile
make run            # run game
make test           # build + run test script

# Manual testing:
echo "SCREENSHOT test.png" | nc -w 2 localhost 9999
echo "QUIT" | nc -w 2 localhost 9999
```
