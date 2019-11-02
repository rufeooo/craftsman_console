
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct Record_s Record_t;
typedef struct {
  uint32_t byte_count;
  uint32_t command_count;
} RecordOffset_t;
typedef struct {
  Record_t* rec;
  RecordOffset_t read;
  RecordOffset_t write;
} RecordRW_t;

#ifndef LOCAL
#define LOCAL __attribute__((visibility("hidden")))
#endif

typedef void (*RecordEvent_t)(size_t strlen, char *str);

LOCAL Record_t *record_alloc();
LOCAL bool record_append(Record_t *rec, size_t len, const char *input,
                         RecordOffset_t *off);
LOCAL bool record_can_playback(const Record_t *rec,
                               const RecordOffset_t *off);
LOCAL bool record_playback(const Record_t *rec, RecordEvent_t handler,
                           RecordOffset_t *read_off);
LOCAL const char *record_peek(const Record_t *rec, const RecordOffset_t *off);
LOCAL const char *record_read(const Record_t *rec, RecordOffset_t *off,
                              size_t *len);
void record_playback_all(const Record_t *rec, RecordEvent_t handler);
LOCAL Record_t *record_clone(const Record_t *rec);
LOCAL void record_reset(Record_t *rec);
LOCAL void record_free(Record_t *rec);

LOCAL size_t record_length(Record_t *rec);
LOCAL int record_compare(const Record_t *lhs, const Record_t *rhs);

LOCAL void record_debug(Record_t *rec);

