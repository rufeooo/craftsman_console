
#include <stdio.h>
#include <string.h>

#include "macro.h"
#include "sys_record.h"

typedef struct Record_s {
  char *restrict buf;
  int allocBuf;
  int usedBuf;
  int writeOffset;
  int readOffset;
} Record_t;

Record_t *
sys_recordAlloc()
{
  Record_t *rec = malloc(sizeof(Record_t));
  rec->usedBuf = 0;
  rec->readOffset = rec->writeOffset = 0;
  rec->allocBuf = 4 * 1024;
  rec->buf = malloc(rec->allocBuf);

  return rec;
}

static bool
sys_recordRealloc(Record_t *rec, int bytesNeeded)
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
sys_recordAppend(Record_t *rec, size_t len, const char *input)
{
  size_t needed = len + rec->writeOffset + 1;
  if (!sys_recordRealloc(rec, needed - rec->allocBuf))
    return false;

  memcpy(rec->buf + rec->writeOffset, input, len);
  rec->writeOffset += len;
  rec->buf[rec->writeOffset++] = 0;
  rec->usedBuf = MAX(rec->usedBuf, rec->writeOffset);

  return true;
}

void
sys_recordPlaybackAll(Record_t *rec, sys_recordEvent handler)
{
  size_t length = { 0 };
  for (int i = 0; i < rec->usedBuf; i += (length + 1)) {
    length = strlen(rec->buf + i);
    handler(length, rec->buf + i);
  }
}

bool
sys_recordPlayback(Record_t *rec, sys_recordEvent handler)
{
  if (rec->readOffset >= rec->usedBuf)
    return false;

  size_t length = strlen(&rec->buf[rec->readOffset]);
  handler(length, &rec->buf[rec->readOffset]);
  rec->readOffset += length + 1;

  return true;
}

void
sys_recordSeekR(Record_t *rec, size_t nth)
{
  int i = 0;
  for (; nth > 0 && i < rec->usedBuf; ++i) {
    if (rec->buf[i] == 0) {
      --nth;
    }
  }

  rec->readOffset = i;
}

void
sys_recordSeekW(Record_t *rec, size_t nth)
{
  int i = 0;
  for (; nth > 0 && i < rec->usedBuf; ++i) {
    if (rec->buf[i] == 0) {
      --nth;
    }
  }

  rec->writeOffset = i;
}

Record_t *
sys_recordClone(Record_t *rec)
{
  Record_t *clone = malloc(sizeof(Record_t));
  *clone = *rec;
  clone->buf = malloc(clone->allocBuf);
  memcpy(clone->buf, rec->buf, rec->usedBuf);
  return clone;
}

void
sys_recordFree(Record_t *rec)
{
  FREE(rec->buf);
  FREE(rec);
}

void
sys_recordDebug(Record_t *rec)
{
  printf("%p: %d alloc %d used %d wo %d ro\n", rec, rec->allocBuf,
         rec->usedBuf, rec->writeOffset, rec->readOffset);
}

