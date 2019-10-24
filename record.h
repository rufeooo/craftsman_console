
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef void (*RecordEvent_t)(size_t strlen, char *str);
typedef struct Record_s Record_t;

Record_t *record_alloc();
bool record_append(Record_t *rec, size_t len, const char *input);
void record_playback_all(Record_t *rec, RecordEvent_t handler);
bool record_playback(Record_t *rec, RecordEvent_t handler, uint32_t *readOffset);
bool record_can_playback(Record_t *rec, uint32_t readOffset);
void record_seek_write(Record_t *rec, size_t nth);
Record_t *record_clone(Record_t *rec);
void record_reset(Record_t *rec);
void record_free(Record_t *rec);

size_t record_length(Record_t *rec);
size_t record_write_offset(Record_t *rec);

void record_debug(Record_t *rec);
