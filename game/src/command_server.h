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
  CMD_QUIT,
  CMD_SCRIPT,
  CMD_WAIT,
  CMD_WAIT_STATE,
  CMD_ASSERT_STATE,
  CMD_ASSERT_EQ,
  CMD_ASSERT_GE,
  CMD_ASSERT_LE,
  CMD_GET,
  CMD_LOG
} CommandType;

typedef struct {
  CommandType type;
  union {
    char filename[512];  // CMD_SCREENSHOT, CMD_SCRIPT
    int key_code;        // CMD_KEY_PRESS
    int mouse_button;    // CMD_MOUSE_PRESS
    struct {
      int x, y;
    } pos;            // CMD_MOVE_MOUSE
    int wait_frames;  // CMD_WAIT
    struct {          // CMD_WAIT_STATE
      int target_state;
      int timeout;
    } wait_state;
    int assert_state;  // CMD_ASSERT_STATE
    struct {           // CMD_ASSERT_EQ/GE/LE, CMD_GET
      char field[64];
      int value;
    } field_check;
    char log_text[256];  // CMD_LOG
  };
} Command;

#define SCRIPT_MAX_LINES 1024
#define SCRIPT_MAX_LINE_LEN 256
#define SCRIPT_REPORT_MAX 512

typedef struct {
  char lines[SCRIPT_MAX_LINES][SCRIPT_MAX_LINE_LEN];
  int line_count;
  int current_line;
  int wait_remaining;
  int wait_state_target;  // -1 = not waiting
  int wait_state_timeout;
  bool active;
  int pass_count;
  int fail_count;
  char report[SCRIPT_REPORT_MAX][SCRIPT_MAX_LINE_LEN];
  int report_count;
  int client_fd;
} ScriptRunner;

int command_server_init(int port);
Command command_server_poll(void);
void command_server_respond(bool success, const char* message);
void command_server_cleanup(void);

// Script runner API
bool script_runner_load(ScriptRunner* sr, const char* filepath, int client_fd);
Command script_runner_tick(ScriptRunner* sr);
void script_runner_finish(ScriptRunner* sr);
void script_runner_report(ScriptRunner* sr, const char* prefix, const char* msg);
void script_runner_respond(ScriptRunner* sr, const char* fmt, ...);

// Global script runner instance
extern ScriptRunner script_runner;

#endif /* COMMAND_SERVER_H */

#ifdef COMMAND_SERVER_IMPLEMENTATION

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
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

ScriptRunner script_runner = {0};

static int script__parse_state_name(const char* name) {
  if (strcmp(name, "LOGO") == 0) return 0;
  if (strcmp(name, "MENU") == 0) return 1;
  if (strcmp(name, "TUTOR1") == 0) return 2;
  if (strcmp(name, "TUTOR2") == 0) return 3;
  if (strcmp(name, "TUTOR3") == 0) return 4;
  if (strcmp(name, "NIGHT") == 0) return 5;
  if (strcmp(name, "DAY") == 0) return 6;
  if (strcmp(name, "WIN") == 0) return 7;
  if (strcmp(name, "OVER") == 0) return 8;
  return -1;
}

