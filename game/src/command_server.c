#include "command_server.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define RECV_BUF_SIZE 1024

static int server_fd = -1;
static int client_fd = -1;
static char recv_buf[RECV_BUF_SIZE];
static int recv_len = 0;

int command_server_init(int port) {
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("command_server: socket");
    return -1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
      .sin_port = htons((uint16_t) port),
  };

  if (bind(server_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    perror("command_server: bind");
    close(server_fd);
    server_fd = -1;
    return -1;
  }

  if (listen(server_fd, 1) < 0) {
    perror("command_server: listen");
    close(server_fd);
    server_fd = -1;
    return -1;
  }

  int flags = fcntl(server_fd, F_GETFL, 0);
  fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

  printf("command_server: listening on port %d\n", port);
  return 0;
}

static Command parse_line(const char *line) {
  Command cmd = {.type = CMD_NONE, .data = {0}};

  if (strncmp(line, "SCREENSHOT ", 11) == 0) {
    cmd.type = CMD_SCREENSHOT;
    const char *data = line + 11;
    size_t len = strlen(data);
    if (len > 0 && len < sizeof(cmd.data)) {
      memcpy(cmd.data, data, len + 1);
    }
  }

  return cmd;
}

Command command_server_poll(void) {
  Command cmd = {.type = CMD_NONE, .data = {0}};

  if (server_fd < 0) {
    return cmd;
  }

  if (client_fd < 0) {
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("command_server: accept");
      }
      return cmd;
    }
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    recv_len = 0;
  }

  ssize_t n = recv(client_fd, recv_buf + recv_len, RECV_BUF_SIZE - recv_len - 1, 0);
  if (n > 0) {
    recv_len += (int) n;
    recv_buf[recv_len] = '\0';

    char *newline = strchr(recv_buf, '\n');
    if (newline != NULL) {
      *newline = '\0';
      cmd = parse_line(recv_buf);

      int remaining = recv_len - (int) (newline - recv_buf) - 1;
      if (remaining > 0) {
        memmove(recv_buf, newline + 1, (size_t) remaining);
      }
      recv_len = remaining;
    }
  } else if (n == 0) {
    close(client_fd);
    client_fd = -1;
    recv_len = 0;
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    perror("command_server: recv");
    close(client_fd);
    client_fd = -1;
    recv_len = 0;
  }

  return cmd;
}

void command_server_respond(bool success, const char *message) {
  if (client_fd < 0) {
    return;
  }

  char buf[1024];
  int len;
  if (success) {
    len = snprintf(buf, sizeof(buf), "OK\n");
  } else {
    len = snprintf(buf, sizeof(buf), "ERROR %s\n", message);
  }

  send(client_fd, buf, (size_t) len, 0);
  close(client_fd);
  client_fd = -1;
  recv_len = 0;
}

void command_server_cleanup(void) {
  if (client_fd >= 0) {
    close(client_fd);
    client_fd = -1;
  }
  if (server_fd >= 0) {
    close(server_fd);
    server_fd = -1;
  }
  recv_len = 0;
}
