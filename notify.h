
#pragma once

#include <stdbool.h>
#include <sys/inotify.h>

typedef void (*NotifyEvent_t)(int idx, const struct inotify_event *);

int notify_last_error();
bool notify_init(uint32_t eventMask, uint32_t argc, char **argv);
bool notify_poll(NotifyEvent_t handler);
void notify_shutdown();