static Command script__parse_line(const char* line) {
  Command cmd = {.type = CMD_NONE};

  // Skip empty lines and comments
  if (line[0] == '\0' || line[0] == '#') return cmd;

  if (strncmp(line, "KEY ", 4) == 0) {
    cmd.type = CMD_KEY_PRESS;
    cmd.key_code = atoi(line + 4);
  } else if (strncmp(line, "MOUSE ", 6) == 0) {
    cmd.type = CMD_MOUSE_PRESS;
    cmd.mouse_button = atoi(line + 6);
  } else if (strncmp(line, "MOVE ", 5) == 0) {
    cmd.type = CMD_MOVE_MOUSE;
    sscanf(line + 5, "%d %d", &cmd.pos.x, &cmd.pos.y);
  } else if (strncmp(line, "SHOT ", 5) == 0) {
    cmd.type = CMD_SCREENSHOT;
    const char* data = line + 5;
    size_t len = strlen(data);
    if (len > 0 && len < sizeof(cmd.filename)) {
      memcpy(cmd.filename, data, len + 1);
    }
  } else if (strncmp(line, "WAIT_STATE ", 11) == 0) {
    cmd.type = CMD_WAIT_STATE;
    char state_name[64];
    int timeout = 600;
    sscanf(line + 11, "%63s %d", state_name, &timeout);
    cmd.wait_state.target_state = script__parse_state_name(state_name);
    cmd.wait_state.timeout = timeout;
  } else if (strncmp(line, "WAIT ", 5) == 0) {
    cmd.type = CMD_WAIT;
    cmd.wait_frames = atoi(line + 5);
  } else if (strncmp(line, "ASSERT_STATE ", 13) == 0) {
    cmd.type = CMD_ASSERT_STATE;
    cmd.assert_state = script__parse_state_name(line + 13);
  } else if (strncmp(line, "ASSERT_GE ", 10) == 0) {
    cmd.type = CMD_ASSERT_GE;
    sscanf(line + 10, "%63s %d", cmd.field_check.field, &cmd.field_check.value);
  } else if (strncmp(line, "ASSERT_LE ", 10) == 0) {
    cmd.type = CMD_ASSERT_LE;
    sscanf(line + 10, "%63s %d", cmd.field_check.field, &cmd.field_check.value);
  } else if (strncmp(line, "ASSERT_EQ ", 10) == 0) {
    cmd.type = CMD_ASSERT_EQ;
    sscanf(line + 10, "%63s %d", cmd.field_check.field, &cmd.field_check.value);
  } else if (strncmp(line, "GET ", 4) == 0) {
    cmd.type = CMD_GET;
    snprintf(cmd.field_check.field, sizeof(cmd.field_check.field), "%s", line + 4);
  } else if (strncmp(line, "LOG ", 4) == 0) {
    cmd.type = CMD_LOG;
    snprintf(cmd.log_text, sizeof(cmd.log_text), "%s", line + 4);
  } else if (strcmp(line, "QUIT") == 0) {
    cmd.type = CMD_QUIT;
  }

  return cmd;
}

bool script_runner_load(ScriptRunner* sr, const char* filepath, int client_fd) {
  FILE* f = fopen(filepath, "r");
  if (!f) {
    printf("script_runner: failed to open %s\n", filepath);
    return false;
  }

  memset(sr, 0, sizeof(*sr));
  sr->client_fd = client_fd;
  sr->wait_state_target = -1;

  char buf[SCRIPT_MAX_LINE_LEN];
  while (fgets(buf, sizeof(buf), f) && sr->line_count < SCRIPT_MAX_LINES) {
    // Strip trailing newline/whitespace
    int len = (int) strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' || buf[len - 1] == ' ')) buf[--len] = '\0';
    memcpy(sr->lines[sr->line_count], buf, (size_t) (len + 1));
    sr->line_count++;
  }
  fclose(f);

  sr->active = true;
  printf("script_runner: loaded %s (%d lines)\n", filepath, sr->line_count);
  return true;
}

void script_runner_report(ScriptRunner* sr, const char* prefix, const char* msg) {
  if (sr->report_count < SCRIPT_REPORT_MAX) {
    snprintf(sr->report[sr->report_count], SCRIPT_MAX_LINE_LEN, "[%s] line %d: %s", prefix, sr->current_line, msg);
    sr->report_count++;
  }
}

static void script__send(ScriptRunner* sr, const char* msg) {
  if (sr->client_fd >= 0) {
    send(sr->client_fd, msg, strlen(msg), 0);
  }
}

void script_runner_respond(ScriptRunner* sr, const char* fmt, ...) {
  if (sr->client_fd < 0) {
    return;
  }
  char buf[512];
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(buf, sizeof(buf) - 1, fmt, args);
  va_end(args);
  buf[len] = '\n';
  send(sr->client_fd, buf, (size_t) len + 1, 0);
}

void script_runner_finish(ScriptRunner* sr) {
  char buf[1024];
  script__send(sr, "=== SCRIPT REPORT ===\n");
  snprintf(buf, sizeof(buf), "PASS: %d\nFAIL: %d\nTOTAL: %d\n---\n", sr->pass_count, sr->fail_count,
           sr->pass_count + sr->fail_count);
  script__send(sr, buf);
  for (int i = 0; i < sr->report_count; i++) {
    script__send(sr, sr->report[i]);
    script__send(sr, "\n");
  }
  script__send(sr, "=== END REPORT ===\n");

  if (sr->client_fd >= 0) {
    close(sr->client_fd);
    sr->client_fd = -1;
  }
  sr->active = false;
  printf("script_runner: finished (pass=%d fail=%d)\n", sr->pass_count, sr->fail_count);
}

