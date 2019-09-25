
#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef void (*sys_recordEvent)(size_t strlen, char *str);

bool sys_recordInit();
bool sys_recordAppend(size_t len, const char *input);
void sys_recordPlayback(sys_recordEvent handler);
void sys_recordShutdown();
