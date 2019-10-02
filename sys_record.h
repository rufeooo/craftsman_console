
#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef void (*sys_recordEvent)(size_t strlen, char *str);
typedef struct Record_s Record_t;

Record_t *sys_recordInit();
bool sys_recordAppend(Record_t *rec, size_t len, const char *input);
void sys_recordPlayback(Record_t *rec, sys_recordEvent handler);
void sys_recordShutdown(Record_t *rec);