Command script_runner_tick(ScriptRunner* sr) {
  Command cmd = {.type = CMD_NONE};

  if (!sr->active) return cmd;

  // Waiting for frames
  if (sr->wait_remaining > 0) {
    sr->wait_remaining--;
    return cmd;
  }

  // Waiting for state (checked externally in main.c)
  if (sr->wait_state_target >= 0) {
    sr->wait_state_timeout--;
    if (sr->wait_state_timeout <= 0) {
      sr->fail_count++;
      char msg[128];
      snprintf(msg, sizeof(msg), "WAIT_STATE timeout (target=%d)", sr->wait_state_target);
      script_runner_report(sr, "FAIL", msg);
      sr->wait_state_target = -1;
    }
    return cmd;
  }

  // All lines executed
  if (sr->current_line >= sr->line_count) {
    script_runner_finish(sr);
    return cmd;
  }

  // Parse next line
  cmd = script__parse_line(sr->lines[sr->current_line++]);

  // Handle script-internal commands that don't go to main.c
  if (cmd.type == CMD_WAIT) {
    sr->wait_remaining = cmd.wait_frames;
    cmd.type = CMD_NONE;
  } else if (cmd.type == CMD_WAIT_STATE) {
    sr->wait_state_target = cmd.wait_state.target_state;
    sr->wait_state_timeout = cmd.wait_state.timeout;
    cmd.type = CMD_NONE;
  } else if (cmd.type == CMD_LOG) {
    printf("script: %s\n", cmd.log_text);
    script_runner_report(sr, "LOG", cmd.log_text);
    cmd.type = CMD_NONE;
  } else if (cmd.type == CMD_QUIT) {
    // Finish report before quitting
    script_runner_finish(sr);
    return cmd;
  } else if (cmd.type == CMD_NONE) {
    // Empty line or comment — advance immediately on same frame
    if (sr->current_line < sr->line_count) {
      return script_runner_tick(sr);
    }
  }

  return cmd;
}

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

  if (bind(command_server__fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
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

static Command command_server__parse_line(const char* line) {
  Command cmd = {.type = CMD_NONE};

  if (strncmp(line, "SCREENSHOT ", 11) == 0) {
    cmd.type = CMD_SCREENSHOT;
    const char* data = line + 11;
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
  } else if (strncmp(line, "SCRIPT ", 7) == 0) {
    cmd.type = CMD_SCRIPT;
    const char* data = line + 7;
    size_t len = strlen(data);
    if (len > 0 && len < sizeof(cmd.filename)) {
      memcpy(cmd.filename, data, len + 1);
    }
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

  // If script is active, execute from script runner
  if (script_runner.active) {
    return script_runner_tick(&script_runner);
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

  ssize_t n = recv(command_server__client_fd, command_server__recv_buf + command_server__recv_len,
                   COMMAND_SERVER__RECV_BUF_SIZE - command_server__recv_len - 1, 0);
  if (n > 0) {
    command_server__recv_len += (int) n;
    command_server__recv_buf[command_server__recv_len] = '\0';

    char* newline = strchr(command_server__recv_buf, '\n');
    if (newline != NULL) {
      *newline = '\0';
      cmd = command_server__parse_line(command_server__recv_buf);

      int remaining = command_server__recv_len - (int) (newline - command_server__recv_buf) - 1;
      if (remaining > 0) {
        memmove(command_server__recv_buf, newline + 1, (size_t) remaining);
      }
      command_server__recv_len = remaining;

      // Handle SCRIPT command: load and start script, keep connection open
      if (cmd.type == CMD_SCRIPT) {
        if (script_runner_load(&script_runner, cmd.filename, command_server__client_fd)) {
          // Transfer ownership of client_fd to script_runner
          command_server__client_fd = -1;
          command_server__recv_len = 0;
        } else {
          command_server_respond(false, "Failed to load script");
        }
        cmd.type = CMD_NONE;
      }
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

void command_server_respond(bool success, const char* message) {
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
