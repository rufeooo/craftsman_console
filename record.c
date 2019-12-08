#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "macro.h"

// extern
extern void *malloc(size_t __size) __THROW __attribute_malloc__ __wur;
extern void *realloc(void *__ptr,
                     size_t __size) __THROW __attribute_warn_unused_result__;
extern void free(void *__ptr) __THROW;

typedef struct Record_s {
  char *buf;
  uint32_t alloc_bytes;
  uint32_t used_bytes;
} Record_t;
typedef struct {
  uint32_t byte_count;
  uint32_t command_count;
} RecordOffset_t;
typedef struct {
  Record_t *rec;
  RecordOffset_t read;
  RecordOffset_t write;
} RecordRW_t;

typedef void (*RecordEvent_t)(size_t strlen, char *str);

Record_t *
record_alloc()
{
  Record_t *rec = malloc(sizeof(Record_t));
  rec->used_bytes = 0;
  rec->alloc_bytes = 4 * 1024;
  rec->buf = malloc(rec->alloc_bytes);

  return rec;
}

static bool
record_realloc(Record_t *rec, int bytesNeeded)
{
  if (bytesNeeded <= 0)
    return true;

  char *const newBuf = realloc(rec->buf, rec->alloc_bytes * 2);
  if (!newBuf)
    return false;

  rec->buf = newBuf;
  rec->alloc_bytes *= 2;

  return true;
}

bool
record_append(const size_t len, const char *input, Record_t *rec,
              RecordOffset_t *off)
{
  const uint32_t byte_offset = off->byte_count;
  const size_t needed = byte_offset + len + 1;
  if (!record_realloc(rec, needed - rec->alloc_bytes))
    CRASH();

  rec->used_bytes = MAX(rec->used_bytes, needed);
  memcpy(rec->buf + byte_offset, input, len);
  rec->buf[byte_offset + len] = 0;

  off->byte_count += len + 1;
  off->command_count++;

  return true;
}

bool
record_can_playback(const Record_t *rec, const RecordOffset_t *off)
{
  uint32_t read_offset = off->byte_count;
  if (read_offset >= rec->used_bytes)
    return false;

  return true;
}

bool
record_playback(const Record_t *rec, RecordEvent_t handler,
                RecordOffset_t *off)
{
  uint32_t read_offset = off->byte_count;
  if (read_offset >= rec->used_bytes)
    return false;

  size_t length = strlen(&rec->buf[read_offset]);
  handler(length, &rec->buf[read_offset]);

  off->byte_count += length + 1;
  off->command_count++;

  return true;
}

const char *
record_peek(const Record_t *rec, const RecordOffset_t *off)
{
  uint32_t read_offset = off->byte_count;
  if (read_offset >= rec->used_bytes)
    return 0;

  return &rec->buf[read_offset];
}

const char *
record_read(const Record_t *rec, RecordOffset_t *off, size_t *len)
{
  uint32_t read_offset = off->byte_count;
  if (read_offset >= rec->used_bytes) {
    *len = 0;
    return 0;
  }

  const char *cmd = &rec->buf[read_offset];
  size_t length = strlen(cmd);

  off->byte_count += length + 1;
  off->command_count++;

  *len = length;
  return cmd;
}

bool
record_read_bytes(const Record_t *rec, size_t len, char buffer[static len],
                  RecordOffset_t *off)
{
  uint32_t read_offset = off->byte_count;
  if (read_offset + len >= rec->used_bytes)
    return false;

  memcpy(buffer, &rec->buf[read_offset], len);
  off->byte_count += len + 1;
  off->command_count++;

  return true;
}

void
record_playback_all(const Record_t *rec, RecordEvent_t handler)
{
  size_t length = { 0 };
  for (int i = 0; i < rec->used_bytes; i += (length + 1)) {
    length = strlen(rec->buf + i);
    handler(length, rec->buf + i);
  }
}

bool
record_to_disk(const Record_t *rec, const char *pathname)
{
  FILE *fd = fopen(pathname, "w+");
  if (!fd)
    return false;

  fwrite(rec->buf, rec->used_bytes, 1, fd);
  fclose(fd);
  return true;
}

size_t
record_length(const Record_t *rec)
{
  return rec->used_bytes;
}

int
record_compare(const Record_t *lhs, const Record_t *rhs)
{
  return memcmp(lhs->buf, rhs->buf, MIN(lhs->used_bytes, rhs->used_bytes));
}

Record_t *
record_clone(const Record_t *rec)
{
  Record_t *clone = malloc(sizeof(Record_t));
  *clone = *rec;
  clone->buf = malloc(clone->alloc_bytes);
  memcpy(clone->buf, rec->buf, rec->used_bytes);
  return clone;
}

void
record_reset(Record_t *rec)
{
  rec->used_bytes = 0;
}

void
record_free(Record_t *rec)
{
  if (rec) {
    FREE(rec->buf);
  }
  FREE(rec);
}

void
record_debug(Record_t *rec)
{
  printf("%p: %d alloc %d used\n", rec, rec->alloc_bytes, rec->used_bytes);
}

