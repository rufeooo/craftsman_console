
#include <stdio.h>
#include <string.h>

#include "macro.h"
#include "record.h"

typedef struct Record_s {
  char *restrict buf;
  uint32_t alloc_bytes;
  uint32_t used_bytes;
} Record_t;

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
record_append(Record_t *rec, size_t len, const char *input,
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

LOCAL const char *
record_read(const Record_t *rec, RecordOffset_t *off, size_t *len)
{
  uint32_t read_offset = off->byte_count;
  if (read_offset >= rec->used_bytes)
    return 0;

  const char *cmd = &rec->buf[read_offset];
  size_t length = strlen(cmd);

  off->byte_count += length + 1;
  off->command_count++;

  *len = length;
  return cmd;
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

size_t
record_length(Record_t *rec)
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

