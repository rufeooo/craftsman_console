#pragma once

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef void (*InputEvent_t)(size_t strlen, char *str);

static char buf[4096] __attribute__((aligned(4096)));
static const int buf_len = sizeof(buf) - 1;
static const int nfds = 1;
static int used_buf;
static struct pollfd fds[1];

bool
input_init()
{
  buf[buf_len] = 0;

  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  return true;
}

bool
input_poll(InputEvent_t handler)
{
  int poll_num = poll(fds, nfds, 0);

  if (poll_num == -1) {
    if (errno == EINTR)
      return true;

    perror("poll");
    return false;
  }

  for (int i = 0; i < nfds; ++i) {
    if (fds[i].revents & POLLIN) {
      int bytes = read(fds[i].fd, buf + used_buf, buf_len - used_buf);
      used_buf += bytes;
      char *end;
      while ((end = strchr(buf, '\n'))) {
        *end = 0;
        size_t strlen = end - buf;
        handler(strlen, buf);

        // nullterm
        ++strlen;
        memmove(buf, end + 1, sizeof(buf) - strlen);
        used_buf -= strlen;
      }

      if (used_buf == buf_len) {
        perror("dropped buffer, line longer than storage.");
        used_buf = 0;
      }
    }
  }

  return true;
}

bool
input_shutdown()
{
  memset(fds, 0, sizeof(fds));
  used_buf = 0;

  return true;
}
