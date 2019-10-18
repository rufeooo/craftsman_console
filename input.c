#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "input.h"

static char buf[4096] __attribute__((aligned(4096)));
static const int bufLen = sizeof(buf) - 1;
static const int nfds = 1;
static int usedBuf;
static struct pollfd fds[1];

bool
input_init()
{
  buf[bufLen] = 0;

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
      int bytes = read(fds[i].fd, buf + usedBuf, bufLen - usedBuf);
      usedBuf += bytes;
      char *end;
      while ((end = strchr(buf, '\n'))) {
        *end = 0;
        size_t strlen = end - buf;
        handler(strlen, buf);

        // nullterm
        ++strlen;
        memmove(buf, end + 1, sizeof(buf) - strlen);
        usedBuf -= strlen;
      }

      if (usedBuf == bufLen) {
        perror("dropped buffer, line longer than storage.");
        usedBuf = 0;
      }
    }
  }

  return true;
}

bool
input_shutdown()
{
  memset(fds, 0, sizeof(fds));
  usedBuf = 0;

  return true;
}
