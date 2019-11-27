#pragma once

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

// extern
extern void *calloc(size_t __nmemb,
                    size_t __size) __THROW __attribute_malloc__ __wur;

// Types
typedef void (*NotifyEvent_t)(int idx, const struct inotify_event *);

typedef struct {
  int nfds;
  struct pollfd *fds;
  int *wd;
} Watch_t;

static Watch_t watch_state;
static int watch_error;

int
notify_last_error()
{
  return watch_error;
}

bool
notify_init(uint32_t eventMask, uint32_t argc, const char **argv)
{
  watch_state.fds = calloc(argc, sizeof(struct pollfd));
  if (!watch_state.fds) {
    watch_error = 2;
    return false;
  }

  watch_state.wd = calloc(argc, sizeof(int));
  if (!watch_state.wd) {
    watch_error = 3;
    return false;
  }

  watch_state.nfds = 0;
  for (uint32_t i = 0; i < argc; ++i) {
    ++watch_state.nfds;
    struct pollfd pfd = {
      .fd = inotify_init1(IN_NONBLOCK),
      .events = POLLIN,
    };
    watch_state.fds[i] = pfd;

    if (pfd.fd == -1) {
      watch_error = 4;
      perror("inotify_init1");
      return false;
    }

    watch_state.wd[i] = inotify_add_watch(pfd.fd, argv[i], eventMask);
    if (watch_state.wd[i] == -1) {
      watch_error = 5;
      perror("inotify_add_watch");
      return false;
    }
  }

  watch_error = 0;

  return true;
}

bool
notify_poll(NotifyEvent_t handler)
{
  static char buf[4096]
    __attribute__((aligned(__alignof__(struct inotify_event))));
  const char *event;
  const char *eventEnd;

  int poll_num = poll(watch_state.fds, watch_state.nfds, 0);
  // printf("poll %d\n", poll_num);

  if (poll_num == -1) {
    if (errno == EINTR)
      return true;
    watch_error = 6;
    perror("poll");
    return false;
  }

  for (int i = 0; i < watch_state.nfds; ++i) {
    if (0 == (watch_state.fds[i].revents & POLLIN)) {
      continue;
    }

    watch_state.fds[i].revents = 0;

    ssize_t len = read(watch_state.fds[i].fd, buf, sizeof buf);
    if (len == -1 && errno != EAGAIN) {
      perror("read");
      watch_error = 7;
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

  watch_error = 0;

  return true;
}

void
notify_shutdown()
{
  for (int i = 0; i < watch_state.nfds; ++i) {
    close(watch_state.fds[i].fd);
  }
  free(watch_state.fds);
  free(watch_state.wd);
  memset(&watch_state, 0, sizeof(watch_state));
}

