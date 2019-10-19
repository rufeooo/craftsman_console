
#include <stdio.h>
#include <string.h>

#include "macro.h"
#include "record.h"

typedef struct Record_s {
  char *restrict buf;
  int allocBuf;
  int usedBuf;
  int writeOffset;
} Record_t;

Record_t *
record_alloc()
{
  Record_t *rec = malloc(sizeof(Record_t));
  rec->usedBuf = 0;
  rec->allocBuf = 4 * 1024;
  rec->buf = malloc(rec->allocBuf);

  return rec;
}

static bool
record_realloc(Record_t *rec, int bytesNeeded)
{
  if (bytesNeeded <= 0)
    return true;

  char *const newBuf = realloc(rec->buf, rec->allocBuf * 2);
  if (!newBuf)
    return false;

  rec->buf = newBuf;
  rec->allocBuf *= 2;

  return true;
}

bool
record_append(Record_t *rec, size_t len, const char *input)
{
  size_t needed = len + rec->writeOffset + 1;
  if (!record_realloc(rec, needed - rec->allocBuf))
    return false;

  memcpy(rec->buf + rec->writeOffset, input, len);
  rec->writeOffset += len;
  rec->buf[rec->writeOffset++] = 0;
  rec->usedBuf = MAX(rec->usedBuf, rec->writeOffset);

  return true;
}

void
record_playback_all(Record_t *rec, RecordEvent_t handler)
{
  size_t length = { 0 };
  for (int i = 0; i < rec->usedBuf; i += (length + 1)) {
    length = strlen(rec->buf + i);
    handler(length, rec->buf + i);
  }
}

bool
record_playback(Record_t *rec, RecordEvent_t handler, int *readOffset)
{
  if (*readOffset >= rec->usedBuf)
    return false;

  size_t length = strlen(&rec->buf[*readOffset]);
  handler(length, &rec->buf[*readOffset]);
  *readOffset += length + 1;

  return true;
}

void
record_seek_write(Record_t *rec, size_t nth)
{
  int i = 0;
  for (; nth > 0 && i < rec->usedBuf; ++i) {
    if (rec->buf[i] == 0) {
      --nth;
    }
  }

  rec->writeOffset = i;
}

size_t
record_length(Record_t *rec)
{
  return rec->usedBuf;
}

size_t
record_write_offset(Record_t *rec)
{
  return rec->writeOffset;
}

Record_t *
record_clone(Record_t *rec)
{
  Record_t *clone = malloc(sizeof(Record_t));
  *clone = *rec;
  clone->buf = malloc(clone->allocBuf);
  memcpy(clone->buf, rec->buf, rec->usedBuf);
  return clone;
}

void
record_reset(Record_t *rec)
{
  rec->writeOffset = rec->usedBuf = 0;
}

void
record_free(Record_t *rec)
{
  FREE(rec->buf);
  FREE(rec);
}

void
record_debug(Record_t *rec)
{
  printf("%p: %d alloc %d used %d wo\n", rec, rec->allocBuf, rec->usedBuf,
         rec->writeOffset);
}

