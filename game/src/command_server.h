/*
 * command_server.h — single-header TCP command server for raylib games
 *
 * Usage:
 *   In exactly ONE .c file, before including this header, define:
 *     #define COMMAND_SERVER_IMPLEMENTATION
 *     #include "command_server.h"
 *
 *   In all other files just include normally:
 *     #include "command_server.h"
 */

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

#endif /* COMMAND_SERVER_H */

#ifdef COMMAND_SERVER_IMPLEMENTATION

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define COMMAND_SERVER__RECV_BUF_SIZE 1024

static int command_server__fd = -1;
static int command_server__client_fd = -1;
static char command_server__recv_buf[COMMAND_SERVER__RECV_BUF_SIZE];
static int command_server__recv_len = 0;

int command_server_init(int port) {
  command_server__fd = socket(AF_INET, SOCK_STREAM, 0);
  if (command_server__fd < 0) {
    perror("command_server: socket");
    return -1;
  }

  int opt = 1;
  setsockopt(command_server__fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
      .sin_port = htons((uint16_t) port),
  };

  if (bind(command_server__fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    perror("command_server: bind");
    close(command_server__fd);
    command_server__fd = -1;
    return -1;
  }

  if (listen(command_server__fd, 1) < 0) {
    perror("command_server: listen");
    close(command_server__fd);
    command_server__fd = -1;
    return -1;
  }

  int flags = fcntl(command_server__fd, F_GETFL, 0);
  fcntl(command_server__fd, F_SETFL, flags | O_NONBLOCK);

  printf("command_server: listening on port %d\n", port);
  return 0;
}

static Command command_server__parse_line(const char *line) {
  Command cmd = {.type = CMD_NONE};

  if (strncmp(line, "SCREENSHOT ", 11) == 0) {
    cmd.type = CMD_SCREENSHOT;
    const char *data = line + 11;
    size_t len = strlen(data);
    if (len > 0 && len < sizeof(cmd.filename)) {
      memcpy(cmd.filename, data, len + 1);
    }
  } else if (strncmp(line, "KEY_PRESS ", 10) == 0) {
    cmd.type = CMD_KEY_PRESS;
    cmd.key_code = atoi(line + 10);
  } else if (strncmp(line, "MOUSE_PRESS ", 12) == 0) {
    cmd.type = CMD_MOUSE_PRESS;
    cmd.mouse_button = atoi(line + 12);
  } else if (strncmp(line, "MOVE_MOUSE ", 11) == 0) {
    cmd.type = CMD_MOVE_MOUSE;
    sscanf(line + 11, "%d %d", &cmd.pos.x, &cmd.pos.y);
  } else if (strcmp(line, "QUIT") == 0) {
    cmd.type = CMD_QUIT;
  }

  return cmd;
}

Command command_server_poll(void) {
  Command cmd = {.type = CMD_NONE};

  if (command_server__fd < 0) {
    return cmd;
  }

  if (command_server__client_fd < 0) {
    command_server__client_fd = accept(command_server__fd, NULL, NULL);
    if (command_server__client_fd < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("command_server: accept");
      }
      return cmd;
    }
    int flags = fcntl(command_server__client_fd, F_GETFL, 0);
    fcntl(command_server__client_fd, F_SETFL, flags | O_NONBLOCK);
    command_server__recv_len = 0;
  }

  ssize_t n = recv(command_server__client_fd,
                   command_server__recv_buf + command_server__recv_len,
                   COMMAND_SERVER__RECV_BUF_SIZE - command_server__recv_len - 1,
                   0);
  if (n > 0) {
    command_server__recv_len += (int) n;
    command_server__recv_buf[command_server__recv_len] = '\0';

    char *newline = strchr(command_server__recv_buf, '\n');
    if (newline != NULL) {
      *newline = '\0';
      cmd = command_server__parse_line(command_server__recv_buf);

      int remaining = command_server__recv_len -
                      (int) (newline - command_server__recv_buf) - 1;
      if (remaining > 0) {
        memmove(command_server__recv_buf, newline + 1, (size_t) remaining);
      }
      command_server__recv_len = remaining;
    }
  } else if (n == 0) {
    close(command_server__client_fd);
    command_server__client_fd = -1;
    command_server__recv_len = 0;
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    perror("command_server: recv");
    close(command_server__client_fd);
    command_server__client_fd = -1;
    command_server__recv_len = 0;
  }

  return cmd;
}

void command_server_respond(bool success, const char *message) {
  if (command_server__client_fd < 0) {
    return;
  }

  char buf[1024];
  int len;
  if (success) {
    len = snprintf(buf, sizeof(buf), "OK\n");
  } else {
    len = snprintf(buf, sizeof(buf), "ERROR %s\n", message);
  }

  send(command_server__client_fd, buf, (size_t) len, 0);
  close(command_server__client_fd);
  command_server__client_fd = -1;
  command_server__recv_len = 0;
}

void command_server_cleanup(void) {
  if (command_server__client_fd >= 0) {
    close(command_server__client_fd);
    command_server__client_fd = -1;
  }
  if (command_server__fd >= 0) {
    close(command_server__fd);
    command_server__fd = -1;
  }
  command_server__recv_len = 0;
}

#endif /* COMMAND_SERVER_IMPLEMENTATION */
