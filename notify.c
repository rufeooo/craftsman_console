#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "notify.h"

typedef struct {
  int nfds;
  struct pollfd *fds;
  int *wd;
} Watch_t;

static Watch_t watchState;
static int lastError;

int
notify_last_error()
{
  return lastError;
}

bool
notify_init(uint32_t eventMask, uint32_t argc, char **argv)
{
  watchState.fds = calloc(argc, sizeof(struct pollfd));
  if (!watchState.fds) {
    lastError = 2;
    return false;
  }

  watchState.wd = calloc(argc, sizeof(int));
  if (!watchState.wd) {
    lastError = 3;
    return false;
  }

  watchState.nfds = 0;
  for (uint32_t i = 0; i < argc; ++i) {
    ++watchState.nfds;
    struct pollfd pfd = {
      .fd = inotify_init1(IN_NONBLOCK),
      .events = POLLIN,
    };
    watchState.fds[i] = pfd;

    if (pfd.fd == -1) {
      lastError = 4;
      perror("inotify_init1");
      return false;
    }

    watchState.wd[i] =
      inotify_add_watch(pfd.fd, argv[i], eventMask);
    if (watchState.wd[i] == -1) {
      lastError = 5;
      perror("inotify_add_watch");
      return false;
    }
  }

  lastError = 0;

  return true;
}

bool
notify_poll(NotifyEvent_t handler)
{
  static char buf[4096]
    __attribute__((aligned(__alignof__(struct inotify_event))));
  const char *event;
  const char *eventEnd;

  int poll_num = poll(watchState.fds, watchState.nfds, 0);
  // printf("poll %d\n", poll_num);

  if (poll_num == -1) {
    if (errno == EINTR)
      return true;
    lastError = 6;
    perror("poll");
    return false;
  }

  for (int i = 0; i < watchState.nfds; ++i) {
    if (0 == (watchState.fds[i].revents & POLLIN)) {
      continue;
    }

    watchState.fds[i].revents = 0;

    ssize_t len = read(watchState.fds[i].fd, buf, sizeof buf);
    if (len == -1 && errno != EAGAIN) {
      perror("read");
      lastError = 7;
      return false;
    }

    event = buf;
    eventEnd = buf + len;
    while (event < eventEnd) {
      const struct inotify_event *notify =
        (const struct inotify_event *) event;
      handler(i, notify);
      event += sizeof(struct inotify_event) + notify->len;
    }
  }

  lastError = 0;

  return true;
}

void
notify_shutdown()
{
  for (int i = 0; i < watchState.nfds; ++i) {
    close(watchState.fds[i].fd);
  }
  free(watchState.fds);
  free(watchState.wd);
  memset(&watchState, 0, sizeof(watchState));
}

