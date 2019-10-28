
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint32_t byte_count;
  uint32_t command_count;
} RecordOffset_t;

typedef void (*RecordEvent_t)(size_t strlen, char *str);
typedef struct Record_s Record_t;

Record_t *record_alloc();
bool record_append(Record_t *rec, size_t len, const char *input,
                   RecordOffset_t *write_offset);
bool record_can_playback(const Record_t *rec,
                         const RecordOffset_t *read_offset);
bool record_playback(const Record_t *rec, RecordEvent_t handler,
                     RecordOffset_t *read_offset);
void record_playback_all(const Record_t *rec, RecordEvent_t handler);
Record_t *record_clone(const Record_t *rec);
void record_reset(Record_t *rec);
void record_free(Record_t *rec);

size_t record_length(Record_t *rec);
int record_compare(const Record_t* lhs, const Record_t* rhs);

void record_debug(Record_t *rec);

