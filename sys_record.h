
#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef void (*sys_recordEvent)(size_t strlen, char *str);
typedef struct Record_s Record_t;

Record_t *sys_recordAlloc();
bool sys_recordAppend(Record_t *rec, size_t len, const char *input);
void sys_recordPlaybackAll(Record_t *rec, sys_recordEvent handler);
bool sys_recordPlayback(Record_t *rec, sys_recordEvent handler);
void sys_recordSeekW(Record_t *rec, size_t nth);
void sys_recordSeekR(Record_t *rec, size_t nth);
size_t sys_recordLength(Record_t *rec);
Record_t *sys_recordClone(Record_t *rec);
void sys_recordFree(Record_t *rec);

void sys_recordDebug(Record_t *rec);
