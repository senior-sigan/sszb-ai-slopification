#ifndef COMMAND_SERVER_H
#define COMMAND_SERVER_H

#include <stdbool.h>

typedef enum {
  CMD_NONE,
  CMD_SCREENSHOT,
  CMD_KEY_PRESS,
  CMD_MOUSE_PRESS,
  CMD_MOVE_MOUSE
} CommandType;

typedef struct {
    CommandType type;
    char data[512];
} Command;

int command_server_init(int port);
Command command_server_poll(void);
void command_server_respond(bool success, const char *message);
void command_server_cleanup(void);

#endif
