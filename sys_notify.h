
#pragma once

#include <sys/inotify.h>

typedef void (*sys_notifyEvent)(int idx, const struct inotify_event *);

int sys_notifyLastError();
bool sys_notifyInit(uint32_t eventMask, uint32_t argc, char **argv);
bool sys_notifyPoll(sys_notifyEvent handler);
void sys_notifyShutdown();
