
#include <string.h>

#include "macro.h"
#include "sys_record.h"

typedef struct Record_s {
  char *restrict buf;
  int allocBuf;
  int usedBuf;
  bool playback;
} Record_t;

Record_t *
sys_recordInit()
{
  Record_t *rec = calloc(sizeof(Record_t), 1);
  rec->usedBuf = 0;
  rec->allocBuf = 4 * 1024;
  free(rec->buf);
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
  if (rec->playback)
    return false;
  if (!sys_recordRealloc(rec, (len + rec->usedBuf + 1) - rec->allocBuf))
    return false;

  memcpy(rec->buf + rec->usedBuf, input, len);
  rec->usedBuf += len;
  rec->buf[rec->usedBuf++] = 0;

  return true;
}

void
sys_recordPlayback(Record_t *rec, sys_recordEvent handler)
{
  rec->playback = true;
  size_t length = { 0 };
  for (int i = 0; i < rec->usedBuf; i += (length + 1)) {
    length = strlen(rec->buf + i);
    handler(length, rec->buf + i);
  }
  rec->playback = false;
}

void
sys_recordShutdown(Record_t *rec)
{
  rec->usedBuf = 0;
  rec->allocBuf = 0;
  FREE(rec->buf);
  FREE(rec);
}
