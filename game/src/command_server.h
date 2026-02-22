#ifndef COMMAND_SERVER_H
#define COMMAND_SERVER_H

#include <stdbool.h>

typedef enum {
  CMD_NONE,
  CMD_SCREENSHOT,
  CMD_KEY_PRESS,
  CMD_MOUSE_PRESS,
  CMD_MOVE_MOUSE,
  CMD_QUIT
} CommandType;

typedef struct {
    CommandType type;
    union {
        char filename[512];       // CMD_SCREENSHOT
        int key_code;             // CMD_KEY_PRESS
        int mouse_button;         // CMD_MOUSE_PRESS
        struct { int x, y; } pos; // CMD_MOVE_MOUSE
    };
} Command;

int command_server_init(int port);
Command command_server_poll(void);
void command_server_respond(bool success, const char *message);
void command_server_cleanup(void);

#endif
